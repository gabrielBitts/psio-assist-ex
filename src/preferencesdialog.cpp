#include "preferencesdialog.h"
#include "config.h" // Make sure to include the Config class header
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

PreferencesDialog::PreferencesDialog(QWidget *parent) 
    : QDialog(parent)
    , autoFixCueBox(nullptr)
    , createBackupsBox(nullptr)
    , backupDirEdit(nullptr)
    , backupDirButton(nullptr)
{
    setWindowTitle(tr("Preferences"));
    setupUI();
    loadSettings();
}

void PreferencesDialog::saveSettings() {
    Config::getInstance().autoFixCue = autoFixCueBox->isChecked();
    Config::getInstance().createBackups = createBackupsBox->isChecked();
    Config::getInstance().backupDirectory = backupDirEdit->text();
    Config::getInstance().save();
    accept();
}

void PreferencesDialog::selectBackupDir() {
    QString dir = QFileDialog::getExistingDirectory(this, 
        tr("Select Backup Directory"),
        backupDirEdit->text());
    if (!dir.isEmpty()) {
        backupDirEdit->setText(dir);
    }
}

void PreferencesDialog::setupUI() {
    auto *layout = new QVBoxLayout(this);
    auto *formLayout = new QFormLayout();

    // Auto-fix options
    autoFixCueBox = new QCheckBox(tr("Automatically fix CUE files"), this);
    formLayout->addRow(autoFixCueBox);

    // Backup options
    createBackupsBox = new QCheckBox(tr("Create backups before modifying files"), this);
    formLayout->addRow(createBackupsBox);

    // Backup directory
    auto *backupDirLayout = new QHBoxLayout();
    backupDirEdit = new QLineEdit(this);
    backupDirButton = new QPushButton(tr("Select"), this);
    backupDirLayout->addWidget(backupDirEdit);
    backupDirLayout->addWidget(backupDirButton);
    formLayout->addRow(tr("Backup Directory:"), backupDirLayout);

    layout->addLayout(formLayout);

    // Buttons
    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::saveSettings);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(backupDirButton, &QPushButton::clicked, this, &PreferencesDialog::selectBackupDir);
}

void PreferencesDialog::loadSettings() {
    const auto &config = Config::getInstance();
    autoFixCueBox->setChecked(config.autoFixCue);
    createBackupsBox->setChecked(config.createBackups);
    backupDirEdit->setText(config.backupDirectory);
}