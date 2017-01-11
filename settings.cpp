#include "settings.h"
#include "ui_settings.h"

CSettings::CSettings(CSampServers *servers, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CSettings)
{
    ui->setupUi(this);
    this->setFixedSize(162, 82);
    this->servers = servers;
    regset = new QSettings("HKEY_CURRENT_USER\\SOFTWARE\\SAMP",
                           QSettings::NativeFormat);
    ui->cbAsiLoader->setChecked(regset->value("asi_loader").toBool());
    ui->cbWinMode->setChecked(regset->value("win_mode").toBool());
}

CSettings::~CSettings()
{
    delete ui;
}

void CSettings::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void CSettings::on_cbAsiLoader_toggled(bool checked)
{
    regset->setValue("asi_loader", checked);
}

void CSettings::on_cbWinMode_toggled(bool checked)
{
    regset->setValue("win_mode", checked);
}

void CSettings::on_btnImport_clicked()
{
    servers->Import();
    QMessageBox msgBox;
    msgBox.setText("Servers has imported.");
    msgBox.exec();
}