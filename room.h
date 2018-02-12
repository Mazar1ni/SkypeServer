#ifndef ROOM_H
#define ROOM_H

#include <QObject>
#include <QTcpSocket>
#include <QSqlDatabase>

class TcpSocket;
class Rooms;

class Room : public QObject
{
public:
    Room(QString name, QString password, TcpSocket* socket, QSqlDatabase *db, Rooms* p);

    void GetRoom(TcpSocket *socket);
    void SendAudioToAllClients(TcpSocket *SentAudio, QByteArray buffer);
    //void SendMessageToAllClients(TcpSocket *SentAudio, QString str);
    int countSocket();
    void closeRoom(TcpSocket *SentAudio);
    void leaveTheRoom(TcpSocket *client);

    QString getName() const;
    QString getPassword() const;

private:
    QString Name;
    QString Password;
    QList<TcpSocket*> Clients;
    QSqlDatabase *dataBase;
    Rooms* parent;

};

#endif // ROOM_H
