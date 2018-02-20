#include "filetransfer.h"
#include <QDataStream>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QTimer>

FileTransfer::FileTransfer(int descriptor, QSqlDatabase db, QObject *parent)
    : QThread(parent), DataBase(db), socketDescriptor(descriptor)
{

}

void FileTransfer::run()
{
    socket = new QTcpSocket;
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()), Qt::DirectConnection);
    connect(socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()), Qt::DirectConnection);

    exec();
}

void FileTransfer::endFile(QByteArray buffer)
{
    uploadFile->write(buffer);
    uploadFile->close();

    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT IconName from users WHERE id = :id");
    query.bindValue(":id", id);
    query.exec();
    query.next();

    lastIconName = query.value(0).toString();
    QTimer::singleShot(5000, [this](){
        QFile("clientsFiles/" + id + "/" + this->lastIconName).remove();
    });

    query.prepare("UPDATE users SET IconName = :iconName WHERE id = :id");
    query.bindValue(":id", id);
    query.bindValue(":iconName", nameFile);
    query.exec();

    uploadFile->deleteLater();
}

void FileTransfer::onReadyRead()
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

        QSqlQuery query = QSqlQuery(DataBase);
        query.prepare("SELECT * FROM users WHERE id = :id AND IdentificationNumber = :idNumber");
        query.bindValue(":id", id);
        query.bindValue(":idNumber", list[1]);
        query.exec();

        if(!query.next())
        {
            slotSendClient("/invalidData/");
            deleteLater();
        }
    }
    else if(str.indexOf("/informationFileIcon/") != -1)
    {
        str.remove("/informationFileIcon/");

        nameFile = str;

        QDir("clientsFiles/").mkdir(id);
        uploadFile = new QFile("clientsFiles/" + id + "/" + nameFile);
        uploadFile->open(QFile::WriteOnly);
    }
    else if(buffer.indexOf("endFile") != -1)
    {
        buffer.remove(buffer.size() - 8, 7);
        endFile(buffer);
    }
    else if(buffer.indexOf("mainIconEndFile") != -1)
    {
        buffer.remove(buffer.size() - 16, 15);
        endFile(buffer);
        slotSendClient("/successTransferMainIcon/");
    }
    else if(str.indexOf("/informationFileAcquisitionIcon/") != -1)
    {
        str.remove("/informationFileAcquisitionIcon/");

        QStringList list = str.split("!");

        QFile file("clientsFiles/" + list[0] + "/" + list[1]);
        file.open(QFile::ReadOnly);
        QByteArray dataFile = file.readAll();

        socket->write(dataFile + "/endFileIcon/");
    }
    else
    {
        uploadFile->write(buffer);
    }
}

void FileTransfer::onDisconnected()
{
    qDebug() << "disconnect";
    socket->close();
    quit();
    deleteLater();
}

void FileTransfer::slotSendClient(QString str)
{
    QByteArray  arrBlock;

    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out << str;

    socket->write(arrBlock);
}
