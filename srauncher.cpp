#include "srauncher.h"
#include "ui_srauncher.h"

SRauncher::SRauncher(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SRauncher)
{
    ui->setupUi(this);
    this->setFixedSize(450, 380);
    rx.setPattern(R"((.*):(\d{1,5}))");
    ui->btnRemove->setEnabled(false);
    ui->btnRename->setEnabled(false);
    manager = new QNetworkAccessManager(this);
    regset = new QSettings("HKEY_CURRENT_USER\\SOFTWARE\\SAMP",
                           QSettings::NativeFormat);
    ui->edtNick->setText(regset->value("PlayerName").toString());
    servers = new CSampServers(ui->edtNick->text(), ui->srvList);
    game = new CRunGame();
    inject = new SelectLibs(this);
    sets = new CSettings(servers, this);
    udp = nullptr;
}

SRauncher::~SRauncher()
{
    QList<QListWidgetItem *> lst = ui->srvList->selectedItems();
    g_SrvList[lst.front()->text()].gta_sa = ui->edtGta->text();
    g_SrvList[lst.front()->text()].samp = ui->edtSamp->text();
    g_SrvList[lst.front()->text()].nick = ui->edtNick->text();
    g_SrvList[lst.front()->text()].comment = ui->edtComment->toPlainText();
    delete servers;
    delete ui;
}

void SRauncher::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void SRauncher::AddToSrvList()
{
    QNetworkReply *reply=
      qobject_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError)
    {
      // Получаем содержимое ответа
      QByteArray content= reply->readAll();

      // Реализуем преобразование кодировки
      // (зависит от кодировки сайта)
      QTextCodec *codec = QTextCodec::codecForName("cp1251");

      // Выводим результат
      stServer srv;
      srv.name = codec->toUnicode(content.data());
      bool _next = true;
      foreach (auto it, g_SrvList)
          if (it.name == srv.name)
              _next = false;
      if (_next){

          if (rx.indexIn(ui->edtIp->text()) != -1){
              srv.ip = rx.cap(1);
              srv.port = rx.cap(2).toShort();
          } else {
              srv.ip = ui->edtIp->text();
              srv.port = 7777;
          }
          srv.gta_sa = "gta_sa.exe";
          srv.samp = "samp.dll";
          srv.nick = regset->value("PlayerName").toString();
          ui->srvList->addItem(codec->toUnicode(content.data()));
          g_SrvList[srv.name] = srv;
      }
      ui->edtIp->setText("");
      ui->edtIp->setEnabled(true);
      ui->btnAddSrv->setEnabled(true);
    }

    // разрешаем объекту-ответа "удалится"
    reply->deleteLater();
}

void SRauncher::UpdateSrvInfo()
{
    if (!srvNeedUpd)
        return;
    srvNeedUpd = false;

    QNetworkReply *reply=
      qobject_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError)
    {
      QByteArray content= reply->readAll();
      QTextCodec *codec = QTextCodec::codecForName("cp1251");

      ui->srvInfo->setText(codec->toUnicode(content.data()));
    }

    reply->deleteLater();
}

void SRauncher::on_btnAddSrv_clicked()
{
    if (rx.indexIn(ui->edtIp->text()) != -1){
        ui->edtIp->setEnabled(false);
        ui->btnAddSrv->setEnabled(false);
        // берем адрес введенный в текстовое поле
        QUrl url("http://samp.prime-hack.net/?ip=" + rx.cap(1) +
                 "&port=" + rx.cap(2) + "&info=name");

        // создаем объект для запроса
        QNetworkRequest request(url);

        // Выполняем запрос, получаем указатель на объект
        // ответственный за ответ
        QNetworkReply* reply=  manager->get(request);

        // Подписываемся на сигнал о готовности загрузки
        connect( reply, SIGNAL(finished()),
                 this, SLOT(AddToSrvList()) );
    } else {
        QMessageBox msgBox;
        msgBox.setText("Bad IP:Port");
        msgBox.exec();
    }
}

void SRauncher::on_srvList_itemClicked(QListWidgetItem *item)
{
    ui->btnRemove->setEnabled(true);
    ui->btnRename->setEnabled(true);

    stServer srv = g_SrvList[item->text()];
    if (udp != nullptr)
        delete udp;
    udp = new CUdpConnect(srv.ip, srv.port, this);
    for(int i = 0; i < 3; ++i){
        udp->requestInfo();
        udp->requestRule();
    }
    QUrl url("http://samp.prime-hack.net/?ip=" + srv.ip +
             "&port=" + QString::number(srv.port) + "&info=bingui");

    ui->edtIp->setText(srv.ip + ":" + QString::number(srv.port));

    QNetworkRequest request(url);
    QNetworkReply* reply=  manager->get(request);
    connect( reply, SIGNAL(finished()),
             this, SLOT(UpdateSrvInfo()) );
    ui->srvInfo->setText("Please wait, information is updating...");
    srvNeedUpd = true;
}

void SRauncher::on_btnConnect_clicked()
{
    game->setGta(ui->edtGta->text());
    game->addLib(ui->edtSamp->text());

    if (regset->value("asi_loader").toBool()){
        QDir dir;
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        dir.setSorting(QDir::Name);
        QFileInfoList list = dir.entryInfoList();
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
            if (fileInfo.suffix().toLower() == "asi")
                game->addLib(fileInfo.fileName());

        }
    }
    game->setWindowMode(regset->value("win_mode").toBool());

    QList<QString> addLibs = inject->enabledLibs();
    foreach (auto lib, addLibs) {
        game->addLib(lib);
    }

    if (rx.indexIn(ui->edtIp->text()) != -1){
        game->Connect(ui->edtNick->text(),
                      rx.cap(1),
                      rx.cap(2).toUInt());
    } else {
        QMessageBox msgBox;
        msgBox.setText("Bad IP:Port");
        msgBox.exec();
    }
}

void SRauncher::on_btnRename_clicked()
{
    QList<QListWidgetItem *> lst = ui->srvList->selectedItems();
    ServerRename* dlg = new ServerRename(lst.front(), this);
    dlg->show();
}

void SRauncher::on_btnRemove_clicked()
{
    QList<QListWidgetItem *> lst = ui->srvList->selectedItems();
    g_SrvList.remove(lst.front()->text());
    ui->srvList->removeItemWidget(lst.front());
    delete lst.front();
}

void SRauncher::on_srvList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (previous != nullptr){
        g_SrvList[previous->text()].gta_sa = ui->edtGta->text();
        g_SrvList[previous->text()].samp = ui->edtSamp->text();
        g_SrvList[previous->text()].nick = ui->edtNick->text();
        g_SrvList[previous->text()].comment = ui->edtComment->toPlainText();
    }
    ui->edtGta->setText(g_SrvList[current->text()].gta_sa);
    ui->edtSamp->setText(g_SrvList[current->text()].samp);
    ui->edtNick->setText(g_SrvList[current->text()].nick);
    ui->edtComment->setText(g_SrvList[current->text()].comment);
}

void SRauncher::on_btnInject_clicked()
{
    inject->show();
}

void SRauncher::on_btnSettings_clicked()
{
    sets->show();
}
