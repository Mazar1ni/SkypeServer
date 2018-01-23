#include "rooms.h"
#include "room.h"
#include "tcpsocket.h"

Rooms::Rooms()
{

}

Room* Rooms::CreateRoom(QString name, QString pass, TcpSocket *socket, QSqlDatabase* db)
{
    rooms.append(new Room(name, pass, socket, db));
    return rooms.at(rooms.length() - 1);
}

Room* Rooms::GetInRoom(QString name, QString pass, TcpSocket *socket)
{
    for(int i = 0; i < rooms.count(); i++)
    {
        if(rooms[i]->getName() == name && rooms[i]->getPassword() == pass)
        {
            rooms[i]->GetRoom(socket);
            return rooms[i];
        }
    }

    return nullptr;
}

void Rooms::closeRoomFriendHangUp(QString name)
{
    for(int i = 0; i < rooms.count(); i++)
    {
        if(rooms[i]->getName() == name && rooms[i]->countSocket() == 1)
        {
            rooms.removeAt(i);
        }
    }
}
