#include "mainwindow.h"
#include <QDir>
#include <QHeaderView>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>
#include <filesystem>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "utils.h"
#include <QDebug>
#include "config.h"
#include "cu2generator.h"
#include "uiconfig.h"
#include "preferencesdialog.h"
#include <set>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QLibraryInfo>

namespace fs = std::filesystem;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      gameTable(nullptr),
      mergeBinFilesCheck(nullptr),
      createCu2Check(nullptr),
      fixInvalidNameCheck(nullptr),
      autoRenameCheck(nullptr),
      createMultiDiscCheck(nullptr),
      addCoverArtCheck(nullptr),
      processButton(nullptr),
      statusLabel(nullptr),
      progressBar(nullptr),
      languageCombo(nullptr),
      translator(nullptr),
      qtTranslator(nullptr)
{
    // Initialize translators
    translator = new QTranslator(this);
    qtTranslator = new QTranslator(this);
    
    // Apply custom style for dialogs
    setDialogStyle();
    
    // Set initial window size
    resize(800, 600);
    setMinimumSize(800, 600);
    
    Config::getInstance().load();
    setupUI();
    setupMenus();
    
    // Load database settings
    loadFromDatabase();

    // Restore last used folder
    if (!Config::getInstance().getLastDirectory().isEmpty()) {
        loadGames(Config::getInstance().getLastDirectory());
    }
    
    // Load initial language
    loadLanguage(currentLanguage);
}

MainWindow::~MainWindow() {
    // Close database connection
    QSqlDatabase db = QSqlDatabase::database();
    db.close();
    QSqlDatabase::removeDatabase(db.connectionName());
}

void MainWindow::setupUI() {
    setStyleSheet(UIConfig::MAIN_STYLE);
    
    // Widget central
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // SD Root section
    auto *sdRootGroup = new QGroupBox("SD Root", this);
    auto *sdRootLayout = new QHBoxLayout(sdRootGroup);
    
    selectDirButton = new QPushButton(tr("Browse"), this);
    searchBox = new QLineEdit(this);
    searchBox->setPlaceholderText(tr("Search games..."));
    
    // Language selection
    languageCombo = new QComboBox(this);
    languageCombo->addItem("Português", "pt_BR");
    languageCombo->addItem("English", "en_US");
    languageCombo->setCurrentText("Português");
    
    sdRootLayout->addWidget(selectDirButton);
    sdRootLayout->addWidget(searchBox, 1);
    sdRootLayout->addWidget(languageCombo);
    
    mainLayout->addWidget(sdRootGroup);

    // Files section
    gameTable = new QTableWidget(this);
    gameTable->setColumnCount(5);
    gameTable->setHorizontalHeaderLabels({tr("ID"), tr("Name"), tr("Disc"), tr("BIN Files"), tr("Status")});
    
    // Configurar larguras das colunas
    gameTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);  // ID
    gameTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // Nome
    gameTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);  // Disco
    gameTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);  // BIN Files
    gameTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);  // Status
    
    gameTable->setColumnWidth(0, 75);   // ID
    gameTable->setColumnWidth(2, 60);   // Disco
    gameTable->setColumnWidth(3, 80);   // BIN Files
    gameTable->setColumnWidth(4, 150);  // Status
    
    mainLayout->addWidget(gameTable);

    // Tools section
    auto *toolsGroup = new QGroupBox("Tools", this);
    auto *toolsLayout = new QGridLayout(toolsGroup);  // Definido o parent diretamente aqui
    
    mergeBinFilesCheck = new QCheckBox(tr("Merge Bin Files"), this);
    cu2ForAllCheck = new QCheckBox(tr("CU2 For All"), this);
    fixInvalidNameCheck = new QCheckBox(tr("Fix Invalid Name"), this);
    autoRenameCheck = new QCheckBox(tr("Auto Rename"), this);
    autoRenameCheck->setEnabled(true);  // Enable the checkbox
    addCoverArtCheck = new QCheckBox(tr("Add Cover Art"), this);
    createMultiDiscCheck = new QCheckBox(tr("Create Multi-Disc"), this);
    
    toolsLayout->addWidget(mergeBinFilesCheck, 0, 0);
    toolsLayout->addWidget(cu2ForAllCheck, 1, 0);
    toolsLayout->addWidget(fixInvalidNameCheck, 0, 1);
    toolsLayout->addWidget(autoRenameCheck, 1, 1);
    toolsLayout->addWidget(addCoverArtCheck, 0, 2);
    toolsLayout->addWidget(createMultiDiscCheck, 1, 2);
    
    mainLayout->addWidget(toolsGroup);

    // Progress section
    auto *progressGroup = new QGroupBox("Progress", this);
    auto *progressLayout = new QVBoxLayout(progressGroup);  // Definido o parent diretamente aqui
    
    progressBar = new QProgressBar(this);
    progressBar->setTextVisible(true);
    progressBar->setAlignment(Qt::AlignCenter);
    
    statusLabel = new QLabel(tr("Status: Ready"), this);
    processButton = new QPushButton(tr("Process"), this);
    processButton->setEnabled(false);
    
    progressLayout->addWidget(progressBar);
    progressLayout->addWidget(statusLabel);
    progressLayout->addWidget(processButton);
    
    mainLayout->addWidget(progressGroup);

    // Connect signals
    connect(selectDirButton, &QPushButton::clicked, this, &MainWindow::onSelectDirectory);
    connect(searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(processButton, &QPushButton::clicked, this, &MainWindow::onProcessGames);
    connect(gameTable, &QTableWidget::cellClicked, this, &MainWindow::onGameSelected);
    connect(languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onLanguageChanged);
}

void MainWindow::setupMenus() {
    // File Menu
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("Select Directory"), this, &MainWindow::onSelectDirectory);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Export Database"), this, &MainWindow::onExportDatabase);
    fileMenu->addAction(tr("Import Database"), this, &MainWindow::onImportDatabase);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), this, &QWidget::close);

    // Tools Menu
    toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(tr("Process Games"), this, &MainWindow::onProcessGames);
    toolsMenu->addAction(tr("Verify Files"), this, &MainWindow::onVerifyFiles);
    toolsMenu->addAction(tr("Fix CUE Files"), this, &MainWindow::onFixCueFiles);
    toolsMenu->addAction(tr("Create Backup"), this, &MainWindow::onCreateBackup);
    toolsMenu->addSeparator();
    toolsMenu->addAction(tr("Preferences"), this, &MainWindow::onPreferences);

    // Help Menu
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("About"), this, &MainWindow::onAbout);
}

