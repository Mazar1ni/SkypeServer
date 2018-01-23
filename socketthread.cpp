#include "socketthread.h"
#include <QDataStream>
#include "rooms.h"
#include "room.h"
#include "tcpsocket.h"
#include <QFile>
#include "sox.h"
#include <QDate>

using namespace std;

SocketThread::SocketThread(int descriptor, Rooms* rooms, QSqlDatabase db, QList<SocketThread *> *Clients, QObject * parent) :
    QThread(parent), SocketDescriptor(descriptor), AllRooms(rooms), DataBase(db), socketClients(Clients)
{

}

SocketThread::~SocketThread()
{
    Socket->abort();
    delete Socket;
}

void SocketThread::run()
{
    Socket = new TcpSocket;
    Socket->setSocketDescriptor(SocketDescriptor);

    myProcess = new Sox(this);
    myProcess->moveToThread(&thread);
    thread.start();

    connect(this, SIGNAL(noiseRemove()), myProcess, SLOT(removeNoise()));
    connect(myProcess, SIGNAL(sendSound()), this, SLOT(sendSound()));

    connect(Socket, SIGNAL(readyRead()), this, SLOT(OnReadyRead()), Qt::DirectConnection);
    connect(Socket, SIGNAL(disconnected()), this, SLOT(OnDisconnected()), Qt::DirectConnection);

    exec();

}

void SocketThread::Authentication(QString login, QString pass)
{
    // запрос на поиск логина в бд
    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT id, Login, Email, Phone, FIO, Password, Status FROM users WHERE Login = :login");
    query.bindValue(":login", login);
    query.exec();

    // получение пароля из строки
    QString password;
    QString email;
    QString FIO;
    QString phone;
    QString status;
    while(query.next())
    {
        password = query.value("Password").toString();
        id = query.value("id").toString();
        email = query.value("Email").toString();
        FIO = query.value("FIO").toString();
        phone = query.value("Phone").toString();
        status = query.value("Status").toString();
    }

    // проверка, найден ли такой пользователь
    if(!password.isEmpty())
    {
        // проверка, совпадают ли пароли
        if(password == pass)
        {
            if(status == "1")
            {
                SlotSendToClient("/-1/");
            }
            else
            {
                name = login;
                SlotSendToClient("/1/" + id + " " + name + " " + email + " " + FIO + " " + phone);

                query.prepare("UPDATE users SET status = '1' WHERE id = :id");
                query.bindValue(":id", id);
                query.exec();

                query.prepare("SELECT idFirstFriends, idSecoundFriends FROM friends WHERE idFirstFriends = :first "
                              "OR idSecoundFriends = :secound");
                query.bindValue(":first", id);
                query.bindValue(":secound", id);
                query.exec();

                while(query.next())
                {
                    for(int i = 0; i < socketClients->size(); i++)
                    {
                        if(socketClients->at(i) != this)
                        {
                            if(query.value("idFirstFriends").toString() != id)
                            {
                                QMetaObject::invokeMethod(socketClients->at(i), "checkFriendsUpdateOnline",
                                                          Qt::AutoConnection,
                                                          Q_ARG(QString, query.value("idFirstFriends").toString()),
                                                          Q_ARG(QString, id),
                                                          Q_ARG(QString, "online"));
                            }
                            else
                            {
                                QMetaObject::invokeMethod(socketClients->at(i), "checkFriendsUpdateOnline",
                                                          Qt::AutoConnection,
                                                          Q_ARG(QString, query.value("idSecoundFriends").toString()),
                                                          Q_ARG(QString, id),
                                                          Q_ARG(QString, "online"));
                            }

                        }
                    }
                }
            }
        }
        else
        {
            SlotSendToClient("/0/");
        }
    }
    else
    {
        SlotSendToClient("/0/");
    }

    query.clear();
}

void SocketThread::CreateRooms(QString name, QString pass)
{
    room = AllRooms->CreateRoom(name, pass, Socket, &DataBase);
    if(room != nullptr)
    {
        qDebug("CreateRoom");
    }
}

