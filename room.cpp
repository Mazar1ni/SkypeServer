#include "room.h"
#include "tcpsocket.h"
#include "rooms.h"
#include <QTimer>
#include <QDataStream>

Room::Room(QString name, QString password, TcpSocket *socket, QSqlDatabase* db, Rooms *p, QString ip, QString port)
    : Name(name), Password(password), dataBase(db), parent(p)
{
    clientsData.append(Client{ip, port});
    Clients.append(socket);
}

void Room::GetRoom(TcpSocket *socket, QString ip, QString port)
{
    sendIpPortAllClients(socket, ip, port);

    clientsData.append(Client{ip, port});
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
//            QString message = "/outoftheroom/";
//            QMetaObject::invokeMethod(sock, "onWrite", Qt::AutoConnection,
//                                      Q_ARG(QByteArray, message.toUtf8()));
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

void Room::sendIpPortAllClients(TcpSocket *sender, QString ip, QString port)
{
    foreach (TcpSocket* sock, Clients)
    {
        QByteArray  arrBlock;

        QDataStream out(&arrBlock, QIODevice::WriteOnly);
        out << "/friendIpPort/" + ip + "!" + port;

        QMetaObject::invokeMethod(sock, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, arrBlock));

    }

    foreach (Client client, clientsData)
    {
        QByteArray  arrBlock;

        QDataStream out(&arrBlock, QIODevice::WriteOnly);
        out << "/friendIpPort/" + client.ip + "!" + client.port;

        QMetaObject::invokeMethod(sender, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, arrBlock));
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
