#ifndef ROOMS_H
#define ROOMS_H

#include <QObject>

class TcpSocket;

class Room;
class SocketThread;
class QSqlDatabase;

class Rooms
{
public:
    Rooms();

    Room *CreateRoom(QString name, QString pass, TcpSocket *socket, QSqlDatabase *db, QString ip, QString port);
    Room *GetInRoom(QString name, QString pass, TcpSocket *socket, QString ip, QString port);
    void closeRoomFriendHangUp(QString name, TcpSocket *SentAudio);
    void removeRoom(Room *room);

private:
    QList<Room*> rooms;
};

#endif // ROOMS_H
