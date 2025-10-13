#include "database.h"
#include <QDir>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QImage>
#include <QBuffer>

const QString Database::DATABASE_NAME = "psio_games.db";

Database::Database() : QObject(nullptr) {
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("covers.db");
    }
    
    if (!initDatabase()) {
        qDebug() << "Erro ao inicializar banco de dados:" << db.lastError().text();
    }
}

Database::~Database() {
    QString connection = db.connectionName();
    if (db.isOpen()) {
        db.close();
    }
    QSqlDatabase::removeDatabase(connection); // Add this line to remove the connection
}

bool Database::init() {
    if (!openDatabase()) {
        return false;
    }
    return createTables();
}

bool Database::openDatabase() {
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbPath);
    dbPath = QDir(dbPath).filePath(DATABASE_NAME);

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "Erro ao abrir banco de dados:" << db.lastError().text();
        return false;
    }
    return true;
}

bool Database::createTables() {
    QSqlQuery query(db);
    
    // Tabela de jogos
    if (!query.exec("CREATE TABLE IF NOT EXISTS games ("
                   "id TEXT PRIMARY KEY,"
                   "name TEXT,"
                   "directory TEXT,"
                   "disc_number INTEGER,"
                   "has_cover INTEGER,"
                   "has_cu2 INTEGER"
                   ")")) {
        qDebug() << "Erro ao criar tabela games:" << query.lastError().text();
        return false;
    }

    // Tabela de covers
    if (!query.exec("CREATE TABLE IF NOT EXISTS covers ("
                   "game_id TEXT PRIMARY KEY,"
                   "cover_data BLOB"
                   ")")) {
        qDebug() << "Erro ao criar tabela covers:" << query.lastError().text();
        return false;
    }

    // Collection discs table
    if (!query.exec("CREATE TABLE IF NOT EXISTS disc_collection ("
                   "game_id TEXT,"
                   "disc_path TEXT,"
                   "FOREIGN KEY(game_id) REFERENCES games(id)"
                   ")")) {
        qDebug() << "Erro ao criar tabela disc_collection:" << query.lastError().text();
        return false;
    }

    // Tabela de arquivos BIN
    if (!query.exec("CREATE TABLE IF NOT EXISTS bin_files ("
                   "game_id TEXT,"
                   "file_name TEXT,"
                   "file_path TEXT,"
                   "FOREIGN KEY(game_id) REFERENCES games(id)"
                   ")")) {
        qDebug() << "Erro ao criar tabela bin_files:" << query.lastError().text();
        return false;
    }

    return true;
}

