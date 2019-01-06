#include "socketthread.h"
#include <QDataStream>
#include "rooms.h"
#include "room.h"
#include "tcpsocket.h"
#include "stunserver.h"
#include <QDate>
#include <QFile>
#include <QTimer>

using namespace std;

SocketThread::SocketThread(int descriptor, Rooms* rooms, QSqlDatabase db, QList<SocketThread *> *Clients,
                           QObject * parent) :
    QThread(parent), SocketDescriptor(descriptor), AllRooms(rooms), DataBase(db), socketClients(Clients)
{
    QTimer *restartDatabaseTimer = new QTimer;
    connect(restartDatabaseTimer, &QTimer::timeout, [this](){
        slotRestartDatabase();
    });
    restartDatabaseTimer->start(30000);
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
    slotRestartDatabase();

    // ������ �� ����� ������ � ��
    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT id, Login, Email, Phone, Name, Password, Status, IconName FROM users WHERE Login = :login");
    query.bindValue(":login", login);
    query.exec();

    // ��������� ������ �� ������
    QString password;
    QString email;
    QString nameUser;
    QString phone;
    QString status;
    QString iconName;

    if(query.next())
    {
        password = query.value("Password").toString();
        id = query.value("id").toString();
        email = query.value("Email").toString();
        nameUser = query.value("Name").toString();
        phone = query.value("Phone").toString();
        status = query.value("Status").toString();
        iconName = query.value("IconName").toString();
    }

    // ��������, ������ �� ����� ������������
    if(!password.isEmpty())
    {
        // ��������, ��������� �� ������
        if(password == pass)
        {
            if(status == "1")
            {
                fakeUser = true;
                SlotSendToClient("/-1/");
            }
            else
            {
                fakeUser = false;
                loginUser = login;

                // ����� ��������� �������������
                query.prepare("SELECT FLOOR(RAND() * 99999) AS IdNu "
                              "FROM users "
                              "WHERE 'IdNu' NOT IN (SELECT IdentificationNumber FROM users) "
                              "LIMIT 1");
                query.exec();
                query.next();

                QString idNumber = query.value("IdNu").toString();

                query.prepare("UPDATE users SET IdentificationNumber = :idNumber "
                              "WHERE id = :id");
                query.bindValue(":idNumber", idNumber);
                query.bindValue(":id", id);
                query.exec();

                SlotSendToClient("/1/" + id + "!" + loginUser + "!" + email + "!" + nameUser + "!" + phone
                                 + "!" + idNumber + "!" + iconName);

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
    room = AllRooms->CreateRoom(name, pass, Socket, &DataBase, ip, port);
    if(room != nullptr)
    {

    }
}

void SocketThread::GetInRoom(QString name, QString pass)
{
    // ��������, ��������� �� ����� � ��������� �������
    room = AllRooms->GetInRoom(name, pass, Socket, ip, port);
    if(room != nullptr)
    {
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

void SocketThread::checkIdForInviteToFriend(QString testId, QString idFriend)
{
    if(id == testId)
    {
        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("SELECT Login, Email, Name, Phone, Status, IconName FROM users WHERE id = :id");
        query.bindValue(":id", idFriend);
        query.exec();

        query.next();

        QString infoFriend = "/inviteToFriend/";

        infoFriend += idFriend + "!";
        infoFriend += query.value("Login").toString() + "!";
        infoFriend += query.value("Email").toString() + "!";
        infoFriend += query.value("Name").toString() + "!";
        infoFriend += query.value("Phone").toString() + "!";
        infoFriend += query.value("Status").toString() + "!";
        infoFriend += "0!";
        infoFriend += query.value("IconName").toString();

        QByteArray  arr;

        QDataStream out(&arr, QIODevice::WriteOnly);
        out << infoFriend;

        QMetaObject::invokeMethod(Socket, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, arr));
    }
}

void SocketThread::checkIdForAcceptInviteToFriend(QString testId, QString idFriend)
{
    if(id == testId)
    {
        QString mess = "/acceptInviteToFriend/" + idFriend;

        QByteArray  arr;

        QDataStream out(&arr, QIODevice::WriteOnly);
        out << mess;

        QMetaObject::invokeMethod(Socket, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, arr));
    }
}

void SocketThread::changeInfoAboutYourself(QString testId, QString idFriend, QString info)
{
    if(id == testId)
    {
        QString mess = "/updateFriendInfo/" + idFriend + "!" + info;

        QByteArray  arr;

        QDataStream out(&arr, QIODevice::WriteOnly);
        out << mess;

        QMetaObject::invokeMethod(Socket, "onWrite", Qt::AutoConnection,
                                  Q_ARG(QByteArray, arr));
    }
}

void SocketThread::sendFriendUpdateIcon(QString testId, QString idFriend, QString iconName)
{
    if(id == testId)
    {
        QString mess = "/updateFriendIcon/" + idFriend + "!" + iconName;

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

    if(buffer.indexOf("/camera/") != -1 || buffer.indexOf("/end/") != -1)
    {
        room->SendAudioToAllClients(Socket, buffer);
    }
    else if(str.indexOf("/startRecordVideo/") != -1 || str.indexOf("/stopRecordVideo/") != -1)
    {
        room->SendAudioToAllClients(Socket, str.toLocal8Bit());
    }
    // ��������, ��������� �� ���� ����� � �����������
    else if(str.indexOf("/1/") != -1)
    {
        // �������� ��������������
        str.remove("/1/");

        // ���������� ������ � ������
        QStringList list = str.split("!");
        Authentication(list[0], list[1]);
    }
    else if(str.indexOf("/registration/") != -1)
    {
        fakeUser = true;
        // �������� ��������������
        str.remove("/registration/");

        // ����������
        QStringList list = str.split("!");
        /*list[0] = name
         * list[1] = email
         * list[2] = login
         * list[3] = phone
         * list[4] = pass
         * list[5] = date
        */
        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("SELECT * FROM users WHERE Login = :login");
        query.bindValue(":login", list[2]);
        query.exec();

        if(query.next())
        {
            SlotSendToClient("/loginExists/");
            return;
        }

        // ����� ��������� �������������
        query.prepare("SELECT FLOOR(RAND() * 99999) AS IdNu "
                      "FROM users "
                      "WHERE 'IdNu' NOT IN (SELECT IdentificationNumber FROM users) "
                      "LIMIT 1");
        query.exec();
        query.next();

        QString idNumber = query.value("IdNu").toString();

        query.prepare("INSERT INTO users (Login, Password, Email, Name, Phone, DateBirth, IdentificationNumber) "
                      "VALUES (:Login, :Password, :Email, :Name, :Phone, :DateBirth, :IdentificationNumber)");
        query.bindValue(":Login", list[2]);
        query.bindValue(":Password", list[4]);
        query.bindValue(":Email", list[1]);
        query.bindValue(":Name", list[0]);
        query.bindValue(":Phone", list[3]);
        query.bindValue(":DateBirth", list[5]);
        query.bindValue(":IdentificationNumber", idNumber);
        query.exec();

        SlotSendToClient("/successfully/");
    }
    // ��������, ��������� �� ���� ����� � �������� �������
    else if(str.indexOf("/2/") != -1)
    {
        // �������� ��������������
        str.remove("/2/");

        // ���������� ��� ������� � ������ �������
        QStringList list = str.split(":");
        CreateRooms(list[0], list[1]);
    }
    // ��������, ��������� �� ���� ����� �� ����� � �������
    else if(str.indexOf("/3/") != -1)
    {
        // �������� ��������������
        str.remove("/3/");

        // ���������� ��� ������� � ������ �������
        QStringList list = str.split(":");
        GetInRoom(list[0], list[1]);
    }
    // ��������, ��������� �� ���� ����� � ��������� ������ ��������
    else if(str.indexOf("/4/") != -1)
    {
        // �������� ��������������
        str.remove("/4/");

        gettingFriends();
    }
    // ��������, ��������� �� ���� ����� � ��������� ��������� ������� ��������
    else if(str.indexOf("/recent/") != -1)
    {
        // �������� ��������������
        str.remove("/recent/");

        gettingRecent();
    }
    else if(str.indexOf("/inviteFr/") != -1)
    {
        // �������� ��������������
        str.remove("/inviteFr/");

        gettingInviteFriends();
    }
    else if(str.indexOf("/newRecent/") != -1)
    {
        // �������� ��������������
        str.remove("/newRecent/");

        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("SELECT Login, Email, Name, Phone, Status, IconName FROM users WHERE id = :id");
        query.bindValue(":id", str);
        query.exec();

        query.next();

        QString infoFriend = "/newRecent/";

        infoFriend += str + "!";
        infoFriend += query.value("Login").toString() + "!";
        infoFriend += query.value("Email").toString() + "!";
        infoFriend += query.value("Name").toString() + "!";
        infoFriend += query.value("Phone").toString() + "!";
        infoFriend += query.value("Status").toString() + "!";
        infoFriend += "0!";
        infoFriend += query.value("IconName").toString();

        SlotSendToClient(infoFriend);
    }
    // ��������, ��������� �� ���� ����� � ����� ������ �����
    else if(str.indexOf("/19/") != -1)
    {
        // �������� ��������������
        str.remove("/19/");

        for(int i = 0; i < socketClients->size(); i++)
        {
            if(socketClients->at(i) != this)
            {
                QString nameRoom = loginUser + "~" + qrand() + "-" + str;
                CreateRooms(nameRoom, "123");
                QMetaObject::invokeMethod(socketClients->at(i), "checkId", Qt::AutoConnection,
                                          Q_ARG(QString, str),
                                          Q_ARG(QString, nameRoom),
                                          Q_ARG(QString, "123"));
            }
        }
    }
    else if(str.indexOf("/potentialFriends/") != -1)
    {
        str.remove("/potentialFriends/");

        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("SELECT id, Login, Email, Name, Phone, Status, IconName "
                      "FROM users "
                      "WHERE (Name LIKE :name) AND "
                      "id NOT IN "
                      "(SELECT idFirstFriends FROM friends WHERE idSecoundFriends = :id) "
                      "AND id NOT IN (SELECT idSecoundFriends FROM friends WHERE idFirstFriends = :id) "
                      "AND id NOT IN (SELECT idFirstFriends FROM invitefriends WHERE idSecoundFriends = :id) "
                      "AND id NOT IN (SELECT idSecoundFriends FROM invitefriends WHERE idFirstFriends = :id)");
        query.bindValue(":name", "%" + str + "%");
        query.bindValue(":id", id);
        query.exec();

        QSqlQuery queryInvite = QSqlQuery(DataBase);
        QString infoFriend;
        QStringList listStructInfoFriend;
        QString inviteFriends;

        while(query.next())
        {
            if(query.value("id").toString() == id)
            {
                continue;
            }

            infoFriend = "/potentialFriends/";

            infoFriend += query.value("id").toString() + "!";
            infoFriend += query.value("Login").toString() + "!";
            infoFriend += query.value("Email").toString() + "!";
            infoFriend += query.value("Name").toString() + "!";
            infoFriend += query.value("Phone").toString() + "!";
            infoFriend += query.value("Status").toString() + "!";
            infoFriend += "0!";
            infoFriend += query.value("IconName").toString() + "!";

            queryInvite.prepare("SELECT * FROM invitefriends "
                                "WHERE (idFirstFriends = :id AND idSecoundFriends = :idFriend) "
                                "OR (idFirstFriends = :idFriend AND idSecoundFriends = :id)");
            queryInvite.bindValue(":id", id);
            queryInvite.bindValue(":idFriend", query.value("id").toString());
            queryInvite.exec();

            if(queryInvite.next())
            {
                inviteFriends = "0";
            }
            else
            {
                inviteFriends = "1";
            }

            infoFriend += inviteFriends;

            listStructInfoFriend << infoFriend;
        }

        for(int i = 0; i < listStructInfoFriend.size(); i++)
        {
            SlotSendToClient(listStructInfoFriend[i]);
            QThread::msleep(100);
        }
    }
    else if(str.indexOf("/inviteToFriends/") != -1)
    {
        str.remove("/inviteToFriends/");

        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("INSERT INTO invitefriends (idFirstFriends, idSecoundFriends) "
                      "VALUES (:idFirstFriends, :idSecoundFriends)");
        query.bindValue(":idFirstFriends", id);
        query.bindValue(":idSecoundFriends", str);
        query.exec();

        for(int i = 0; i < socketClients->size(); i++)
        {
            if(socketClients->at(i) != this)
            {
                QMetaObject::invokeMethod(socketClients->at(i), "checkIdForInviteToFriend", Qt::AutoConnection,
                                      Q_ARG(QString, str),
                                      Q_ARG(QString, id));
            }
        }
    }
    else if(str.indexOf("/acceptInviteToFriends/") != -1)
    {
        str.remove("/acceptInviteToFriends/");

        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("DELETE FROM invitefriends WHERE (idFirstFriends = :id AND idSecoundFriends = :idFriend) "
                      "OR (idFirstFriends = :idFriend AND idSecoundFriends = :id)");
        query.bindValue(":id", id);
        query.bindValue(":idFriend", str);
        query.exec();

        query.prepare("INSERT INTO friends (idFirstFriends, idSecoundFriends) "
                      "VALUES (:idFirstFriends, :idSecoundFriends)");
        query.bindValue(":idFirstFriends", id);
        query.bindValue(":idSecoundFriends", str);
        query.exec();

        for(int i = 0; i < socketClients->size(); i++)
        {
            if(socketClients->at(i) != this)
            {
                QMetaObject::invokeMethod(socketClients->at(i), "checkIdForAcceptInviteToFriend", Qt::AutoConnection,
                                      Q_ARG(QString, str),
                                      Q_ARG(QString, id));
            }
        }
    }
    else if(str.indexOf("/doNotAcceptInviteToFriends/") != -1)
    {
        str.remove("/doNotAcceptInviteToFriends/");

        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("DELETE FROM invitefriends WHERE (idFirstFriends = :id AND idSecoundFriends = :idFriend) "
                      "OR (idFirstFriends = :idFriend AND idSecoundFriends = :id)");
        query.bindValue(":id", id);
        query.bindValue(":idFriend", str);
        query.exec();
    }
    // ��������, ��������� �� ���� ����� � ������ ������ ������
    else if(str.indexOf("/30/") != -1)
    {
        // �������� ��������������
        str.remove("/30/");

        // ���������� ��� ������� � ������ �������
        QStringList list = str.split(":");
        GetInRoom(list[0], list[1]);
    }
    // ��������, ��������� �� ���� ����� � ������ ������ ������
    else if(str.indexOf("/31/") != -1)
    {
        // �������� ��������������
        str.remove("/31/");

        // ���������� ��� ������� � ������ �������
        QStringList list = str.split(":");
        closeRoomFriendHangUp(list[0]);
    }
    // ��������, ��������� �� ���� ����� � ��������� ������� ���������
    else if(str.indexOf("/HistoryMessage/") != -1)
    {
        // �������� ��������������
        str.remove("/HistoryMessage/");

        QStringList list = str.split("!");

        sendHistoryMessage(list[0], list[1]);
    }
    // ��������, ��������� �� ���� ����� � �������� ��������� �����
    else if(str.indexOf("/message/") != -1)
    {
        // �������� ��������������
        str.remove("/message/");

        QStringList list = str.split("!");

        QDate date = QDate::currentDate();
        QTime time = QTime::currentTime();

        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("INSERT INTO messages (idFirstFriends, idSecoundFriends, message, time, status) "
                      "VALUES (:idFirstFriends, :idSecoundFriends, :message, :time, :status)");
        query.bindValue(":idFirstFriends", id);
        query.bindValue(":idSecoundFriends", list[0]);
        query.bindValue(":message", list[1]);
        query.bindValue(":time", date.toString("dd MM yyyy") + "!" + time.toString("hh:mm"));
        query.bindValue(":status", "1");
        query.exec();

        QString idMessage = query.lastInsertId().toString();

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

        for(int i = 0; i < socketClients->size(); i++)
        {
            if(socketClients->at(i) != this)
            {
                QMetaObject::invokeMethod(socketClients->at(i), "checkIdForSendMessage", Qt::AutoConnection,
                                      Q_ARG(QString, list[0]),
                                      Q_ARG(QString, date.toString("dd MM yyyy")
                                            + "!" + time.toString("hh:mm") + "!" + str + "!" + "1"),
                                      Q_ARG(QString, id + "!" + idMessage));
            }
        }
        checkIdForSendMessage(id, date.toString("dd MM yyyy")
                         + "!" + time.toString("hh:mm") + "!" + str, id + "!" + idMessage);
    }
    else if(str.indexOf("/readUnreadMessages/") != -1)
    {
        str.remove("/readUnreadMessages/");

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
        str.remove("/beginnigCall/");

        //P.S. str - ��� id �����
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
    // ��������, ��������� �� ���� ����� � ����� ������
    else if(str.indexOf("/endCall/") != -1)
    {
//        QMetaObject::invokeMethod(room, "closeRoom", Qt::DirectConnection,
//                              Q_ARG(TcpSocket*, socketAudio));
        room->closeRoom(Socket);
    }
    else if(str.indexOf("/endingCall/") != -1)
    {
        str.remove("/endingCall/");

        //P.S. str - ��� id �����
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
    else if(str.indexOf("/profileInfo/") != -1)
    {
        str.remove("/profileInfo/");

        QString info;
        QSqlQuery query = QSqlQuery(DataBase);

        if(str.indexOf("/name/") != -1)
        {
            info = str;
            str.remove("/name/");

            query.prepare("UPDATE users SET Name = :name "
                          "WHERE id = :id");
            query.bindValue(":name", str);
            query.bindValue(":id", id);
            query.exec();
        }
        else if(str.indexOf("/email/") != -1)
        {
            info = str;
            str.remove("/email/");

            query.prepare("UPDATE users SET Email = :email "
                          "WHERE id = :id");
            query.bindValue(":email", str);
            query.bindValue(":id", id);
            query.exec();
        }
        else if(str.indexOf("/phone/") != -1)
        {
            info = str;
            str.remove("/phone/");

            query.prepare("UPDATE users SET Phone = :phone "
                          "WHERE id = :id");
            query.bindValue(":phone", str);
            query.bindValue(":id", id);
            query.exec();
        }
        query.exec();

        if(str.indexOf("/changePass/") != -1)
        {
            str.remove("/changePass/");
            str.remove("/oldPass/");

            int index = str.indexOf("/newPass/");

            QString oldPass = str.left(index);
            str.remove(0, index);

            str.remove("/newPass/");

            QString newPass = str;

            query.prepare("SELECT Password FROM users WHERE id = :id");
            query.bindValue(":id", id);
            query.exec();

            query.next();

            if(oldPass != query.value("Password").toString())
            {
                SlotSendToClient("/Incorrect password/");
                return;
            }

            query.prepare("UPDATE users SET Password = :pass "
                          "WHERE id = :id");
            query.bindValue(":pass", newPass);
            query.bindValue(":id", id);
            query.exec();

            SlotSendToClient("/Password changed/");
        }
        else
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
                            QMetaObject::invokeMethod(socketClients->at(i), "changeInfoAboutYourself",
                                                      Qt::AutoConnection,
                                                      Q_ARG(QString, query.value("idFirstFriends").toString()),
                                                      Q_ARG(QString, id),
                                                      Q_ARG(QString, info));
                        }
                        else
                        {
                            QMetaObject::invokeMethod(socketClients->at(i), "changeInfoAboutYourself",
                                                      Qt::AutoConnection,
                                                      Q_ARG(QString, query.value("idSecoundFriends").toString()),
                                                      Q_ARG(QString, id),
                                                      Q_ARG(QString, info));
                        }
                    }
                }
            }
        }
    }
    else if(str.indexOf("/sendFriendsUpdateIcon/") != -1)
    {
        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("SELECT IconName FROM users WHERE id = :id");
        query.bindValue(":id", id);
        query.exec();
        query.next();

        QString iconName = query.value(0).toString();

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
                        QMetaObject::invokeMethod(socketClients->at(i), "sendFriendUpdateIcon",
                                                  Qt::AutoConnection,
                                                  Q_ARG(QString, query.value("idFirstFriends").toString()),
                                                  Q_ARG(QString, id),
                                                  Q_ARG(QString, iconName));
                    }
                    else
                    {
                        QMetaObject::invokeMethod(socketClients->at(i), "sendFriendUpdateIcon",
                                                  Qt::AutoConnection,
                                                  Q_ARG(QString, query.value("idSecoundFriends").toString()),
                                                  Q_ARG(QString, id),
                                                  Q_ARG(QString, iconName));
                    }
                }
            }
        }

        SlotSendToClient("/updateMainIcon/" + iconName);
    }
    else if(str.indexOf("/IpPort/") != -1)
    {
        str.remove("/IpPort/");

        QStringList list = str.split("!");

        ip = list[0];
        port = list[1];

        SlotSendToClient("/ConnectedAudio/");
    }
    else
    {
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
    terminate();
    wait();
    deleteLater();
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
    // ������ �� ����� ������ � ��
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

        queryInfo.prepare("SELECT Login, Email, Name, Phone, Status, IconName FROM users WHERE id = :id");
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
        infoFriend += underMessages + "!";
        infoFriend += queryInfo.value("IconName").toString();

        listStructInfoFriend << infoFriend;
    }

    for(int i = 0; i < listStructInfoFriend.size(); i++)
    {
        SlotSendToClient(listStructInfoFriend[i]);
        QThread::msleep(100);
    }
}

