#include "socketthread.h"
#include <QDataStream>
#include "rooms.h"
#include "room.h"
#include "tcpsocket.h"
#include <QFile>
#include <QDate>

using namespace std;

SocketThread::SocketThread(int descriptor, Rooms* rooms, QSqlDatabase db, QList<SocketThread *> *Clients, QObject * parent) :
    QThread(parent), SocketDescriptor(descriptor), AllRooms(rooms), DataBase(db), socketClients(Clients)
{

}

SocketThread::~SocketThread()
{

}

void SocketThread::run()
{
    Socket = new TcpSocket;
    Socket->setSocketDescriptor(SocketDescriptor);

    connect(Socket, SIGNAL(readyRead()), this, SLOT(OnReadyRead()), Qt::DirectConnection);
    connect(Socket, SIGNAL(disconnected()), this, SLOT(OnDisconnected()), Qt::DirectConnection);

    exec();
}

void SocketThread::Authentication(QString login, QString pass)
{
    // запрос на поиск логина в бд
    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT id, Login, Email, Phone, Name, Password, Status FROM users WHERE Login = :login");
    query.bindValue(":login", login);
    query.exec();

    // получение пароля из строки
    QString password;
    QString email;
    QString nameUser;
    QString phone;
    QString status;

    if(query.next())
    {
        password = query.value("Password").toString();
        id = query.value("id").toString();
        email = query.value("Email").toString();
        nameUser = query.value("Name").toString();
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
                fakeUser = true;
                SlotSendToClient("/-1/");
            }
            else
            {
                loginUser = login;
                SlotSendToClient("/1/" + id + "!" + loginUser + "!" + email + "!" + nameUser + "!" + phone);

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
    // проверка, добавился ли клент в указанную комнату
    room = AllRooms->GetInRoom(name, pass, Socket);
    if(room != nullptr)
    {
        qDebug("getinroom");
        SlotSendToClient("/9/");
    }
}

void SocketThread::checkId(QString testId, QString nameRoom, QString passRoom)
{
    if(id == testId)
    {
        QString mess = "/29/" + nameRoom + "!" + passRoom;

        QByteArray  arr;

        QDataStream out(&arr, QIODevice::WriteOnly);
        out << mess;

        QMetaObject::invokeMethod(Socket, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, arr));

    }
}

void SocketThread::checkIdForSendMessage(QString testId, QString message, QString idSender)
{
    if(id == testId)
    {
        QString mess = "/newMessage/" + idSender  + "!" + message;

        QByteArray  arr;

        QDataStream out(&arr, QIODevice::WriteOnly);
        out << mess;

        QMetaObject::invokeMethod(Socket, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, arr));
    }
}

void SocketThread::checkFriendsUpdateOnline(QString testId, QString idFriend, QString status)
{
    if(id == testId)
    {
        QString mess = "/33/" + idFriend + ":" + status;

        QByteArray  arr;

        QDataStream out(&arr, QIODevice::WriteOnly);
        out << mess;

        QMetaObject::invokeMethod(Socket, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, arr));
    }
}

void SocketThread::chekIdForSendBeginnigCall(QString testId, QString idFriend)
{
    if(id == testId)
    {
        QString mess = "/beginnigCall/" + idFriend;

        QByteArray  arr;

        QDataStream out(&arr, QIODevice::WriteOnly);
        out << mess;

        QMetaObject::invokeMethod(Socket, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, arr));
    }
}

