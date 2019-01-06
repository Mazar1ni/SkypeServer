#include "rooms.h"
#include "room.h"
#include "tcpsocket.h"

Rooms::Rooms()
{

}

Room* Rooms::CreateRoom(QString name, QString pass, TcpSocket *socket, QSqlDatabase* db, QString ip, QString port)
{
    rooms.append(new Room(name, pass, socket, db, this, ip, port));
    return rooms.at(rooms.length() - 1);
}

Room* Rooms::GetInRoom(QString name, QString pass, TcpSocket *socket, QString ip, QString port)
{
    for(int i = 0; i < rooms.count(); i++)
    {
        if(rooms[i]->getName() == name && rooms[i]->getPassword() == pass)
        {
            rooms[i]->GetRoom(socket, ip, port);
            return rooms[i];
        }
    }

    return nullptr;
}

void Rooms::closeRoomFriendHangUp(QString name, TcpSocket *SentAudio)
{
    for(int i = 0; i < rooms.count(); i++)
    {
        if(rooms[i]->getName() == name && rooms[i]->countSocket() == 1)
        {
            rooms.at(i)->closeRoom(SentAudio);
            rooms.removeAt(i);
        }
    }
}

void Rooms::removeRoom(Room* room)
{
    rooms.removeOne(room);
}