void SocketThread::GetInRoom(QString name, QString pass)
{
    // проверка, добавился ли клент в указанную комноту
    room = AllRooms->GetInRoom(name, pass, Socket);
    if(room != nullptr)
    {
        qDebug("getinroom");
        SlotSendToClient("/9/");
    }
}

//float lopass(float input, float cutoff) {
// lo_pass_output= outputs[0]+ (cutoff*(input-outputs[0]));
//outputs[0]= lo_pass_output;
//return(lo_pass_output);
//}

//float hipass(float input, float cutoff) {
// hi_pass_output=input-(outputs[0] + cutoff*(input-outputs[0]));
// outputs[0]=hi_pass_output;
// return(hi_pass_output);
//}

void SocketThread::writingToFile(QByteArray buffer)
{
    QFile destinationFile;
    destinationFile.setFileName(".//sox/files/test.raw");
    destinationFile.open( QIODevice::WriteOnly);

    destinationFile.write(buffer);
    destinationFile.close();
//    float input[buffer.size()];
//    for(int i = 0; i < buffer.size(); i++)
//    {
//        input[i] = buffer[i];
//    }

//    QString n = "/18/";
//    QByteArray buff = n.toUtf8();
//    QByteArray bufff;
//    for(int i = 0; i < buffer.size(); i++)
//    {
//        bufff[i] = input[i];
//    }
//    buff += bufff;
//    room->SendAudioToAllClients(Socket, buff);

    removeNoise();
}

void SocketThread::removeNoise()
{
    emit(noiseRemove());
    //QMetaObject::invokeMethod(myProcess, "removeNoise", Qt::AutoConnection);
}

void SocketThread::sendSound()
{
    QFile destinationFile;
    destinationFile.setFileName(".//sox/files/clear.raw");
    destinationFile.open( QIODevice::ReadOnly);

    QString n = "/18/";
    QByteArray buff = n.toUtf8();

    buff += destinationFile.readAll();

    room->SendAudioToAllClients(Socket, buff);

    destinationFile.close();
}

void SocketThread::checkId(QString testId, QString nameRoom, QString passRoom)
{
    if(id == testId)
    {
        QString mess = "/29/" + nameRoom + " " + passRoom;
        QMetaObject::invokeMethod(Socket, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, mess.toUtf8()));

    }
}

void SocketThread::checkIdForSendMessage(QString testId, QString message, QString idSender)
{
    if(id == testId)
    {
        QString mess = "/newMessage/" + idSender  + "!" + message;
        QMetaObject::invokeMethod(Socket, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, mess.toUtf8()));
    }
}

void SocketThread::checkFriendsUpdateOnline(QString testId, QString idFriend, QString status)
{
    if(id == testId)
    {
        QString mess = "/33/" + idFriend + ":" + status;
        QMetaObject::invokeMethod(Socket, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, mess.toLocal8Bit()));
    }
}

