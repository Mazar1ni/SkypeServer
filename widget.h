#ifndef WIDGET_H
#define WIDGET_H

#include <QObject>
#include <QSqlDatabase>
#include <QTcpServer>

class Rooms;
class SocketThread;

class Widget : public QObject
{
    Q_OBJECT

public:
    Widget(QObject* parent = nullptr);
    ~Widget();

private slots:
    void newConnection();

private:
    QSqlDatabase DataBase;
    QList<SocketThread*>* socketClients;
    Rooms* rooms;
    QTcpServer* systemServer;
};

#endif // WIDGET_H
