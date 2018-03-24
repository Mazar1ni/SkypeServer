#ifndef SOCKETTHREAD_H
#define SOCKETTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>

class Rooms;
class Room;
class Server;
class TcpSocket;
class AudioServer;

class SocketThread : public QThread
{
    Q_OBJECT

public:
    SocketThread(int descriptor, Rooms *rooms, QSqlDatabase db, QList<SocketThread*>* Clients, QObject *parent = nullptr);
    ~SocketThread();

    void run();
    void Authentication(QString login, QString pass);
    void CreateRooms(QString name, QString pass);
    void GetInRoom(QString name, QString pass);
    void gettingFriends();
    void gettingRecent();
    void gettingInviteFriends();
    void closeRoomFriendHangUp(QString name);
    void sendHistoryMessage(QString idFriend, QString i);

    int numberDisplay = 0;
    QString id;

private:
    inline void slotRestartDatabase();

public slots:
    void checkIdForAudioServer(QString testId, AudioServer* audio, TcpSocket* socket);

private slots:
    void SlotSendToClient(QString str);
    void OnReadyRead();
    void OnDisconnected();
    void checkId(QString testId, QString nameRoom, QString passRoom);
    void checkIdForSendMessage(QString testId, QString message, QString idSender);
    void checkFriendsUpdateOnline(QString testId, QString idFriend, QString status);
    void chekIdForSendBeginnigCall(QString testId, QString idFriend);
    void chekIdForSendEndingCall(QString testId, QString idFriend);
    void checkIdForInviteToFriend(QString testId, QString idFriend);
    void checkIdForAcceptInviteToFriend(QString testId, QString idFriend);
    void changeInfoAboutYourself(QString testId, QString idFriend, QString info);
    void sendFriendUpdateIcon(QString testId, QString idFriend, QString iconName);

signals:
    QSqlDatabase restartDatabase();
    void newRoom(Room*);

private:
    int SocketDescriptor;
    Rooms* AllRooms;
    QSqlDatabase DataBase;
    Room* room = nullptr;
    TcpSocket* Socket;
    QString loginUser;
    QList<SocketThread*>* socketClients;
    bool fakeUser = false;
    AudioServer* audioServer;
    TcpSocket* socketAudio;

};

#endif // SOCKETTHREAD_H
