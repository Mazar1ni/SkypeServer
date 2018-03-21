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

void FileTransfer::endFile()
{
    uploadFile.close();

    QSqlQuery query = QSqlQuery(DataBase);
    query.prepare("SELECT IconName from users WHERE id = :id");
    query.bindValue(":id", id);
    query.exec();
    query.next();

    lastIconName = query.value(0).toString();
    QFile("Z:/root/Release/clientsFiles/" + id + "/" + this->lastIconName).remove();

    query.prepare("UPDATE users SET IconName = :iconName WHERE id = :id");
    query.bindValue(":id", id);
    query.bindValue(":iconName", nameFile);
    query.exec();
}

void FileTransfer::endUploadFile()
{
    uploadFile.close();
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
        }
        isCanSendData = true;
    }
    else if(str.indexOf("/informationFileIcon/") != -1 && isCanSendData == true)
    {
        str.remove("/informationFileIcon/");

        QStringList list = str.split("~");

        nameFile = list[0];
        sizeFile = list[1];
        typeFile = "mainIconEndFile";

        QDir("Z:/root/Release/clientsFiles/").mkdir(id);
        uploadFile.setFileName("Z:/root/Release/clientsFiles/" + id + "/" + nameFile);
        uploadFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered);
    }
    else if(str.indexOf("/informationFileAcquisitionIcon/") != -1 && isCanSendData == true)
    {
        str.remove("/informationFileAcquisitionIcon/");

        QStringList list = str.split("!");

        QFile file("Z:/root/Release/clientsFiles/" + list[0] + "/" + list[1]);
        file.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        QByteArray dataFile = file.readAll();

        slotSendClient("/sizeFile/" + QString::number(dataFile.size()));
        QThread::msleep(100);

        socket->write(dataFile);
        socket->flush();
    }
    else if(str.indexOf("/informationUploadFile/") != -1 && isCanSendData == true)
    {
        str.remove("/informationUploadFile/");

        QStringList list = str.split("~");

        sizeFile = list[1];
        typeFile = "uploadEndFile";

        QDir("Z:/root/Release/clientsFiles/").mkdir(id);
        uploadFile.setFileName("Z:/root/Release/clientsFiles/" + id + "/" + list[0]);
        uploadFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered);
    }
    else if(str.indexOf("/informationAcquisitionFile/") != -1 && isCanSendData == true)
    {
        str.remove("/informationAcquisitionFile/");

        QStringList list = str.split("!");

        QFile file("Z:/root/Release/clientsFiles/" + list[0] + "/" + list[1]);
        file.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        QByteArray dataFile = file.readAll();

        slotSendClient("/sizeFile/" + QString::number(dataFile.size()));
        QThread::msleep(100);

        socket->write(dataFile);
        socket->flush();
    }
    else if(isCanSendData == true)
    {
        uploadFile.write(buffer);

        currentSizeFile += buffer.size();

        if(currentSizeFile == sizeFile.toInt())
        {
            if(typeFile == "mainIconEndFile")
            {
                endFile();
                slotSendClient("/successTransferMainIcon/");
            }
            else if(typeFile == "uploadEndFile")
            {
                endUploadFile();
                slotSendClient("/successTransferFile/");
            }
        }
    }
}

void FileTransfer::onDisconnected()
{
    socket->close();
    terminate();
    wait();
    deleteLater();
}

void FileTransfer::slotSendClient(QString str)
{
    QByteArray  arrBlock;

    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out << str;

    socket->write(arrBlock);
    socket->flush();
}
