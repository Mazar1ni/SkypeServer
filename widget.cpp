#include "widget.h"
#include "socketthread.h"
#include "rooms.h"
#include "filetransfer.h"
#include "updater.h"

using namespace std;

Widget::Widget(QObject* parent) : QObject(parent)
{
    // подключение к бд
    DataBase = QSqlDatabase::addDatabase("QMYSQL");
    DataBase.setHostName("localhost");
    DataBase.setDatabaseName("skype");
    DataBase.setUserName("root");
    DataBase.setPassword("");

    DataBase.open();

    socketClients = new QList<SocketThread*>;

    rooms = new Rooms;

    systemServer = new QTcpServer();
    systemServer->listen(QHostAddress::Any, 7070);
    connect(systemServer, SIGNAL(newConnection()), this, SLOT(newConnection()));

    fileTransferServer = new QTcpServer();
    fileTransferServer->listen(QHostAddress::Any, 7071);
    connect(fileTransferServer, SIGNAL(newConnection()), this, SLOT(newConnectionTransferFile()));

    updaterServer = new QTcpServer();
    updaterServer->listen(QHostAddress::Any, 7072);
    connect(updaterServer, SIGNAL(newConnection()), this, SLOT(newConnectionUpdater()));
}

Widget::~Widget()
{
    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("UPDATE users SET status = '0'");
    query.exec();
}

void Widget::newConnection()
{
    SocketThread* systemSocket = new SocketThread(systemServer->nextPendingConnection()->socketDescriptor(),
                                            rooms, DataBase, socketClients);
    systemSocket->start();
    socketClients->append(systemSocket);
}

void Widget::newConnectionTransferFile()
{
    FileTransfer* fileTransferSocket = new FileTransfer(fileTransferServer->nextPendingConnection()->socketDescriptor(),
                                                        DataBase);
    fileTransferSocket->start();
}

void Widget::newConnectionUpdater()
{
    Updater* updaterSocket = new Updater(updaterServer->nextPendingConnection()->socketDescriptor());
    updaterSocket->start();
}
