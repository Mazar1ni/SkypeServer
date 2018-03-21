#ifndef WIDGET_H
#define WIDGET_H

#include <QObject>
#include <QSqlDatabase>
#include <QTcpServer>
#include <QThread>

class Rooms;
class SocketThread;
class FileTrnsfer;

class Widget : public QObject
{
    Q_OBJECT

public:
    Widget(QObject* parent = nullptr);
    ~Widget();

private slots:
    void newConnection();
    void newConnectionTransferFile();
    void newConnectionUpdater();
    QSqlDatabase restartDatabase();

private:
    QSqlDatabase DataBase;
    QList<SocketThread*>* socketClients;
    Rooms* rooms;
    QTcpServer* systemServer;
    QTcpServer* fileTransferServer;
    QTcpServer* updaterServer;
    QThread thread;
};

#endif // WIDGET_H
