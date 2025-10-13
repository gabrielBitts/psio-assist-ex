#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include <QDir>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("PSIO Assist EX");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("PSIO Assist");
    
    // Setup translation system
    QTranslator translator;
    QTranslator qtTranslator;
    
    // Get the language from settings or use system default
    QString language = "pt_BR"; // Default to Portuguese
    
    // Try to load application translation
    QString translationPath = QCoreApplication::applicationDirPath() + "/translations/";
    if (translator.load("psio-assist_" + language, translationPath)) {
        app.installTranslator(&translator);
    }
    
    // Try to load Qt translation
    if (qtTranslator.load("qt_" + language, QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&qtTranslator);
    }
    
    MainWindow window;
    window.show();
    return app.exec();
}