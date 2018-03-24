#include "audioserver.h"
#include <QSqlQuery>
#include <QDataStream>
#include "socketthread.h"
#include "room.h"
#include "tcpsocket.h"

AudioServer::AudioServer(int descriptor, QSqlDatabase db, QList<SocketThread*>* socClients, QObject *parent)
    : QThread(parent), database(db), socketDescriptor(descriptor), socketClients(socClients)
{

}

void AudioServer::run()
{
    socket = new TcpSocket;
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()), Qt::DirectConnection);
    connect(socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()), Qt::DirectConnection);

    exec();
}

void AudioServer::slotSendClient(QString str)
{
    QByteArray  arrBlock;

    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out << str;

    socket->write(arrBlock);
    socket->flush();
}

void AudioServer::newRoom(Room *r)
{
    room = r;
}

void AudioServer::onReadyRead()
{
    QByteArray buffer;
    buffer = socket->readAll();

    QDataStream in(buffer);

    QString str;
    in >> str;

    if(str.indexOf("/connect/") != -1)
    {
        str.remove("/connect/");
        QStringList list = str.split("!");

        id = list[0];

        QSqlQuery query = QSqlQuery(database);
        query.prepare("SELECT * FROM users WHERE id = :id AND IdentificationNumber = :idNumber");
        query.bindValue(":id", id);
        query.bindValue(":idNumber", list[1]);
        query.exec();

        if(query.next())
        {
            for(int i = 0; i < socketClients->size(); i++)
            {
                QMetaObject::invokeMethod(socketClients->at(i), "checkIdForAudioServer", Qt::DirectConnection,
                                          Q_ARG(QString, id),
                                          Q_ARG(AudioServer*, this),
                                          Q_ARG(TcpSocket*, socket));
            }
            slotSendClient("/connected/");
        }
    }
    else if(buffer.indexOf("/18/") != -1)
    {
        sendAudioToClients(socket, buffer);
    }
}

void AudioServer::onDisconnected()
{
    socket->close();
    terminate();
    wait();
    deleteLater();
}
