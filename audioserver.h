#ifndef AUDIOSERVER_H
#define AUDIOSERVER_H

#include <QThread>
#include <QTcpSocket>
#include <QSqlDatabase>

class SocketThread;
class Room;
class TcpSocket;

class AudioServer : public QThread
{
    Q_OBJECT
public:
    AudioServer(int descriptor, QSqlDatabase db, QList<SocketThread*>* socClients, QObject *parent = nullptr);
    void run();
    void slotSendClient(QString str);

signals:
    void sendAudioToClients(TcpSocket *SentAudio, QByteArray buffer);

public slots:
    void newRoom(Room* r);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QSqlDatabase database;
    int socketDescriptor;
    TcpSocket* socket;
    QString id;
    QList<SocketThread*>* socketClients;
    Room* room = nullptr;

};

#endif // AUDIOSERVER_H