void MainWindow::setDialogStyle()
{
    // Define a custom style sheet for dialogs
    QString dialogStyle = R"(
        QDialog, QMessageBox, QFileDialog {
            background-color: #2D2D30;
            color: #E0E0E0;
        }
        
        QDialog QLabel, QMessageBox QLabel, QFileDialog QLabel {
            color: #E0E0E0;
        }
        
        QDialog QPushButton, QMessageBox QPushButton, QFileDialog QPushButton {
            background-color: #3F3F46;
            color: #E0E0E0;
            border: 1px solid #555555;
            padding: 5px;
            border-radius: 3px;
        }
        
        QDialog QPushButton:hover, QMessageBox QPushButton:hover, QFileDialog QPushButton:hover {
            background-color: #505050;
        }
        
        QDialog QPushButton:pressed, QMessageBox QPushButton:pressed, QFileDialog QPushButton:pressed {
            background-color: #404040;
        }
        
        QDialog QLineEdit, QFileDialog QLineEdit {
            background-color: #333337;
            color: #E0E0E0;
            border: 1px solid #555555;
            padding: 3px;
        }
        
        QDialog QTreeView, QFileDialog QTreeView, QDialog QListView, QFileDialog QListView {
            background-color: #252526;
            color: #E0E0E0;
            border: 1px solid #555555;
        }
        
        QDialog QTreeView::item:selected, QFileDialog QTreeView::item:selected,
        QDialog QListView::item:selected, QFileDialog QListView::item:selected {
            background-color: #3F3F46;
        }
        
        QDialog QComboBox, QFileDialog QComboBox {
            background-color: #333337;
            color: #E0E0E0;
            border: 1px solid #555555;
            padding: 3px;
        }
        
        QDialog QComboBox QAbstractItemView, QFileDialog QComboBox QAbstractItemView {
            background-color: #252526;
            color: #E0E0E0;
            selection-background-color: #3F3F46;
        }
    )";
    
    qApp->setStyleSheet(qApp->styleSheet() + dialogStyle);
}

std::string MainWindow::extractGameId(const QString& binPath) {
    static const QStringList REGION_CODES = {
        "DTLS", "SCES", "SLES", "SLED", "SCED", "SCUS", "SLUS",
        "SLPS", "SCAJ", "SLKA", "SLPM", "SCPS", "SCPM", "PCPX",
        "PAPX", "PTPX", "LSP0", "LSP1", "LSP2", "LSP9", "SIPS",
        "ESPM", "SCZS", "SPUS", "PBPX", "LSP"
    };

    QFile file(binPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Não foi possível abrir o arquivo BIN:" << binPath;
        return "";
    }

    // Ler os primeiros 64KB do arquivo em blocos
    char buffer[4096];
    QByteArray content;
    qint64 totalRead = 0;
    while (totalRead < 65536) {  // 64KB
        qint64 bytesRead = file.read(buffer, sizeof(buffer));
        if (bytesRead <= 0) break;
        content.append(buffer, bytesRead);
        totalRead += bytesRead;
    }
    file.close();

    // Search for region codes in binary content
    for (const QString& code : REGION_CODES) {
        QByteArray searchCode = code.toLatin1() + "_";  // Add underscore for search
        int index = content.indexOf(searchCode);
        if (index != -1) {
            // Extract the code and number
            QByteArray idBytes = content.mid(index, 11);
            QString fullId = QString::fromLatin1(idBytes.constData(), idBytes.length()).trimmed();
            
            // Replace underscore with hyphen
            fullId.replace("_", "-");
            
            qDebug() << "ID encontrado:" << fullId << "para" << binPath;
            return fullId.toStdString();
        }
    }

    qDebug() << "Nenhum ID encontrado para:" << binPath;
    return "";
}

void MainWindow::onSelectDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select Directory"), 
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty()) return;

    // Limpar lista atual
    games.clear();
    gameTable->setRowCount(0);
    
    // Configurar progresso para scan
    progressBar->setMaximum(0);
    progressBar->setValue(0);
    processButton->setEnabled(false);
    statusLabel->setText(tr("Scanning directory..."));
    
    QApplication::processEvents();

    // First, find all directories that contain BIN files
    QDirIterator it(dir, QStringList() << "*.bin", QDir::Files, QDirIterator::Subdirectories);
    QSet<QString> gameDirs;
    
    while (it.hasNext()) {
        QString binPath = it.next();
        gameDirs.insert(QFileInfo(binPath).dir().absolutePath());
    }
    
    // Configurar progresso
    progressBar->setMaximum(gameDirs.size());
    progressBar->setValue(0);
    
    // Process each found directory
    int count = 0;
    for (const QString& gamePath : gameDirs) {
        QDir gameDir(gamePath);
        statusLabel->setText(QString("Processando: %1").arg(gameDir.dirName()));
        
        try {
            // Verificar se tem capa
            bool hasCoverArt = QFileInfo::exists(gameDir.filePath("cover.jpg")) ||
                              QFileInfo::exists(gameDir.filePath("cover.png")) ||
                              QFileInfo::exists(gameDir.filePath("cover.bmp"));
            
            // Verificar se tem CU2
            bool hasCu2 = QFileInfo::exists(gameDir.filePath(gameDir.dirName() + ".cu2"));
            
            // Encontrar arquivo CUE (se existir)
            QStringList cueFiles = gameDir.entryList({"*.cue"}, QDir::Files);
            CueSheet cueSheet;
            
            if (!cueFiles.isEmpty()) {
                QString cuePath = gameDir.filePath(cueFiles.first());
                cueSheet = CueSheet(
                    cueFiles.first().toStdString(),
                    cuePath.toStdString(),
                    gameDir.dirName().toStdString()
                );
            }

            // Encontrar todos os arquivos BIN
            QStringList binFiles = gameDir.entryList({"*.bin"}, QDir::Files);
            
            // Extrair o ID do jogo do arquivo BIN principal
            QString binPath = gameDir.filePath(gameDir.dirName() + ".bin");
            if (!QFile::exists(binPath) && !binFiles.isEmpty()) {
                binPath = gameDir.filePath(binFiles.first());
            }
            
            std::string gameId = extractGameId(binPath);
            if (gameId.empty()) {
                qDebug() << "Aviso: Não foi possível extrair ID do jogo:" << gameDir.dirName();
                gameId = "UNKNOWN";
            }
            
            // Criar objeto Game
            Game game(
                gameDir.dirName().toStdString(),
                gameDir.absolutePath().toStdString(),
                gameId,
                1,  // TODO: detect disc number
                std::vector<std::string>(),
                cueSheet,
                hasCoverArt,
                hasCu2
            );
            
            games.push_back(game);
        }
        catch (const std::exception& e) {
            qDebug() << "Erro ao processar" << gameDir.dirName() << ":" << e.what();
        }
        
        progressBar->setValue(++count);
        QApplication::processEvents();
    }

    // Atualizar interface
    updateGameList();
    processButton->setEnabled(!games.empty());
    progressBar->setValue(0);
    statusLabel->setText(tr("Games found: %1").arg(games.size()));
}