void SocketThread::OnReadyRead()
{
    QByteArray buffer;
    buffer = Socket->readAll();

    QDataStream in(buffer);

    QString str;
    in >> str;

    // отправка всем пользователям комнаты аудио
    if(buffer.indexOf("/18/") != -1)
    {
        //buffer.remove(0, 4);
        room->SendAudioToAllClients(Socket, buffer);
        //writingToFile(buffer);
    }

    // проверка, относится ли этот текст в авторизации
    if(str.indexOf("/1/") != -1)
    {
        // удаление идентификатора
        str.remove(str.length() - 3, 3);

        // разделение логина и пароля
        int pos = str.indexOf(":");
        Authentication(str.left(pos), str.mid(pos+1));
    }

    // проверка, относитмя ли этот текст к созданию комнаты
    if(str.indexOf("/2/") != -1)
    {
        // удаление идентификатора
        str.remove(str.length() - 3, 3);

        // разделение имя комнаты и пароля комнаты
        int pos = str.indexOf(":");
        CreateRooms(str.left(pos), str.mid(pos+1));
    }

    // проверка, относитмя ли этот текст ко входу в комнату
    if(str.indexOf("/3/") != -1)
    {
        // удаление идентификатора
        str.remove(str.length() - 3, 3);

        // разделение имя комнаты и пароля комнаты
        int pos = str.indexOf(":");
        GetInRoom(str.left(pos), str.mid(pos+1));
    }

    // проверка, относится ли этот текст к получению друзей клиентом
    if(str.indexOf("/4/") != -1)
    {
        // удаление идентификатора
        str.remove(str.length() - 3, 3);

        gettingFriends();
    }

    // проверка, относится ли этот текст к начал звонка другу
    if(str.indexOf("/19/") != -1)
    {
        // удаление идентификатора
        str.remove(0, 4);

        for(int i = 0; i < socketClients->size(); i++)
        {
            if(socketClients->at(i) != this)
            {
                CreateRooms(name + "-" + str, "123");
                QMetaObject::invokeMethod(socketClients->at(i), "checkId", Qt::AutoConnection,
                                          Q_ARG(QString, str),
                                          Q_ARG(QString, name + "-" + str),
                                          Q_ARG(QString, "123"));
            }
        }
    }

    // проверка, относится ли этот текст к взятию трубки другом
    if(str.indexOf("/30/") != -1)
    {
        // удаление идентификатора
        str.remove(0, 4);

        // разделение имя комнаты и пароля комнаты
        int pos = str.indexOf(":");
        GetInRoom(str.left(pos), str.mid(pos+1));
    }

    // проверка, относится ли этот текст к сбросу звонка другом
    if(str.indexOf("/31/") != -1)
    {
        // удаление идентификатора
        str.remove(0, 4);

        // разделение имя комнаты и пароля комнаты
        int pos = str.indexOf(":");
        closeRoomFriendHangUp(str.left(pos));
    }

    // проверка, относится ли этот текст к получению истории сообщений
    if(str.indexOf("/HistoryMessage/") != -1)
    {
        // удаление идентификатора
        str.remove(0, 16);

        sendHistoryMessage(str);
    }

    // проверка, относится ли этот текст к отправке сообщения другу
    if(str.indexOf("/message/") != -1)
    {
        // удаление идентификатора
        str.remove(0, 9);

        int pos = str.indexOf("!");

        QDate date = QDate::currentDate();
        QTime time = QTime::currentTime();

        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("INSERT INTO messages (idFirstFriends, idSecoundFriends, message, time) "
                      "VALUES (:idFirstFriends, :idSecoundFriends, :message, :time)");
        query.bindValue(":idFirstFriends", id);
        query.bindValue(":idSecoundFriends", str.left(pos));
        query.bindValue(":message", str.mid(pos+1));
        query.bindValue(":time", date.toString("dddd MMMM dd yyyy") + " " + time.toString("hh:mm"));
        query.exec();

//        query.prepare("SELECT idFirstFriends, idSecoundFriends FROM friends WHERE idFirstFriends = :first "
//                      "OR idSecoundFriends = :secound");
//        query.bindValue(":first", id);
//        query.bindValue(":secound", id);
//        query.exec();

//        while(query.next())
//        {
            for(int i = 0; i < socketClients->size(); i++)
            {
                if(socketClients->at(i) != this)
                {
                    QMetaObject::invokeMethod(socketClients->at(i), "checkIdForSendMessage", Qt::AutoConnection,
                                          Q_ARG(QString, str.left(pos)),
                                          Q_ARG(QString, date.toString("dddd MMMM dd yyyy")
                                                + " " + time.toString("hh:mm") + "!" + str),
                                          Q_ARG(QString, id));
                }
            }
            checkIdForSendMessage(id, date.toString("dddd MMMM dd yyyy")
                             + " " + time.toString("hh:mm") + "!" + str, id);
//        }
    }

    if(str.indexOf("/readAllMessages/") != -1)
    {
        str.remove(0, 17);

        // P.S. str - это id друга
        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("SELECT idFirstFriends, idSecoundFriends, "
                      "unreadMessageFirstFriend, unreadMessageSecoundFriend "
                      "FROM friends "
                      "WHERE (idFirstFriends = :firstFriend "
                      "AND idSecoundFriends = :first) "
                      "OR (idFirstFriends = :first "
                      "AND idSecoundFriends = :firstFriend)");
        query.bindValue(":firstFriend", str);
        query.bindValue(":first", id);
        query.exec();

        int firstUnread;
        int secoundUnread;

        query.next();

        firstUnread = query.value(2).toInt();
        secoundUnread = query.value(3).toInt();

        query.value(0).toString() == id ? firstUnread = 0 : secoundUnread = 0;

        query.prepare("UPDATE friends SET unreadMessageFirstFriend = :firstUnread, "
                      "unreadMessageSecoundFriend = :secoundUnread "
                      "WHERE (idFirstFriends = :firstFriend "
                      "AND idSecoundFriends = :first) "
                      "OR (idFirstFriends = :first "
                      "AND idSecoundFriends = :firstFriend)");
        query.bindValue(":firstUnread", firstUnread);
        query.bindValue(":secoundUnread", secoundUnread);
        query.bindValue(":firstFriend", str);
        query.bindValue(":first", id);
        query.exec();
    }

    if(str.indexOf("/UnreadMessage/") != -1)
    {
        str.remove(0, 15);

        // P.S. str - это id друга
        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("SELECT idFirstFriends, idSecoundFriends, "
                      "unreadMessageFirstFriend, unreadMessageSecoundFriend "
                      "FROM friends "
                      "WHERE (idFirstFriends = :firstFriend "
                      "AND idSecoundFriends = :first) "
                      "OR (idFirstFriends = :first "
                      "AND idSecoundFriends = :firstFriend)");
        query.bindValue(":firstFriend", str);
        query.bindValue(":first", id);
        query.exec();

        int firstUnread;
        int secoundUnread;

        query.next();

        firstUnread = query.value(2).toInt();
        secoundUnread = query.value(3).toInt();

        query.value(0).toString() == str ? firstUnread++ : secoundUnread++;

        query.prepare("UPDATE friends SET unreadMessageFirstFriend = :firstUnread, "
                      "unreadMessageSecoundFriend = :secoundUnread "
                      "WHERE (idFirstFriends = :firstFriend "
                      "AND idSecoundFriends = :first) "
                      "OR (idFirstFriends = :first "
                      "AND idSecoundFriends = :firstFriend)");
        query.bindValue(":firstUnread", firstUnread);
        query.bindValue(":secoundUnread", secoundUnread);
        query.bindValue(":firstFriend", str);
        query.bindValue(":first", id);
        query.exec();
    }
}