void SocketThread::gettingRecent()
{
    // ������ �� ����� ������ � ��
    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT DISTINCT idFirstFriends, idSecoundFriends FROM (SELECT friends.idFirstFriends, "
                  "friends.idSecoundFriends, messages.id FROM friends, messages "
                  "WHERE (messages.idFirstFriends = :first OR messages.idSecoundFriends = :secound) "
                  "AND ((friends.idFirstFriends = messages.idFirstFriends "
                  "AND friends.idSecoundFriends = messages.idSecoundFriends) "
                  "OR (friends.idFirstFriends = messages.idSecoundFriends "
                  "AND friends.idSecoundFriends = messages.idFirstFriends)) "
                  "AND (messages.message LIKE :message) "
                  "ORDER BY messages.id DESC) t LIMIT 20");
    query.bindValue(":first", id);
    query.bindValue(":secound", id);
    query.bindValue(":message", "%beginningCall%");
    query.exec();

    QString infoFriend;
    QStringList listStructInfoFriend;
    QString friends;

    QSqlQuery queryInfo = QSqlQuery(DataBase);
    while(query.next())
    {
        friends = query.value("idFirstFriends").toString();
        if(friends == id)
        {
            friends = query.value("idSecoundFriends").toString();
        }

        queryInfo.prepare("SELECT Login, Email, Name, Phone, Status, IconName FROM users WHERE id = :id");
        queryInfo.bindValue(":id", friends);
        queryInfo.exec();

        queryInfo.next();

        infoFriend = "/recent/";

        infoFriend += friends + "!";
        infoFriend += queryInfo.value("Login").toString() + "!";
        infoFriend += queryInfo.value("Email").toString() + "!";
        infoFriend += queryInfo.value("Name").toString() + "!";
        infoFriend += queryInfo.value("Phone").toString() + "!";
        infoFriend += queryInfo.value("Status").toString() + "!";
        infoFriend += "0!";
        infoFriend += queryInfo.value("IconName").toString();

        listStructInfoFriend << infoFriend;
    }

    for(int i = 0; i < listStructInfoFriend.size(); i++)
    {
        SlotSendToClient(listStructInfoFriend[i]);
        QThread::msleep(100);
    }
}