void MainWindow::loadGames(const QString &directory) {
    games.clear();
    QDir dir(directory);
    QStringList gameDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &gameDir : gameDirs) {
        QDir fullGameDir(dir.filePath(gameDir));
        QStringList cueFiles = fullGameDir.entryList({"*.cue"}, QDir::Files);
        
        if (cueFiles.isEmpty()) continue;

        // Check necessary files
        bool hasCoverArt = fullGameDir.exists("cover.jpg") || fullGameDir.exists("cover.png");
        bool hasCu2 = fullGameDir.exists(gameDir + ".cu2");
        
        // Extrair ID do jogo (assumindo formato SLUS-XXXXX)
        QRegularExpression idRegex("([A-Z]{4}-\\d{5})");
        auto match = idRegex.match(gameDir);
        QString gameId = match.hasMatch() ? match.captured(1) : "";

        // Encontrar arquivos BIN relacionados
        QStringList binFiles = fullGameDir.entryList({"*.bin"}, QDir::Files);
        
        // Criar CueSheet para o jogo
        CueSheet cueSheet(cueFiles[0].toStdString(),
                         fullGameDir.filePath(cueFiles[0]).toStdString(),
                         gameDir.toStdString());

        // Adicionar arquivos BIN ao CueSheet
        for (const QString &binFile : binFiles) {
            cueSheet.addBinFile(BinFile(
                binFile.toStdString(),
                fullGameDir.filePath(binFile).toStdString()
            ));
        }

        // Criar objeto Game
        games.emplace_back(
            gameDir.toStdString(),
            fullGameDir.path().toStdString(),
            gameId.toStdString(),
            1, // Disc number (will be updated later)
            std::vector<std::string>(), // Disc collection (will be updated later)
            cueSheet,
            hasCoverArt,
            hasCu2
        );
    }

    updateGameList();
    statusLabel->setText(tr("Games found: %1").arg(games.size()));
}