void SocketThread::OnDisconnected()
{
    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT idFirstFriends, idSecoundFriends FROM friends WHERE idFirstFriends = :first "
                  "OR idSecoundFriends = :secound");
    query.bindValue(":first", id);
    query.bindValue(":secound", id);
    query.exec();

    while(query.next())
    {
        for(int i = 0; i < socketClients->size(); i++)
        {
            if(socketClients->at(i) != this)
            {
                if(query.value("idFirstFriends").toString() != id)
                {
                    QMetaObject::invokeMethod(socketClients->at(i), "checkFriendsUpdateOnline", Qt::AutoConnection,
                                              Q_ARG(QString, query.value("idFirstFriends").toString()),
                                              Q_ARG(QString, id),
                                              Q_ARG(QString, "offline"));
                }
                else
                {
                    QMetaObject::invokeMethod(socketClients->at(i), "checkFriendsUpdateOnline", Qt::AutoConnection,
                                              Q_ARG(QString, query.value("idSecoundFriends").toString()),
                                              Q_ARG(QString, id),
                                              Q_ARG(QString, "offline"));
                }
            }
        }
    }

    socketClients->removeOne(this);
    query.prepare("UPDATE users SET status = '0' WHERE id = :id");
    query.bindValue(":id", id);
    query.exec();
    Socket->close();
    quit();
}

