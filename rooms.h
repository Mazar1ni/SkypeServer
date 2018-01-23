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

    Room *CreateRoom(QString name, QString pass, TcpSocket *socket, QSqlDatabase *db);
    Room *GetInRoom(QString name, QString pass, TcpSocket *socket);
    void closeRoomFriendHangUp(QString name);

private:
    QList<Room*> rooms;
};

#endif // ROOMS_H