void MainWindow::updateGameList() {
    gameTable->setRowCount(0);
    
    for (const Game &game : games) {
        int row = gameTable->rowCount();
        gameTable->insertRow(row);

        // ID do jogo
        auto* idItem = new QTableWidgetItem(QString::fromStdString(game.getId()));
        idItem->setTextAlignment(Qt::AlignCenter);
        gameTable->setItem(row, 0, idItem);
        
        // Nome do jogo
        auto* nameItem = new QTableWidgetItem(QString::fromStdString(game.getDirectoryName()));
        nameItem->setToolTip(QString::fromStdString(game.getDirectoryPath()));
        gameTable->setItem(row, 1, nameItem);
        
        // Disc number
        auto* discItem = new QTableWidgetItem(QString::number(game.getDiscNumber()));
        discItem->setTextAlignment(Qt::AlignCenter);
        gameTable->setItem(row, 2, discItem);

        // Number of BIN files
        QDir gameDir(QString::fromStdString(game.getDirectoryPath()));
        QStringList binFiles = gameDir.entryList({"*.bin"}, QDir::Files);
        auto* binCountItem = new QTableWidgetItem(QString::number(binFiles.size()));
        binCountItem->setTextAlignment(Qt::AlignCenter);
        gameTable->setItem(row, 3, binCountItem);
        
        // Status com cores
        QStringList statusList;
        QColor bgColor = UIConfig::ERROR_COLOR;
        
        if (!game.hasCoverArt()) {
            statusList << tr("No cover");
        }
        
        if (!game.hasCu2()) {
            statusList << tr("No CU2");
        }
        
        // Check invalid name
        if (!isValidGameName(game.getDirectoryName())) {
            statusList << tr("Invalid name");
        }
        
        QString status = statusList.isEmpty() ? tr("OK") : statusList.join(", ");
        
        if (statusList.isEmpty()) {
            bgColor = UIConfig::SUCCESS_COLOR;
        }
        
        auto* statusItem = new QTableWidgetItem(status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setBackground(bgColor);
        statusItem->setForeground(Qt::white);
        gameTable->setItem(row, 4, statusItem);
    }

    statusLabel->setText(tr("Games found: %1").arg(games.size()));
}

bool MainWindow::isValidGameName(const std::string& name) {
    QString qName = QString::fromStdString(name);
    
    // Verificar tamanho
    if (qName.length() > 60) {
        return false;
    }
    
    // Check invalid characters
    if (qName.contains(QRegularExpression("[.\\/:*?\"<>|]"))) {
        return false;
    }
    
    return true;
}

void MainWindow::onGameSelected(int row, int column) {
    if (row >= 0 && row < static_cast<int>(games.size())) {
        const Game &selectedGame = games[row];
        QString details = QString("Jogo: %1\n"
                                "ID: %2\n"
                                "Caminho: %3\n"
                                "Arquivos BIN: %4\n"
                                "CUE: %5")
                             .arg(QString::fromStdString(selectedGame.getDirectoryName()))
                             .arg(QString::fromStdString(selectedGame.getId()))
                             .arg(QString::fromStdString(selectedGame.getDirectoryPath()))
                             .arg(selectedGame.getCueSheet().getBinFileCount())
                             .arg(QString::fromStdString(selectedGame.getCueSheet().getFileName()));

        QMessageBox::information(this, tr("Game Details"), details);
    }
}

void MainWindow::onSearchTextChanged(const QString &text) {
    for (int row = 0; row < gameTable->rowCount(); ++row) {
        bool match = false;
        for (int col = 0; col < gameTable->columnCount(); ++col) {
            QTableWidgetItem *item = gameTable->item(row, col);
            if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        gameTable->setRowHidden(row, !match);
    }
}

void MainWindow::onProcessGames() {
    int errors = 0;
    QStringList errorList;
    
    // Desabilitar a interface durante o processamento
    setUiEnabled(false);
    
    progressBar->setMaximum(games.size());
    progressBar->setValue(0);
    
    for (size_t i = 0; i < games.size(); ++i) {
        Game& game = games[i];
        
        try {
            // 1. Merge Bin Files
            if (mergeBinFilesCheck->isChecked()) {
                QDir gameDir(QString::fromStdString(game.getDirectoryPath()));
                QStringList binFiles = gameDir.entryList({"*.bin"}, QDir::Files);
                
                if (binFiles.size() > 1) {
                    statusLabel->setText(tr("Merging BIN files: ") + 
                                      QString::fromStdString(game.getDirectoryName()));
                    mergeBinFiles(game);
                }
            }
            
            // 2. Cu2 For All
            if (cu2ForAllCheck->isChecked()) {
                statusLabel->setText(tr("Creating CU2: ") + 
                                  QString::fromStdString(game.getDirectoryName()));
                generateCu2File(game);
            }
            
            // 3. Fix Invalid Name
            if (fixInvalidNameCheck->isChecked()) {
                statusLabel->setText(tr("Fixing name: ") + 
                                  QString::fromStdString(game.getDirectoryName()));
                if (!isValidGameName(game.getDirectoryName())) {
                    fixGameName(game);
                }
            }
            
            // 4. Auto Rename
            if (autoRenameCheck->isChecked()) {
                statusLabel->setText(tr("Renaming: ") + 
                                  QString::fromStdString(game.getDirectoryName()));
                autoRenameGame(game);
            }
            
            // 5. Create Multi-Disc
            if (createMultiDiscCheck->isChecked() && isMultiDisc(game)) {
                statusLabel->setText(tr("Processing multi-disc: ") + 
                                  QString::fromStdString(game.getDirectoryName()));
                processMultiDisc(game);
            }
            
            // 6. Add Cover Art
            if (addCoverArtCheck->isChecked() && !game.hasCoverArt()) {
                statusLabel->setText(tr("Downloading cover: ") + 
                                  QString::fromStdString(game.getDirectoryName()));
                
                // First, check if there is already a .bmp in the directory
                QDir gameDir(QString::fromStdString(game.getDirectoryPath()));
                QStringList bmpFiles = gameDir.entryList({"*.bmp"}, QDir::Files);
                
                if (!bmpFiles.isEmpty()) {
                    // Renomear o primeiro .bmp encontrado para cover.bmp
                    QString oldPath = gameDir.filePath(bmpFiles.first());
                    QString newPath = gameDir.filePath("cover.bmp");
                    
                    if (QFile::exists(newPath)) {
                        QFile::remove(newPath);  // Remove cover.bmp existente se houver
                    }
                    
                    if (QFile::rename(oldPath, newPath)) {
                        game.setCoverArt(true);
                    }
                } else {
                    // Determinar o ID do jogo para baixar a capa
                    QString gameId;
                    
                    // Se for multi-disco, usar o ID do primeiro disco
                    if (isMultiDisc(game)) {
                        // Encontrar o primeiro disco
                        Game* firstDisc = &game;
                        int lowestDiscNum = game.extractDiscNumber();
                        
                        for (auto& otherGame : games) {
                            if (&otherGame != &game && game.isRelatedDisc(otherGame)) {
                                int discNum = otherGame.extractDiscNumber();
                                if (discNum < lowestDiscNum) {
                                    lowestDiscNum = discNum;
                                    firstDisc = &otherGame;
                                }
                            }
                        }
                        
                        gameId = QString::fromStdString(firstDisc->getId());
                    } else {
                        // Normal game, use its own ID
                        gameId = QString::fromStdString(game.getId());
                    }
                    
                    // Baixar a capa
                    QByteArray coverData = db.getCoverArt(gameId);
                    
                    if (!coverData.isEmpty()) {
                        QString coverPath = gameDir.filePath("cover.bmp");
                        QFile coverFile(coverPath);
                        if (coverFile.open(QIODevice::WriteOnly)) {
                            coverFile.write(coverData);
                            coverFile.close();
                            game.setCoverArt(true);
                        }
                    }
                }
            }
            
            progressBar->setValue(i + 1);
            QApplication::processEvents();
            
        } catch (const std::exception& e) {
            errors++;
            errorList.append(QString("%1: %2")
                .arg(QString::fromStdString(game.getDirectoryName()))
                .arg(e.what()));
        }
    }
    
    // Update interface after processing
    statusLabel->setText(tr("Updating game list..."));
    updateGameList();
    progressBar->setValue(0);
    
    // Reabilitar a interface
    setUiEnabled(true);
    
    if (errors > 0) {
        QString errorMessage = tr("There were %1 error(s):\n\n").arg(errors) + 
                             errorList.join("\n");
        QMessageBox::warning(this, tr("Processing Errors"), errorMessage);
    }
    
    statusLabel->setText(tr("Processing completed. %1 game(s) processed")
                        .arg(games.size()));
}

bool MainWindow::fixCueFile(Game& game) {
    QString cuePath = QString::fromStdString(game.getCueSheet().getFilePath());
    
    // Make backup if necessary
    if (Config::getInstance().createBackups) {
        QString backupPath = cuePath + ".backup";
        if (QFile::exists(backupPath)) {
            QFile::remove(backupPath);
        }
        if (!QFile::copy(cuePath, backupPath)) {
            return false;
        }
    }

    // Ler entradas do arquivo CUE
    auto entries = Utils::parseCueFile(cuePath);
    
    // If there are no valid entries, create new ones
    if (entries.empty()) {
        QDir gameDir(QString::fromStdString(game.getDirectoryPath()));
        QStringList binFiles = gameDir.entryList({"*.bin"}, QDir::Files);
        
        for (const QString& binFile : binFiles) {
            Utils::CueEntry entry;
            entry.type = "FILE";
            entry.file = binFile.toStdString();
            entry.trackNumber = entries.empty() ? 1 : entries.back().trackNumber + 1;
            entry.trackType = "BINARY";
            entries.push_back(entry);
        }
    }

    // Generate new CUE file content
    QString newContent = Utils::generateCueFile(entries);
    
    // Salvar novo arquivo CUE
    QFile file(cuePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << newContent;
    file.close();

    return true;
}

void MainWindow::onExportDatabase() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "Exportar Database", "", "JSON Files (*.json)");
    
    if (fileName.isEmpty()) return;

    QJsonArray gamesArray;
    for (const auto &game : games) {
        QJsonObject gameObj;
        gameObj["id"] = QString::fromStdString(game.getId());
        gameObj["directory_name"] = QString::fromStdString(game.getDirectoryName());
        gameObj["directory_path"] = QString::fromStdString(game.getDirectoryPath());
        gameObj["disc_number"] = game.getDiscNumber();
        gameObj["has_cover_art"] = game.hasCoverArt();
        gameObj["has_cu2"] = game.hasCu2();

        gamesArray.append(gameObj);
    }

    QJsonDocument doc(gamesArray);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        QMessageBox::information(this, "Sucesso", "Database exportada com sucesso!");
    } else {
        QMessageBox::critical(this, "Erro", "Erro ao exportar database!");
    }
}

void MainWindow::onImportDatabase() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Importar Database", "", "JSON Files (*.json)");
    
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Error opening file!"));
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isArray()) {
        games.clear();
        QJsonArray gamesArray = doc.array();
        
        for (const auto &gameValue : gamesArray) {
            QJsonObject gameObj = gameValue.toObject();
            
            // Create empty CueSheet (will be updated later)
            CueSheet cueSheet("", "", "");
            
            Game game(
                gameObj["directory_name"].toString().toStdString(),
                gameObj["directory_path"].toString().toStdString(),
                gameObj["id"].toString().toStdString(),
                gameObj["disc_number"].toInt(),
                std::vector<std::string>(),
                cueSheet,
                gameObj["has_cover_art"].toBool(),
                gameObj["has_cu2"].toBool()
            );
            
            games.push_back(game);
        }
        
        updateGameList();
        saveToDatabase();
        QMessageBox::information(this, "Sucesso", "Database importada com sucesso!");
    }
}