void SocketThread::SlotSendToClient(QString str)
{
    QByteArray  arrBlock;

    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out << str;

    Socket->write(arrBlock);
    Socket->flush();
}

void SocketThread::gettingFriends()
{
    // запрос на поиск записи в бд
    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT id, idFirstFriends, idSecoundFriends FROM friends WHERE idFirstFriends = :first "
                  "OR idSecoundFriends = :secound");
    query.bindValue(":first", id);
    query.bindValue(":secound", id);
    query.exec();

    QString infoFriend;
    QStringList listStructInfoFriend;
    QString friends;

    QSqlQuery queryInfo = QSqlQuery(DataBase);
    QSqlQuery queryUnreadMessages = QSqlQuery(DataBase);
    while(query.next())
    {
        friends = query.value("idFirstFriends").toString();
        if(friends == id)
        {
            friends = query.value("idSecoundFriends").toString();
        }

        queryInfo.prepare("SELECT Login, Email, FIO, Phone, Status FROM users WHERE id = :id");
        queryInfo.bindValue(":id", friends);
        queryInfo.exec();

        queryInfo.next();

        queryUnreadMessages.prepare("SELECT idFirstFriends, idSecoundFriends, "
                                    "unreadMessageFirstFriend, unreadMessageSecoundFriend "
                                    "FROM friends "
                                    "WHERE (idFirstFriends = :firstFriend "
                                    "AND idSecoundFriends = :first) "
                                    "OR (idFirstFriends = :first "
                                    "AND idSecoundFriends = :firstFriend)");
        queryUnreadMessages.bindValue(":firstFriend", friends);
        queryUnreadMessages.bindValue(":first", id);
        queryUnreadMessages.exec();

        queryUnreadMessages.next();

        QString underMessages;

        queryUnreadMessages.value(0).toString() == id ?
                    underMessages = queryUnreadMessages.value(2).toString() :
                    underMessages = queryUnreadMessages.value(3).toString();

        infoFriend = "/5/";

        infoFriend += friends + " ";
        infoFriend += queryInfo.value("Login").toString() + " ";
        infoFriend += queryInfo.value("Email").toString() + " ";
        infoFriend += queryInfo.value("FIO").toString() + " ";
        infoFriend += queryInfo.value("Phone").toString() + " ";
        infoFriend += queryInfo.value("Status").toString() + " ";
        infoFriend += underMessages;

        listStructInfoFriend << infoFriend;
    }

    for(int i = 0; i < listStructInfoFriend.size(); i++)
    {
        SlotSendToClient(listStructInfoFriend[i]);
        QThread::msleep(15);
    }
}

void SocketThread::closeRoomFriendHangUp(QString name)
{
    AllRooms->closeRoomFriendHangUp(name);
}

void SocketThread::sendHistoryMessage(QString idFriend)
{
    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT idFirstFriends, message, time FROM messages WHERE (idFirstFriends = :firstFriend "
                  "AND idSecoundFriends = :first) "
                  "OR (idFirstFriends = :first "
                  "AND idSecoundFriends = :firstFriend)");
    query.bindValue(":firstFriend", idFriend);
    query.bindValue(":first", id);
    query.exec();

    QStringList messages;

    while(query.next())
    {
        messages << "/getMessages/"
                    + query.value(2).toString()
                    + "!"
                    + query.value(0).toString()
                    + "!"
                    + query.value(1).toString();
    }

    for(int i = 0; i < messages.size() - 1; i++)
    {
        SlotSendToClient(messages[i]);
        QThread::msleep(15);
    }

    SlotSendToClient(messages[messages.size() - 1] + "/lastMessage/");
    QThread::msleep(15);
}

QString SocketThread::getId()
{
    return id;
}
