#ifndef ROOM_H
#define ROOM_H

#include <QObject>
#include <QTcpSocket>
#include <QSqlDatabase>

class TcpSocket;

class Room
{
public:
    Room(QString name, QString password, TcpSocket* socket, QSqlDatabase *db);

    void GetRoom(TcpSocket *socket);
    void SendAudioToAllClients(TcpSocket *SentAudio, QByteArray buffer);
    void SendMessageToAllClients(TcpSocket *SentAudio, QString str);
    int countSocket();
    void closeRoom();

    QString getName() const;
    QString getPassword() const;

private:
    QString Name;
    QString Password;
    QList<TcpSocket*> Clients;
    QSqlDatabase *dataBase;

};

#endif // ROOM_H