void MainWindow::onVerifyFiles() {
    if (games.empty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No games to verify!"));
        return;
    }

    progressBar->setMaximum(games.size());
    progressBar->setValue(0);
    processButton->setEnabled(false);
    
    int errors = 0;
    QStringList errorList;

    for (size_t i = 0; i < games.size(); ++i) {
        const Game& game = games[i];
        statusLabel->setText(QString("Verificando: %1").arg(
            QString::fromStdString(game.getDirectoryName())));

        if (!verifyGameFiles(game)) {
            errors++;
            errorList.append(QString::fromStdString(game.getDirectoryName()));
        }

        progressBar->setValue(i + 1);
        QApplication::processEvents();
    }

    processButton->setEnabled(true);
    progressBar->setValue(0);
    statusLabel->setText(tr("Games found: %1").arg(games.size()));

    if (errors > 0) {
        QMessageBox::warning(this, tr("Result"), 
            tr("Verification completed with %1 error(s):\n\n%2")
                .arg(errors)
                .arg(errorList.join("\n")));
    } else {
        QMessageBox::information(this, tr("Result"), tr("All files are OK!"));
    }
}

bool MainWindow::verifyGameFiles(const Game &game) {
    QDir gameDir(QString::fromStdString(game.getDirectoryPath()));
    
    // Verificar arquivo CUE
    QString cuePath = gameDir.filePath(QString::fromStdString(game.getCueSheet().getFileName()));
    if (!QFile::exists(cuePath))
        return false;
        
    // Verificar arquivos BIN
    for (const auto &binFile : game.getCueSheet().getBinFiles()) {
        QString binPath = gameDir.filePath(QString::fromStdString(binFile.getFileName()));
        if (!QFile::exists(binPath))
            return false;
    }
    
    // Verificar cover art
    if (game.hasCoverArt() && 
        !QFile::exists(gameDir.filePath("cover.jpg")) &&
        !QFile::exists(gameDir.filePath("cover.png")))
        return false;
        
    // Verificar CU2
    if (game.hasCu2() &&
        !QFile::exists(gameDir.filePath(QString::fromStdString(game.getDirectoryName() + ".cu2"))))
        return false;
        
    return true;
}

void MainWindow::onCreateBackup() {
    if (games.empty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No games for backup!"));
        return;
    }

    progressBar->setMaximum(games.size());
    progressBar->setValue(0);
    processButton->setEnabled(false);

    int processed = 0;
    for (size_t i = 0; i < games.size(); ++i) {
        const Game& game = games[i];
        statusLabel->setText(QString("Backup: %1").arg(
            QString::fromStdString(game.getDirectoryName())));

        // Your backup code here
        processed++;

        progressBar->setValue(i + 1);
        QApplication::processEvents();
    }

    processButton->setEnabled(true);
    progressBar->setValue(0);
    statusLabel->setText(tr("Games found: %1").arg(games.size()));

    QMessageBox::information(this, tr("Result"), 
        tr("Backup completed! %1 games processed.").arg(processed));
}

void MainWindow::onAbout() {
    QMessageBox::about(this, tr("About PSIO Assistant"),
        tr("PSIO Assistant v1.0\n\n"
        "A tool for managing PlayStation games for PSIO.\n\n"
        "Developed with Qt 6"));
}

void MainWindow::onFixCueFiles() {
    if (games.empty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No games to process!"));
        return;
    }

    progressBar->setMaximum(games.size());
    progressBar->setValue(0);
    processButton->setEnabled(false);

    int fixed = 0;
    for (size_t i = 0; i < games.size(); ++i) {
        Game& game = games[i];
        statusLabel->setText(QString("Verificando CUE: %1").arg(
            QString::fromStdString(game.getDirectoryName())));

        if (fixCueFile(game)) {
            fixed++;
        }

        progressBar->setValue(i + 1);
        QApplication::processEvents();
    }

    processButton->setEnabled(true);
    progressBar->setValue(0);
    statusLabel->setText(tr("Games found: %1").arg(games.size()));

    QMessageBox::information(this, tr("Result"), 
        tr("%1 CUE file(s) fixed").arg(fixed));
}

void MainWindow::saveToDatabase() {
    for (const auto& game : games) {
        if (!db.updateGame(game)) {
            if (!db.addGame(game)) {
                qDebug() << "Erro ao salvar jogo:" << QString::fromStdString(game.getId());
            }
        }
    }
}

void MainWindow::loadFromDatabase() {
    games = db.getAllGames();
    updateGameList();
    statusLabel->setText(tr("Games loaded: %1").arg(games.size()));
}

void MainWindow::contextMenuEvent(QContextMenuEvent* event) {
    QModelIndex index = gameTable->indexAt(gameTable->viewport()->mapFrom(this, event->pos()));
    
    if (index.isValid()) {
        QMenu contextMenu(this);
        
        QAction* processAction = contextMenu.addAction(tr("Process Game"));
        QAction* verifyAction = contextMenu.addAction(tr("Verify Files"));
        QAction* openFolderAction = contextMenu.addAction(tr("Open Folder"));
        QAction* removeAction = contextMenu.addAction(tr("Remove"));
        
        QAction* selectedAction = contextMenu.exec(event->globalPos());
        
        if (selectedAction) {
            const Game& game = games[index.row()];
            
            if (selectedAction == processAction) {
                processGame(game);
            }
            else if (selectedAction == verifyAction) {
                verifyGameFiles(game);
            }
            else if (selectedAction == openFolderAction) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(
                    QString::fromStdString(game.getDirectoryPath())));
            }
            else if (selectedAction == removeAction) {
                if (QMessageBox::question(this, tr("Confirm Removal"),
                    tr("Do you want to remove this game from the list?")) == QMessageBox::Yes) {
                    db.removeGame(game.getId());
                    games.erase(games.begin() + index.row());
                    updateGameList();
                }
            }
        }
    }
}

void MainWindow::processGame(const Game& game) {
    QString gameName = QString::fromStdString(game.getDirectoryName());
    
    progressBar->setMaximum(3);
    progressBar->setValue(0);
    processButton->setEnabled(false);
    
    statusLabel->setText(QString("Processando: %1").arg(gameName));
    
    // Processar etapas do jogo
    // ... your processing code here ...
    
    for (int i = 0; i < 3; i++) {
        progressBar->setValue(i + 1);
        QApplication::processEvents();
    }
    
    processButton->setEnabled(true);
    progressBar->setValue(0);
    statusLabel->setText(tr("Games found: %1").arg(games.size()));
}