void SocketThread::chekIdForSendEndingCall(QString testId, QString idFriend)
{
    if(id == testId)
    {
        QString mess = "/endingCall/" + idFriend;

        QByteArray  arr;

        QDataStream out(&arr, QIODevice::WriteOnly);
        out << mess;

        QMetaObject::invokeMethod(Socket, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, arr));
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
        room->SendAudioToAllClients(Socket, buffer);
    }
    else if(buffer.indexOf("/camera/") != -1 || buffer.indexOf("/end/") != -1)
    {
        //Socket->write(buffer);
        room->SendAudioToAllClients(Socket, buffer);
    }
    else if(str.indexOf("/startRecordVideo/") != -1 || str.indexOf("/stopRecordVideo/") != -1)
    {
        room->SendAudioToAllClients(Socket, str.toLocal8Bit());
    }
    // проверка, относится ли этот текст в авторизации
    else if(str.indexOf("/1/") != -1)
    {
        // удаление идентификатора
        str.remove(str.length() - 3, 3);

        // разделение логина и пароля
        QStringList list = str.split("!");
        Authentication(list[0], list[1]);
    }
    // проверка, относитмя ли этот текст к созданию комнаты
    else if(str.indexOf("/2/") != -1)
    {
        // удаление идентификатора
        str.remove(str.length() - 3, 3);

        // разделение имя комнаты и пароля комнаты
        QStringList list = str.split(":");
        CreateRooms(list[0], list[1]);
    }
    // проверка, относитмя ли этот текст ко входу в комнату
    else if(str.indexOf("/3/") != -1)
    {
        // удаление идентификатора
        str.remove(str.length() - 3, 3);

        // разделение имя комнаты и пароля комнаты
        QStringList list = str.split(":");
        GetInRoom(list[0], list[1]);
    }
    // проверка, относится ли этот текст к получению друзей клиентом
    else if(str.indexOf("/4/") != -1)
    {
        // удаление идентификатора
        str.remove(str.length() - 3, 3);

        gettingFriends();
    }
    // проверка, относится ли этот текст к начал звонка другу
    else if(str.indexOf("/19/") != -1)
    {
        // удаление идентификатора
        str.remove(0, 4);

        for(int i = 0; i < socketClients->size(); i++)
        {
            if(socketClients->at(i) != this)
            {
                CreateRooms(loginUser + "-" + str, "123");
                QMetaObject::invokeMethod(socketClients->at(i), "checkId", Qt::AutoConnection,
                                          Q_ARG(QString, str),
                                          Q_ARG(QString, loginUser + "-" + str),
                                          Q_ARG(QString, "123"));
            }
        }
    }
    // проверка, относится ли этот текст к взятию трубки другом
    else if(str.indexOf("/30/") != -1)
    {
        // удаление идентификатора
        str.remove(0, 4);

        // разделение имя комнаты и пароля комнаты
        QStringList list = str.split(":");
        GetInRoom(list[0], list[1]);
    }
    // проверка, относится ли этот текст к сбросу звонка другом
    else if(str.indexOf("/31/") != -1)
    {
        // удаление идентификатора
        str.remove(0, 4);

        // разделение имя комнаты и пароля комнаты
        QStringList list = str.split(":");
        closeRoomFriendHangUp(list[0]);
    }
    // проверка, относится ли этот текст к получению истории сообщений
    else if(str.indexOf("/HistoryMessage/") != -1)
    {
        // удаление идентификатора
        str.remove(0, 16);

        QStringList list = str.split("!");

        sendHistoryMessage(list[0], list[1]);
    }
    // проверка, относится ли этот текст к отправке сообщения другу
    else if(str.indexOf("/message/") != -1)
    {
        // удаление идентификатора
        str.remove(0, 9);

        QStringList list = str.split("!");

        QDate date = QDate::currentDate();
        QTime time = QTime::currentTime();

        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("INSERT INTO messages (idFirstFriends, idSecoundFriends, message, time, status) "
                      "VALUES (:idFirstFriends, :idSecoundFriends, :message, :time, :status)");
        query.bindValue(":idFirstFriends", id);
        query.bindValue(":idSecoundFriends", list[0]);
        query.bindValue(":message", list[1]);
        query.bindValue(":time", date.toString("dd MMMM yyyy") + "!" + time.toString("hh:mm"));
        query.bindValue(":status", "1");
        query.exec();

        QString idMessage = query.lastInsertId().toString();

        {
            query.prepare("SELECT idFirstFriends, idSecoundFriends, "
                          "unreadMessageFirstFriend, unreadMessageSecoundFriend "
                          "FROM friends "
                          "WHERE (idFirstFriends = :firstFriend "
                          "AND idSecoundFriends = :first) "
                          "OR (idFirstFriends = :first "
                          "AND idSecoundFriends = :firstFriend)");
            query.bindValue(":firstFriend", list[0]);
            query.bindValue(":first", id);
            query.exec();

            int firstUnread;
            int secoundUnread;

            query.next();

            firstUnread = query.value(2).toInt();
            secoundUnread = query.value(3).toInt();

            query.value(0).toString() == list[0] ? firstUnread++ : secoundUnread++;

            query.prepare("UPDATE friends SET unreadMessageFirstFriend = :firstUnread, "
                          "unreadMessageSecoundFriend = :secoundUnread "
                          "WHERE (idFirstFriends = :firstFriend "
                          "AND idSecoundFriends = :first) "
                          "OR (idFirstFriends = :first "
                          "AND idSecoundFriends = :firstFriend)");
            query.bindValue(":firstUnread", firstUnread);
            query.bindValue(":secoundUnread", secoundUnread);
            query.bindValue(":firstFriend", list[0]);
            query.bindValue(":first", id);
            query.exec();
        }

        for(int i = 0; i < socketClients->size(); i++)
        {
            if(socketClients->at(i) != this)
            {
                QMetaObject::invokeMethod(socketClients->at(i), "checkIdForSendMessage", Qt::AutoConnection,
                                      Q_ARG(QString, list[0]),
                                      Q_ARG(QString, date.toString("dd MMMM yyyy")
                                            + "!" + time.toString("hh:mm") + "!" + str + "!" + "1"),
                                      Q_ARG(QString, id + "!" + idMessage));
            }
        }
        checkIdForSendMessage(id, date.toString("dd MMMM yyyy")
                         + "!" + time.toString("hh:mm") + "!" + str, id + "!" + idMessage);
    }
    else if(str.indexOf("/readUnreadMessages/") != -1)
    {
        str.remove(0, 20);

        QStringList list = str.split("!");

        QString idMessage = list[0];
        QString idFriend = list[1];

        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("UPDATE messages SET status = 0 "
                      "WHERE id = :idMessage");
        query.bindValue(":idMessage", idMessage);
        query.exec();

        query.prepare("SELECT idFirstFriends, idSecoundFriends, "
                      "unreadMessageFirstFriend, unreadMessageSecoundFriend "
                      "FROM friends "
                      "WHERE (idFirstFriends = :firstFriend "
                      "AND idSecoundFriends = :first) "
                      "OR (idFirstFriends = :first "
                      "AND idSecoundFriends = :firstFriend)");
        query.bindValue(":firstFriend", idFriend);
        query.bindValue(":first", id);
        query.exec();

        int firstUnread;
        int secoundUnread;

        query.next();

        firstUnread = query.value(2).toInt();
        secoundUnread = query.value(3).toInt();

        query.value(0).toString() == id ? firstUnread-- : secoundUnread--;

        query.prepare("UPDATE friends SET unreadMessageFirstFriend = :firstUnread, "
                      "unreadMessageSecoundFriend = :secoundUnread "
                      "WHERE (idFirstFriends = :firstFriend "
                      "AND idSecoundFriends = :first) "
                      "OR (idFirstFriends = :first "
                      "AND idSecoundFriends = :firstFriend)");
        query.bindValue(":firstUnread", firstUnread);
        query.bindValue(":secoundUnread", secoundUnread);
        query.bindValue(":firstFriend", idFriend);
        query.bindValue(":first", id);
        query.exec();
    }
    else if(str.indexOf("/beginnigCall/") != -1)
    {
        str.remove(0, 14);

        //P.S. str - это id друга
        for(int i = 0; i < socketClients->size(); i++)
        {
            if(socketClients->at(i) != this)
            {
                QMetaObject::invokeMethod(socketClients->at(i), "chekIdForSendBeginnigCall", Qt::AutoConnection,
                                      Q_ARG(QString, str),
                                      Q_ARG(QString, id));
            }
        }
    }
    // проверка, относится ли этот текст к концу звонка
    else if(str.indexOf("/endCall/") != -1)
    {
        room->closeRoom(Socket);
    }
    else if(str.indexOf("/endingCall/") != -1)
    {
        str.remove(0, 12);

        //P.S. str - это id друга
        for(int i = 0; i < socketClients->size(); i++)
        {
            if(socketClients->at(i) != this)
            {
                QMetaObject::invokeMethod(socketClients->at(i), "chekIdForSendEndingCall", Qt::AutoConnection,
                                      Q_ARG(QString, str),
                                      Q_ARG(QString, id));
            }
        }
    }
    else
    {
        //Socket->write(buffer);
        room->SendAudioToAllClients(Socket, buffer);
    }
}

