#ifndef ROOM_H
#define ROOM_H

#include <QObject>
#include <QTcpSocket>
#include <QSqlDatabase>

class TcpSocket;
class Rooms;

struct Client
{
    QString ip;
    QString port;
};

class Room : public QObject
{
    Q_OBJECT
public:
    Room(QString name, QString password, TcpSocket* socket, QSqlDatabase *db, Rooms* p, QString ip, QString port);

    void GetRoom(TcpSocket *socket, QString ip, QString port);
    //void SendMessageToAllClients(TcpSocket *SentAudio, QString str);
    int countSocket();
    void leaveTheRoom(TcpSocket *client);
    void sendIpPortAllClients(TcpSocket* sender, QString ip, QString port);

    QString getName() const;
    QString getPassword() const;

public slots:
    void SendAudioToAllClients(TcpSocket *SentAudio, QByteArray buffer);
    void closeRoom(TcpSocket *SentAudio);

private:
    QString Name;
    QString Password;
    QList<TcpSocket*> Clients;
    QSqlDatabase *dataBase;
    Rooms* parent;

    QList<Client> clientsData;

};

#endif // ROOM_H