void MainWindow::onPreferences() {
    PreferencesDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // Reload settings if necessary
        updateGameList();
    }
}

bool MainWindow::isMultiDisc(const Game& game) {
    QString dirName = QString::fromStdString(game.getDirectoryName());
    QString dirPath = QString::fromStdString(game.getDirectoryPath());
    QDir gameDir(dirPath);
    
    // Case 1: Directory with "(Disc X)" in the name
    if (dirName.contains(QRegularExpression(R"(\(Disc\s*\d+\))"))) {
        // Search for other directories with the same base name
        QDir parentDir = QFileInfo(dirPath).dir();
        QString baseName = QString::fromStdString(game.getBaseGameName());
        QStringList entries = parentDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        
        int discCount = 0;
        for (const QString& entry : entries) {
            if (entry.contains(baseName) && entry.contains(QRegularExpression(R"(\(Disc\s*\d+\))"))) {
                discCount++;
            }
        }
        return discCount > 1;
    }
    
    // Case 2: Multiple BIN/CUE or BIN/CU2 files with "DiscX" in the name
    QStringList binFiles = gameDir.entryList({"*.bin"}, QDir::Files);
    int discCount = 0;
    for (const QString& bin : binFiles) {
        if (bin.contains(QRegularExpression(R"(Disc\s*\d+)"))) {
            discCount++;
        }
    }
    return discCount > 1;
}

void MainWindow::processMultiDisc(Game& game) {
    // First check if it is really multi-disc
    if (!isMultiDisc(game)) {
        return;
    }
    
    // Encontrar todos os discos relacionados
    std::vector<Game*> relatedDiscs;
    relatedDiscs.push_back(&game);
    
    for (auto& otherGame : games) {
        if (&otherGame != &game && game.isRelatedDisc(otherGame)) {
            relatedDiscs.push_back(&otherGame);
        }
    }
    
    if (relatedDiscs.size() < 2) {
        return; // Not multi-disc
    }
    
    // Sort by disc number
    std::sort(relatedDiscs.begin(), relatedDiscs.end(),
        [](Game* a, Game* b) { return a->extractDiscNumber() < b->extractDiscNumber(); });
    
    // Create new directory
    QString baseDir = QFileInfo(QString::fromStdString(game.getDirectoryPath())).dir().absolutePath();
    QString newDirName = QString::fromStdString(game.getBaseGameName());
    QString newDirPath = baseDir + "/" + newDirName;
    
    QDir().mkpath(newDirPath);
    
    // Mover arquivos e criar MULTIDISC.LST
    QFile multiDiscFile(newDirPath + "/MULTIDISC.LST");
    if (!multiDiscFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("Não foi possível criar MULTIDISC.LST");
    }
    
    QTextStream out(&multiDiscFile);
    
    // List to store information about new discs
    struct DiscInfo {
        QString binName;
        QString discName;
        QString discPath;
    };
    std::vector<DiscInfo> discInfos;
    
    for (size_t i = 0; i < relatedDiscs.size(); i++) {
        Game* disc = relatedDiscs[i];
        QString discDir = QString::fromStdString(disc->getDirectoryPath());
        int discNum = i + 1;
        
        // Mover arquivos BIN/CUE/CU2
        QDirIterator it(discDir, QStringList() << "*.bin" << "*.cue" << "*.cu2",
                       QDir::Files);
        
        DiscInfo info;
        info.discName = newDirName + QString(" Disc %1").arg(discNum);
        info.discPath = newDirPath;
        
        while (it.hasNext()) {
            QString filePath = it.next();
            QFileInfo fileInfo(filePath);
            QString newName = info.discName + "." + fileInfo.suffix();
            
            QString newPath = newDirPath + "/" + newName;
            QFile::rename(filePath, newPath);
            
            // Adicionar ao MULTIDISC.LST se for BIN
            if (fileInfo.suffix().toLower() == "bin") {
                info.binName = newName;
                out << newName << "\n";
            }
        }
        
        discInfos.push_back(info);
        
        // Remove empty directory
        QDir(discDir).removeRecursively();
    }
    
    multiDiscFile.close();
    
    // Generate CU2 for each disc if necessary
    if (createMultiDiscCheck->isChecked()) {
        for (const auto& info : discInfos) {
            // Create a temporary Game for each disc
            Game tempGame(
                info.discName.toStdString(),
                info.discPath.toStdString(),
                game.getId(),
                1,
                std::vector<std::string>(),
                CueSheet(),
                false,
                false
            );
            
            // Gerar CU2
            generateCu2File(tempGame);
        }
    }
}

void MainWindow::mergeBinFiles(Game& game) {
    QDir gameDir(QString::fromStdString(game.getDirectoryPath()));
    QString gameName = QString::fromStdString(game.getDirectoryName());
    
    // Obter lista de BINs ordenada pelo CUE
    std::vector<QString> binFiles;
    QString cuePath = gameDir.filePath(gameName + ".cue");
    
    if (!QFile::exists(cuePath)) {
        throw std::runtime_error("Arquivo CUE não encontrado");
    }
    
    // Ler arquivo CUE para obter ordem dos BINs
    QFile cueFile(cuePath);
    if (!cueFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Não foi possível abrir arquivo CUE");
    }
    
    QTextStream in(&cueFile);
    QStringList cueLines;
    while (!in.atEnd()) {
        cueLines.append(in.readLine());
    }
    cueFile.close();
    
    // Extrair nomes dos arquivos BIN
    QRegularExpression fileRegex("FILE\\s+\"([^\"]+)\"");
    for (const QString& line : cueLines) {
        QRegularExpressionMatch match = fileRegex.match(line);
        if (match.hasMatch()) {
            binFiles.push_back(match.captured(1));
        }
    }
    
    if (binFiles.empty()) {
        throw std::runtime_error("Nenhum arquivo BIN encontrado no CUE");
    }
    
    // Create output BIN file
    QString outputPath = gameDir.filePath(gameName + "_merged.bin");
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        throw std::runtime_error("Não foi possível criar arquivo BIN de saída");
    }
    
    // Merge dos arquivos
    qint64 totalSize = 0;
    for (const QString& binName : binFiles) {
        QString binPath = gameDir.filePath(binName);
        QFile binFile(binPath);
        
        if (!binFile.open(QIODevice::ReadOnly)) {
            outputFile.close();
            throw std::runtime_error("Erro ao abrir arquivo BIN: " + binName.toStdString());
        }
        
        // Copiar dados
        const qint64 bufferSize = 1024 * 1024; // 1MB buffer
        char buffer[bufferSize];
        qint64 bytesRead;
        
        while ((bytesRead = binFile.read(buffer, bufferSize)) > 0) {
            if (outputFile.write(buffer, bytesRead) != bytesRead) {
                binFile.close();
                outputFile.close();
                throw std::runtime_error("Erro ao escrever no arquivo de saída");
            }
            totalSize += bytesRead;
        }
        
        binFile.close();
    }
    
    outputFile.close();
    
    // Renomear arquivo merged para o nome final
    QString finalPath = gameDir.filePath(gameName + ".bin");
    if (QFile::exists(finalPath)) {
        QFile::remove(finalPath);
    }
    QFile::rename(outputPath, finalPath);
    
    // Remover arquivos BIN originais
    for (const QString& binName : binFiles) {
        QString binPath = gameDir.filePath(binName);
        QFile::remove(binPath);
    }
    
    // Atualizar o CUE file
    QString newCuePath = gameDir.filePath(gameName + "_new.cue");
    QFile newCueFile(newCuePath);
    if (!newCueFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("Não foi possível criar novo arquivo CUE");
    }
    
    QTextStream out(&newCueFile);
    out << "FILE \"" << gameName << ".bin\" BINARY\n";
    
    // Extract track information from original CUE
    QRegularExpression trackRegex("\\s*TRACK\\s+(\\d+)\\s+(\\w+)");
    for (const QString& line : cueLines) {
        QRegularExpressionMatch match = trackRegex.match(line);
        if (match.hasMatch()) {
            out << "  TRACK " << match.captured(1) << " " << match.captured(2) << "\n";
        }
    }
    
    newCueFile.close();
    
    // Substituir CUE original
    QFile::remove(cuePath);
    QFile::rename(newCuePath, cuePath);
}