void SocketThread::OnDisconnected()
{
    if(fakeUser == true)
    {
        Socket->close();
        quit();
        return;
    }
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

        queryInfo.prepare("SELECT Login, Email, Name, Phone, Status FROM users WHERE id = :id");
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

        infoFriend += friends + "!";
        infoFriend += queryInfo.value("Login").toString() + "!";
        infoFriend += queryInfo.value("Email").toString() + "!";
        infoFriend += queryInfo.value("Name").toString() + "!";
        infoFriend += queryInfo.value("Phone").toString() + "!";
        infoFriend += queryInfo.value("Status").toString() + "!";
        infoFriend += underMessages;

        listStructInfoFriend << infoFriend;
    }

    for(int i = 0; i < listStructInfoFriend.size(); i++)
    {
        SlotSendToClient(listStructInfoFriend[i]);
        QThread::msleep(100);
    }
}

void SocketThread::closeRoomFriendHangUp(QString name)
{
    AllRooms->closeRoomFriendHangUp(name, Socket);
}

void SocketThread::sendHistoryMessage(QString idFriend, QString i)
{
    int limit = 20 * i.toInt();
    int offset = 20 * (i.toInt() - 1);

    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT idFirstFriends, idSecoundFriends, message, time, id, status FROM messages "
                  "WHERE (idFirstFriends = :firstFriend "
                  "AND idSecoundFriends = :first) "
                  "OR (idFirstFriends = :first "
                  "AND idSecoundFriends = :firstFriend) ORDER BY id DESC LIMIT :limit OFFSET :offset");
    query.bindValue(":firstFriend", idFriend);
    query.bindValue(":first", id);
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);
    query.exec();

    QStringList messages;

    while(query.next())
    {
        messages << query.value(1).toString()
                    + "!"
                    + query.value(4).toString()
                    + "!"
                    + query.value(3).toString()
                    + "!"
                    + query.value(0).toString()
                    + "!"
                    + query.value(2).toString()
                    + "!"
                    + query.value(5).toString();
    }

    QString message = "/getMessages/";

    if(i == "1")
    {
        for(int i = messages.size() - 1; i >= 0; i--)
        {
            //SlotSendToClient(messages[i]);
            //QThread::msleep(15);
            message += messages[i] + "/!/";
        }
        SlotSendToClient(message);
    }
    else
    {
        for(int i = 0; i <= messages.size() - 1; i++)
        {
            //SlotSendToClient(messages[i]);
            //QThread::msleep(15);
            message += messages[i] + "/!/";
        }
        SlotSendToClient(message);
    }
}
