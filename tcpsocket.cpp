#include "tcpsocket.h"


TcpSocket::TcpSocket(QObject *parent) : QTcpSocket(parent)
{

}

void TcpSocket::onWrite(QByteArray message)
{
    write(message);
}
