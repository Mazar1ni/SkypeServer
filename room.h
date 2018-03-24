#ifndef ROOM_H
#define ROOM_H

#include <QObject>
#include <QTcpSocket>
#include <QSqlDatabase>

class TcpSocket;
class Rooms;

class Room : public QObject
{
    Q_OBJECT
public:
    Room(QString name, QString password, TcpSocket* socket, QSqlDatabase *db, Rooms* p);

    void GetRoom(TcpSocket *socket);
    //void SendMessageToAllClients(TcpSocket *SentAudio, QString str);
    int countSocket();
    void leaveTheRoom(TcpSocket *client);

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

};

#endif // ROOM_H
