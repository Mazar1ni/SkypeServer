#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <QTcpSocket>

class TcpSocket : public QTcpSocket
{

    Q_OBJECT

public:
    TcpSocket(QObject * parent = nullptr);

public slots:
    void onWrite(QByteArray message);

};

#endif // TCPSOCKET_H
