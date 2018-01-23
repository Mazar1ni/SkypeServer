#include "room.h"
#include "tcpsocket.h"

Room::Room(QString name, QString password, TcpSocket *socket, QSqlDatabase* db)
    : Name(name), Password(password), dataBase(db)
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
//        if(sock != SentAudio)
//        {
            QMetaObject::invokeMethod(sock, "onWrite", Qt::AutoConnection,
                                      Q_ARG(QByteArray, buffer));
//        }
    }
}

void Room::SendMessageToAllClients(TcpSocket *SentAudio, QString str)
{
    foreach (TcpSocket* sock, Clients)
    {
//        if(sock != SentAudio)
//        {
            QMetaObject::invokeMethod(sock, "onWrite", Qt::AutoConnection,
                                      Q_ARG(QByteArray, "/message/" + str.toUtf8()));
//        }
    }
}

int Room::countSocket()
{
    return Clients.size();
}

void Room::closeRoom()
{
    foreach (TcpSocket* sock, Clients)
    {
        QString message = "/outoftheroom/";
        QMetaObject::invokeMethod(sock, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, message.toUtf8()));
    }
}

QString Room::getName() const
{
    return Name;
}

QString Room::getPassword() const
{
    return Password;
}