void MainWindow::fixGameName(Game& game) {
    QString oldName = QString::fromStdString(game.getDirectoryName());
    QString newName = oldName;
    
    // Remove invalid characters
    newName.replace(QRegularExpression("[.\\/:*?\"<>|]"), "_");
    
    // Limitar tamanho a 60 caracteres
    if (newName.length() > 60) {
        newName = newName.left(60);
    }
    
    if (oldName != newName) {
        QDir dir(QString::fromStdString(game.getDirectoryPath()));
        QDir parentDir = dir;
        parentDir.cdUp();
        
        // Rename directory
        if (parentDir.rename(oldName, newName)) {
            game.setDirectoryName(newName.toStdString());
            game.setDirectoryPath((parentDir.absolutePath() + "/" + newName).toStdString());
        }
    }
}

void MainWindow::autoRenameGame(Game& game) {
    QString dirPath = QString::fromStdString(game.getDirectoryPath());
    QString dirName = QString::fromStdString(game.getDirectoryName());
    QDir gameDir(dirPath);
    
    // Check if it is a multi-disc game
    bool isMultiDisc = false;
    
    // Check if directory name contains "Disc" or "CD" followed by a number
    QRegularExpression discRegex("(Disc|CD)\\s*\\d+", QRegularExpression::CaseInsensitiveOption);
    isMultiDisc = discRegex.match(dirName).hasMatch();
    
    // Also check if there is more than one .bin file in the directory
    QStringList binFiles = gameDir.entryList({"*.bin"}, QDir::Files);
    if (binFiles.size() > 1) {
        isMultiDisc = true;
    }
    
    QString newBaseName;
    
    if (isMultiDisc) {
        // Para jogos multi-disco, usar apenas "Disc X"
        int discNumber = game.getDiscNumber();
        newBaseName = QString("Disc %1").arg(discNumber);
    } else {
        // For normal games, use the directory name
        newBaseName = dirName;
    }
    
    // Nome do novo arquivo .bin
    QString newBinName = newBaseName + ".bin";
    
    // Renomear arquivos .bin
    for (const QString& binFile : binFiles) {
        if (binFile != newBinName) {
            QString oldPath = gameDir.absoluteFilePath(binFile);
            QString newPath = gameDir.absoluteFilePath(newBinName);
            QFile::rename(oldPath, newPath);
        }
    }
    
    // Renomear arquivos .cue
    QStringList cueFiles = gameDir.entryList({"*.cue"}, QDir::Files);
    for (const QString& cueFile : cueFiles) {
        QString newCueName = newBaseName + ".cue";
        if (cueFile != newCueName) {
            QString oldPath = gameDir.absoluteFilePath(cueFile);
            QString newPath = gameDir.absoluteFilePath(newCueName);
            QFile::rename(oldPath, newPath);
            
            // We also need to update the .cue file content to reflect the new .bin name
            updateCueFileContent(newPath, newBinName);
        }
    }
    
    // Renomear arquivos .cu2
    QStringList cu2Files = gameDir.entryList({"*.cu2"}, QDir::Files);
    for (const QString& cu2File : cu2Files) {
        QString newCu2Name = newBaseName + ".cu2";
        if (cu2File != newCu2Name) {
            QString oldPath = gameDir.absoluteFilePath(cu2File);
            QString newPath = gameDir.absoluteFilePath(newCu2Name);
            QFile::rename(oldPath, newPath);
        }
    }
}

// Helper method to update .cue file content
void MainWindow::updateCueFileContent(const QString& cuePath, const QString& newBinName) {
    QFile cueFile(cuePath);
    if (!cueFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    
    QString content = cueFile.readAll();
    cueFile.close();
    
    // Replace the .bin file name in the .cue file content
    QRegularExpression fileRegex("FILE\\s+\"(.+\\.bin)\"\\s+BINARY");
    QRegularExpressionMatch match = fileRegex.match(content);
    
    if (match.hasMatch()) {
        QString oldBinName = match.captured(1);
        content.replace(oldBinName, newBinName);
        
        if (cueFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&cueFile);
            out << content;
            cueFile.close();
        }
    }
}

