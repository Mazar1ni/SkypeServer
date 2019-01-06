#include "stunserver.h"
#include <QDataStream>

StunServer::StunServer(QObject *parent)
    : QThread(parent)
{

}

void StunServer::run()
{
    socket = new QUdpSocket;
    socket->bind(QHostAddress::Any, 3478);

    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()), Qt::DirectConnection);

    exec();
}

void StunServer::slotSendToClient(QString str, QHostAddress address, uint16_t port)
{
    QByteArray  arrBlock;

    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out << str;

    socket->writeDatagram(arrBlock, address, port);
}

void StunServer::onReadyRead()
{
    QByteArray datagram;
    datagram.resize(socket->pendingDatagramSize());

    QHostAddress address;
    quint16 port;

    socket->readDatagram(datagram.data(), datagram.size(), &address, &port);

    bool conversionOK = false;
    QHostAddress ip4Address(address.toIPv4Address(&conversionOK));
    QString ip4String;
    if (conversionOK)
    {
        ip4String = ip4Address.toString();
    }

    slotSendToClient("/StunResponse/" + ip4String + "!" + QString::number(port), address, port);
}
