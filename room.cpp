#include "room.h"
#include "tcpsocket.h"
#include "rooms.h"
#include <QTimer>

Room::Room(QString name, QString password, TcpSocket *socket, QSqlDatabase* db, Rooms *p)
    : Name(name), Password(password), dataBase(db), parent(p)
{
    Clients.append(socket);
}

void Room::GetRoom(TcpSocket *socket)
{
    Clients.append(socket);
}

void Room::SendAudioToAllClients(TcpSocket *SentAudio, QByteArray buffer)
{
    foreach (TcpSocket* sock, Clients)
    {
        if(sock != SentAudio)
        {
            QMetaObject::invokeMethod(sock, "onWrite", Qt::AutoConnection,
                                      Q_ARG(QByteArray, buffer));
        }
    }
}

//void Room::SendMessageToAllClients(TcpSocket *SentAudio, QString str)
//{
//    foreach (TcpSocket* sock, Clients)
//    {
//        if(sock != SentAudio)
//        {
//            QMetaObject::invokeMethod(sock, "onWrite", Qt::AutoConnection,
//                                      Q_ARG(QByteArray, "/message/" + str.toUtf8()));
//        }
//    }
//}

int Room::countSocket()
{
    return Clients.size();
}

void Room::closeRoom(TcpSocket *SentAudio)
{
    foreach (TcpSocket* sock, Clients)
    {
        if(sock != SentAudio)
        {
            QString message = "/outoftheroom/";
            QMetaObject::invokeMethod(sock, "onWrite", Qt::AutoConnection,
                                      Q_ARG(QByteArray, message.toUtf8()));
            leaveTheRoom(sock);
        }
    }
    parent->removeRoom(this);
    QTimer* time = new QTimer;
    time->singleShot(10000, [=](){
       deleteLater();
    });
}

void Room::leaveTheRoom(TcpSocket *client)
{
    Clients.removeOne(client);
}

QString Room::getName() const
{
    return Name;
}

QString Room::getPassword() const
{
    return Password;
}