void SocketThread::gettingInviteFriends()
{
    // ������ �� ����� ������ � ��
    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT id, idFirstFriends, idSecoundFriends FROM invitefriends WHERE idSecoundFriends = :secound");
    query.bindValue(":secound", id);
    query.exec();

    QString infoFriend;
    QStringList listStructInfoFriend;
    QString friends;

    QSqlQuery queryInfo = QSqlQuery(DataBase);
    while(query.next())
    {
        friends = query.value("idFirstFriends").toString();
        if(friends == id)
        {
            friends = query.value("idSecoundFriends").toString();
        }

        queryInfo.prepare("SELECT Login, Email, Name, Phone, Status, IconName FROM users WHERE id = :id");
        queryInfo.bindValue(":id", friends);
        queryInfo.exec();

        queryInfo.next();

        infoFriend = "/inviteToFriend/";

        infoFriend += friends + "!";
        infoFriend += queryInfo.value("Login").toString() + "!";
        infoFriend += queryInfo.value("Email").toString() + "!";
        infoFriend += queryInfo.value("Name").toString() + "!";
        infoFriend += queryInfo.value("Phone").toString() + "!";
        infoFriend += queryInfo.value("Status").toString() + "!";
        infoFriend += "0!";
        infoFriend += queryInfo.value("IconName").toString();

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
    int offset = 10 * (i.toInt() - 1);

    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT idFirstFriends, idSecoundFriends, message, time, id, status FROM messages "
                  "WHERE (idFirstFriends = :firstFriend "
                  "AND idSecoundFriends = :first) "
                  "OR (idFirstFriends = :first "
                  "AND idSecoundFriends = :firstFriend) ORDER BY id DESC LIMIT 10 OFFSET :offset");
    query.bindValue(":firstFriend", idFriend);
    query.bindValue(":first", id);
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
            message += messages[i] + "/!/";
        }
        SlotSendToClient(message);
    }
    else
    {
        for(int i = 0; i <= messages.size() - 1; i++)
        {
            message += messages[i] + "/!/";
        }
        SlotSendToClient(message);
    }
}

void SocketThread::slotRestartDatabase()
{
    if(!DataBase.exec("SELECT TRUE").isActive())
    {
        DataBase = restartDatabase();
        QFile file("restartDB.txt");
        if(file.open(QIODevice::Append))
        {
            QDate date = QDate::currentDate();
            QTime time = QTime::currentTime();

            file.write(date.toString("dd MMMM yyyy").toLocal8Bit() + "  -   " +
                       time.toString("hh:mm").toLocal8Bit() +
                       QString::number(DataBase.exec("SELECT TRUE").isActive()).toLocal8Bit() + "/n");
        }
        file.close();
        QThread::msleep(5000);
    }
}