bool Database::initDatabase() {
    if (!db.open()) {
        qDebug() << "Erro ao abrir banco de dados:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    
    // Create games table if it does not exist
    if (!query.exec("CREATE TABLE IF NOT EXISTS games ("
                   "id TEXT PRIMARY KEY,"
                   "name TEXT,"
                   "directory TEXT,"
                   "disc_number INTEGER,"
                   "has_cover INTEGER,"
                   "has_cu2 INTEGER"
                   ")")) {
        qDebug() << "Erro ao criar tabela games:" << query.lastError().text();
        return false;
    }

    // Create covers table if it does not exist
    if (!query.exec("CREATE TABLE IF NOT EXISTS covers ("
                   "game_id TEXT PRIMARY KEY,"
                   "cover_data BLOB"
                   ")")) {
        qDebug() << "Erro ao criar tabela covers:" << query.lastError().text();
        return false;
    }

    return true;
}

QByteArray Database::downloadCoverArt(const QString& gameId) {
    // Format the ID correctly (remove dots and replace underscore with hyphen)
    QString formattedId = gameId;
    formattedId.remove('.');  // Remove qualquer ponto que exista
    formattedId.replace("_", "-");  // Replace underscore with hyphen
    
    // URL base do servidor de capas
    QString baseUrl = "https://raw.githubusercontent.com/xlenore/psx-covers/refs/heads/main/covers/default/";
    QString url = baseUrl + formattedId + ".jpg";
    
    qDebug() << "Tentando baixar capa de:" << url;
    
    // Criar request
    QNetworkRequest request{QUrl(url)};
    QNetworkReply* reply = networkManager.get(request);
    
    // Esperar pela resposta
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray jpgData = reply->readAll();
        qDebug() << "Capa JPG baixada com sucesso para" << gameId;
        
        // Converter JPG para BMP com redimensionamento
        QImage image;
        if (image.loadFromData(jpgData, "JPG")) {
            // Redimensionar para 80x84
            QImage resized = image.scaled(80, 84, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            
            // Converter para BMP
            QBuffer buffer;
            buffer.open(QIODevice::WriteOnly);
            resized.save(&buffer, "BMP");
            QByteArray bmpData = buffer.data();
            
            // Salvar BMP no banco de dados
            QSqlQuery query(db);
            query.prepare("INSERT OR REPLACE INTO covers (game_id, cover_data) VALUES (:id, :data)");
            query.bindValue(":id", gameId);
            query.bindValue(":data", bmpData);
            
            if (!query.exec()) {
                qDebug() << "Erro ao salvar capa no banco:" << query.lastError().text();
            }
            
            reply->deleteLater();
            return bmpData;
        } else {
            qDebug() << "Erro ao converter imagem JPG para BMP";
        }
    }
    
    qDebug() << "Erro ao baixar capa:" << reply->errorString() << "para URL:" << url;
    reply->deleteLater();
    return QByteArray();
}

QByteArray Database::getCoverArt(const QString& gameId) {
    QString formattedId = gameId;
    formattedId.replace("-", "_");
    
    // Primeiro tenta buscar no banco local
    QSqlQuery query(db);
    query.prepare("SELECT cover_data FROM covers WHERE game_id = :id");
    query.bindValue(":id", formattedId);
    
    if (query.exec() && query.next()) {
        qDebug() << "Capa encontrada no banco local para" << gameId;
        return query.value(0).toByteArray();
    }
    
    qDebug() << "Capa não encontrada no banco local para" << gameId << ". Tentando download...";
    // If not found in database, try to download
    return downloadCoverArt(gameId);
}

bool Database::saveGame(const Game& game) {
    QSqlQuery query(db);
    
    // Primeiro tenta atualizar
    query.prepare("UPDATE games SET name = :name, directory = :dir, "
                 "disc_number = :disc, has_cover = :cover, has_cu2 = :cu2 "
                 "WHERE id = :id");
    
    query.bindValue(":id", QString::fromStdString(game.getId()));
    query.bindValue(":name", QString::fromStdString(game.getDirectoryName()));
    query.bindValue(":dir", QString::fromStdString(game.getDirectoryPath()));
    query.bindValue(":disc", game.getDiscNumber());
    query.bindValue(":cover", game.hasCoverArt());
    query.bindValue(":cu2", game.hasCu2());
    
    if (!query.exec()) {
        // Se falhou em atualizar, tenta inserir
        query.prepare("INSERT INTO games (id, name, directory, disc_number, has_cover, has_cu2) "
                     "VALUES (:id, :name, :dir, :disc, :cover, :cu2)");
        
        query.bindValue(":id", QString::fromStdString(game.getId()));
        query.bindValue(":name", QString::fromStdString(game.getDirectoryName()));
        query.bindValue(":dir", QString::fromStdString(game.getDirectoryPath()));
        query.bindValue(":disc", game.getDiscNumber());
        query.bindValue(":cover", game.hasCoverArt());
        query.bindValue(":cu2", game.hasCu2());
        
        if (!query.exec()) {
            qDebug() << "Erro ao salvar jogo:" << game.getId().c_str() << "-" << query.lastError().text();
            return false;
        }
    }
    
    return true;
}

bool Database::addGame(const Game& game) {
    QSqlQuery query(db);
    
    // Inserir jogo
    query.prepare("INSERT INTO games (id, name, directory, disc_number, has_cover, has_cu2) "
                 "VALUES (:id, :name, :dir, :disc_num, :has_cover, :has_cu2)");
    
    query.bindValue(":id", QString::fromStdString(game.getId()));
    query.bindValue(":name", QString::fromStdString(game.getDirectoryName()));
    query.bindValue(":dir", QString::fromStdString(game.getDirectoryPath()));
    query.bindValue(":disc_num", game.getDiscNumber());
    query.bindValue(":has_cover", game.hasCoverArt());
    query.bindValue(":has_cu2", game.hasCu2());

    if (!query.exec()) {
        qDebug() << "Erro ao inserir jogo:" << query.lastError().text();
        return false;
    }

    // Insert disc collection
    for (const auto& disc : game.getDiscCollection()) {
        query.prepare("INSERT INTO disc_collection (game_id, disc_path) VALUES (:game_id, :disc_path)");
        query.bindValue(":game_id", QString::fromStdString(game.getId()));
        query.bindValue(":disc_path", QString::fromStdString(disc));
        
        if (!query.exec()) {
            qDebug() << "Erro ao inserir disco na coleção:" << query.lastError().text();
            return false;
        }
    }

    // Inserir arquivos BIN
    for (const auto& binFile : game.getCueSheet().getBinFiles()) {
        query.prepare("INSERT INTO bin_files (game_id, file_name, file_path) "
                     "VALUES (:game_id, :file_name, :file_path)");
        query.bindValue(":game_id", QString::fromStdString(game.getId()));
        query.bindValue(":file_name", QString::fromStdString(binFile.getFileName()));
        query.bindValue(":file_path", QString::fromStdString(binFile.getFilePath()));
        
        if (!query.exec()) {
            qDebug() << "Erro ao inserir arquivo BIN:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

std::vector<Game> Database::getAllGames() {
    std::vector<Game> games;
    QSqlQuery query(db);
    
    if (!query.exec("SELECT * FROM games")) {
        qDebug() << "Erro ao buscar jogos:" << query.lastError().text();
        return games;
    }

    while (query.next()) {
        QString gameId = query.value("id").toString();
        
        // Search for disc collection
        QSqlQuery discQuery(db);
        std::vector<std::string> discCollection;
        discQuery.prepare("SELECT disc_path FROM disc_collection WHERE game_id = :game_id");
        discQuery.bindValue(":game_id", gameId);
        
        if (discQuery.exec()) {
            while (discQuery.next()) {
                discCollection.push_back(discQuery.value("disc_path").toString().toStdString());
            }
        }

        // Buscar arquivos BIN
        QSqlQuery binQuery(db);
        binQuery.prepare("SELECT file_name, file_path FROM bin_files WHERE game_id = :game_id");
        binQuery.bindValue(":game_id", gameId);
        
        CueSheet cueSheet("", "", ""); // Temporary
        if (binQuery.exec()) {
            while (binQuery.next()) {
                cueSheet.addBinFile(BinFile(
                    binQuery.value("file_name").toString().toStdString(),
                    binQuery.value("file_path").toString().toStdString()
                ));
            }
        }

        games.emplace_back(
            query.value("name").toString().toStdString(),
            query.value("directory").toString().toStdString(),
            gameId.toStdString(),
            query.value("disc_number").toInt(),
            discCollection,
            cueSheet,
            query.value("has_cover").toBool(),
            query.value("has_cu2").toBool()
        );
    }

    return games;
}

bool Database::updateGame(const Game& game) {
    QSqlQuery query(db);
    
    // Atualizar jogo
    query.prepare("UPDATE games SET "
                 "name = :name, "
                 "directory = :dir, "
                 "disc_number = :disc_num, "
                 "has_cover = :has_cover, "
                 "has_cu2 = :has_cu2 "
                 "WHERE id = :id");
    
    query.bindValue(":name", QString::fromStdString(game.getDirectoryName()));
    query.bindValue(":dir", QString::fromStdString(game.getDirectoryPath()));
    query.bindValue(":disc_num", game.getDiscNumber());
    query.bindValue(":has_cover", game.hasCoverArt());
    query.bindValue(":has_cu2", game.hasCu2());
    query.bindValue(":id", QString::fromStdString(game.getId()));

    if (!query.exec()) {
        qDebug() << "Erro ao atualizar jogo:" << query.lastError().text();
        return false;
    }

    // Remove old disc collection
    query.prepare("DELETE FROM disc_collection WHERE game_id = :game_id");
    query.bindValue(":game_id", QString::fromStdString(game.getId()));
    if (!query.exec()) {
        qDebug() << "Erro ao remover discos antigos:" << query.lastError().text();
        return false;
    }

    // Insert new disc collection
    for (const auto& disc : game.getDiscCollection()) {
        query.prepare("INSERT INTO disc_collection (game_id, disc_path) VALUES (:game_id, :disc_path)");
        query.bindValue(":game_id", QString::fromStdString(game.getId()));
        query.bindValue(":disc_path", QString::fromStdString(disc));
        
        if (!query.exec()) {
            qDebug() << "Erro ao inserir disco na coleção:" << query.lastError().text();
            return false;
        }
    }

    // Remover arquivos BIN antigos
    query.prepare("DELETE FROM bin_files WHERE game_id = :game_id");
    query.bindValue(":game_id", QString::fromStdString(game.getId()));
    if (!query.exec()) {
        qDebug() << "Erro ao remover arquivos BIN antigos:" << query.lastError().text();
        return false;
    }

    // Inserir novos arquivos BIN
    for (const auto& binFile : game.getCueSheet().getBinFiles()) {
        query.prepare("INSERT INTO bin_files (game_id, file_name, file_path) "
                     "VALUES (:game_id, :file_name, :file_path)");
        query.bindValue(":game_id", QString::fromStdString(game.getId()));
        query.bindValue(":file_name", QString::fromStdString(binFile.getFileName()));
        query.bindValue(":file_path", QString::fromStdString(binFile.getFilePath()));
        
        if (!query.exec()) {
            qDebug() << "Erro ao inserir arquivo BIN:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool Database::removeGame(const std::string& gameId) {
    QSqlQuery query(db);
    
    // Remover registros relacionados primeiro
    query.prepare("DELETE FROM disc_collection WHERE game_id = :game_id");
    query.bindValue(":game_id", QString::fromStdString(gameId));
    if (!query.exec()) {
        qDebug() << "Erro ao remover discos:" << query.lastError().text();
        return false;
    }

    query.prepare("DELETE FROM bin_files WHERE game_id = :game_id");
    query.bindValue(":game_id", QString::fromStdString(gameId));
    if (!query.exec()) {
        qDebug() << "Erro ao remover arquivos BIN:" << query.lastError().text();
        return false;
    }

    // Remover o jogo
    query.prepare("DELETE FROM games WHERE id = :game_id");
    query.bindValue(":game_id", QString::fromStdString(gameId));
    if (!query.exec()) {
        qDebug() << "Erro ao remover jogo:" << query.lastError().text();
        return false;
    }

    return true;
}

Game Database::getGame(const std::string& gameId) {
    QSqlQuery query(db);
    query.prepare("SELECT * FROM games WHERE id = :game_id");
    query.bindValue(":game_id", QString::fromStdString(gameId));
    
    if (!query.exec() || !query.next()) {
        qDebug() << "Erro ao buscar jogo:" << query.lastError().text();
        return Game("", "", "", 0, std::vector<std::string>(), CueSheet("", "", ""), false, false);
    }

    // Search for disc collection
    QSqlQuery discQuery(db);
    std::vector<std::string> discCollection;
    discQuery.prepare("SELECT disc_path FROM disc_collection WHERE game_id = :game_id");
    discQuery.bindValue(":game_id", QString::fromStdString(gameId));
    
    if (discQuery.exec()) {
        while (discQuery.next()) {
            discCollection.push_back(discQuery.value("disc_path").toString().toStdString());
        }
    }

    // Buscar arquivos BIN
    QSqlQuery binQuery(db);
    binQuery.prepare("SELECT file_name, file_path FROM bin_files WHERE game_id = :game_id");
    binQuery.bindValue(":game_id", QString::fromStdString(gameId));
    
    CueSheet cueSheet("", "", "");
    if (binQuery.exec()) {
        while (binQuery.next()) {
            cueSheet.addBinFile(BinFile(
                binQuery.value("file_name").toString().toStdString(),
                binQuery.value("file_path").toString().toStdString()
            ));
        }
    }

    return Game(
        query.value("name").toString().toStdString(),
        query.value("directory").toString().toStdString(),
        gameId,
        query.value("disc_number").toInt(),
        discCollection,
        cueSheet,
        query.value("has_cover").toBool(),
        query.value("has_cu2").toBool()
    );
}

QString Database::getGameTitle(const QString& gameId) {
    if (!db.isOpen()) {
        if (!openDatabase()) {
            return QString();
        }
    }
    
    QSqlQuery query(db);
    query.prepare("SELECT title FROM games WHERE id = :id");
    query.bindValue(":id", gameId);
    
    if (query.exec() && query.next()) {
        return query.value("title").toString();
    }
    
    return QString();
}

// ... other methods will be implemented as needed ...