#include "widget.h"
#include "socketthread.h"
#include "rooms.h"

using namespace std;

Widget::Widget()
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
}

Widget::~Widget()
{

}

void Widget::newConnection()
{
    SocketThread* systemSocket = new SocketThread(systemServer->nextPendingConnection()->socketDescriptor(),
                                            rooms, DataBase, socketClients);
    systemSocket->start();
    socketClients->append(systemSocket);
}
