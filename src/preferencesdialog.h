#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
    ~PreferencesDialog() override = default;

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void selectBackupDir();

    QCheckBox* autoFixCueBox;
    QCheckBox* createBackupsBox;
    QLineEdit* backupDirEdit;
    QPushButton* backupDirButton;
};

#endif // PREFERENCESDIALOG_H