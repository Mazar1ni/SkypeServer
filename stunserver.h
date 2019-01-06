#ifndef AUDIOSERVER_H
#define AUDIOSERVER_H

#include <QThread>
#include <QUdpSocket>

class StunServer : public QThread
{
    Q_OBJECT
public:
    StunServer(QObject *parent = nullptr);
    void run();
    void slotSendToClient(QString str, QHostAddress address, uint16_t port);

private slots:
    void onReadyRead();

private:
    QUdpSocket* socket;
};

#endif // AUDIOSERVER_H