void MainWindow::generateCu2File(const Game& game) {
    // Get the game directory path
    QString dirPath = QString::fromStdString(game.getDirectoryPath());
    
    // Find all .bin files in the directory
    QDir dir(dirPath);
    QStringList binFiles = dir.entryList({"*.bin"}, QDir::Files);
    
    // If there are no .bin files, there is nothing to do
    if (binFiles.isEmpty()) {
        qDebug() << "Nenhum arquivo .bin encontrado em" << dirPath;
        return;
    }
    
    // Para cada arquivo .bin, criar um arquivo .cu2 correspondente
    for (const QString& binFile : binFiles) {
        // Obter o caminho completo do arquivo .bin
        QString binPath = dir.filePath(binFile);
        
        // Criar o nome do arquivo .cu2
        QString cu2File = binFile;
        cu2File.replace(".bin", ".cu2", Qt::CaseInsensitive);
        QString cu2Path = dir.filePath(cu2File);
        
        // Encontrar o arquivo .cue correspondente
        QString cueFile = binFile;
        cueFile.replace(".bin", ".cue", Qt::CaseInsensitive);
        QString cuePath = dir.filePath(cueFile);
        
        qDebug() << "Gerando arquivo CU2:" << cu2Path;
        
        // Obter o tamanho do arquivo .bin
        QFile binFileObj(binPath);
        if (!binFileObj.open(QIODevice::ReadOnly)) {
            qDebug() << "Erro ao abrir o arquivo .bin:" << binPath;
            continue;
        }
        
        qint64 binSize = binFileObj.size();
        binFileObj.close();
        
        // Calculate the number of sectors (each sector has 2352 bytes)
        int sectors = binSize / 2352;
        
        // Calcular o tempo total em formato MM:SS:FF (minutos:segundos:frames)
        // Cada segundo tem 75 frames
        int totalFrames = sectors;
        int minutes = totalFrames / (75 * 60);
        int seconds = (totalFrames / 75) % 60;
        int frames = totalFrames % 75;
        
        qDebug() << "Tamanho do arquivo .bin:" << binSize << "bytes";
        qDebug() << "Número de setores:" << sectors;
        qDebug() << "Tempo total:" << minutes << ":" << seconds << ":" << frames;
        
        // Criar o arquivo .cu2
        QFile cu2FileObj(cu2Path);
        if (!cu2FileObj.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Erro ao criar o arquivo .cu2:" << cu2Path;
            continue;
        }
        
        // Write the .cu2 file content in text format
        QTextStream stream(&cu2FileObj);
        
        // Write the header
        stream << "ntracks 1\n";
        
        // Escrever o tamanho total
        stream << QString("size\t   %1:%2:%3\n")
                  .arg(minutes, 2, 10, QChar('0'))
                  .arg(seconds, 2, 10, QChar('0'))
                  .arg(frames, 2, 10, QChar('0'));
        
        // Write the data start (usually 00:02:00 for PlayStation games)
        stream << "data1\t   00:02:00\n\n";
        
        // Escrever o final da faixa
        // The track end is the total size + 2 seconds (150 frames)
        int endFrames = totalFrames + 150;
        int endMinutes = endFrames / (75 * 60);
        int endSeconds = (endFrames / 75) % 60;
        int endFramesPart = endFrames % 75;
        
        stream << QString("trk end\t %1:%2:%3\n")
                  .arg(endMinutes, 2, 10, QChar('0'))
                  .arg(endSeconds, 2, 10, QChar('0'))
                  .arg(endFramesPart, 2, 10, QChar('0'));
        
        cu2FileObj.close();
        
        qDebug() << "Arquivo CU2 gerado com sucesso:" << cu2Path;
        
        // Delete the .cue file after successful .cu2 file generation
        if (QFile::exists(cuePath)) {
            if (QFile::remove(cuePath)) {
                qDebug() << "Arquivo CUE excluído com sucesso:" << cuePath;
            } else {
                qDebug() << "Erro ao excluir o arquivo CUE:" << cuePath;
            }
        }
    }
}

// Add this method to enable/disable the interface
void MainWindow::setUiEnabled(bool enabled) {
    // Desabilitar/habilitar todos os controles relevantes
    selectDirButton->setEnabled(enabled);
    searchBox->setEnabled(enabled);
    processButton->setEnabled(enabled);
    gameTable->setEnabled(enabled);
    
    // Desabilitar/habilitar checkboxes
    mergeBinFilesCheck->setEnabled(enabled);
    cu2ForAllCheck->setEnabled(enabled);
    fixInvalidNameCheck->setEnabled(enabled);
    autoRenameCheck->setEnabled(enabled);
    addCoverArtCheck->setEnabled(enabled);
    createMultiDiscCheck->setEnabled(enabled);
    
    // Force interface update
    QApplication::processEvents();
}

void MainWindow::onLanguageChanged() {
    QString language = languageCombo->currentData().toString();
    loadLanguage(language);
}

void MainWindow::loadLanguage(const QString& language) {
    if (currentLanguage == language) {
        return;
    }
    
    currentLanguage = language;
    
    // Remove old translators
    qApp->removeTranslator(translator);
    qApp->removeTranslator(qtTranslator);
    
    // Load new translations
    QString translationPath = QCoreApplication::applicationDirPath() + "/translations/";
    if (translator->load("psio-assist_" + language, translationPath)) {
        qApp->installTranslator(translator);
    }
    
    if (qtTranslator->load("qt_" + language, QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        qApp->installTranslator(qtTranslator);
    }
    
    // Retranslate UI
    retranslateUI();
}

void MainWindow::retranslateUI() {
    // Update window title
    setWindowTitle(tr("PSIO Assist EX"));
    
    // Update menu items
    fileMenu->setTitle(tr("&File"));
    fileMenu->actions()[0]->setText(tr("Select Directory"));
    fileMenu->actions()[2]->setText(tr("Export Database"));
    fileMenu->actions()[3]->setText(tr("Import Database"));
    fileMenu->actions()[5]->setText(tr("Exit"));
    
    toolsMenu->setTitle(tr("&Tools"));
    toolsMenu->actions()[0]->setText(tr("Process Games"));
    toolsMenu->actions()[1]->setText(tr("Verify Files"));
    toolsMenu->actions()[2]->setText(tr("Fix CUE Files"));
    toolsMenu->actions()[3]->setText(tr("Create Backup"));
    toolsMenu->actions()[5]->setText(tr("Preferences"));
    
    helpMenu->setTitle(tr("&Help"));
    helpMenu->actions()[0]->setText(tr("About"));
    
    // Update UI elements
    selectDirButton->setText(tr("Browse"));
    searchBox->setPlaceholderText(tr("Search games..."));
    processButton->setText(tr("Process"));
    
    // Update table headers
    gameTable->setHorizontalHeaderLabels({
        tr("ID"), tr("Name"), tr("Disc"), tr("BIN Files"), tr("Status")
    });
    
    // Update group boxes
    if (auto sdRootGroup = findChild<QGroupBox*>("SD Root")) {
        sdRootGroup->setTitle(tr("SD Root"));
    }
    if (auto toolsGroup = findChild<QGroupBox*>("Tools")) {
        toolsGroup->setTitle(tr("Tools"));
    }
    if (auto progressGroup = findChild<QGroupBox*>("Progress")) {
        progressGroup->setTitle(tr("Progress"));
    }
    
    // Update checkboxes
    mergeBinFilesCheck->setText(tr("Merge Bin Files"));
    cu2ForAllCheck->setText(tr("CU2 For All"));
    fixInvalidNameCheck->setText(tr("Fix Invalid Name"));
    autoRenameCheck->setText(tr("Auto Rename"));
    addCoverArtCheck->setText(tr("Add Cover Art"));
    createMultiDiscCheck->setText(tr("Create Multi-Disc"));
    
    // Update status
    if (games.empty()) {
        statusLabel->setText(tr("Status: Ready"));
    } else {
        statusLabel->setText(tr("Games found: %1").arg(games.size()));
    }
} 