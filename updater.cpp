#include "updater.h"
#include <QFile>
#include <QDataStream>
#include <QDir>
#include <QDateTime>

Updater::Updater(int descriptor, QObject *parent) : QThread(parent), socketDescriptor(descriptor)
{

}

void Updater::run()
{
    socket = new QTcpSocket;
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()), Qt::DirectConnection);
    connect(socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()), Qt::DirectConnection);

    exec();
}

void Updater::updateProgram(QString path)
{
    QDir currentFolder(path);

    currentFolder.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    currentFolder.setSorting(QDir::Name);

    QFileInfoList folderitems(currentFolder.entryInfoList());

    foreach (QFileInfo i_file, folderitems)
    {
        if(i_file.fileName().indexOf(".") == -1)
        {
            updateProgram(i_file.path() + "/" + i_file.fileName() + "/");
        }
        else
        {
            QFile testFile(path + i_file.fileName());
            QString lastModified;
            if(testFile.open(QIODevice::ReadOnly))
            {
                QFileInfo fileInfo(testFile);
                QDateTime d;
                d = fileInfo.lastModified();
                lastModified = d.toString("dd MM yyyy");
            }
            path.remove(mainPath);
            slotSendClient("/fileTest/" + path + i_file.fileName() + "$" + lastModified);
            QThread::msleep(15);
        }
    }
}

void Updater::removeFile()
{
    if(!updateFiles.isEmpty())
    {
        QFile out(mainPath + updateFiles.first());
        if(!out.open(QIODevice::ReadOnly))
        {
            slotSendClient("/removeFile/" + updateFiles.first());
            updateFiles.removeFirst();
            QThread::msleep(15);
        }
        else
        {
            updateFiles.removeFirst();
            QThread::msleep(15);
            removeFile();
        }
    }
    else
    {
        slotSendClient("/endUpdate/" + version);
    }
}

void Updater::onReadyRead()
{
    QByteArray buffer;
    buffer = socket->readAll();

    QDataStream in(buffer);

    QString str;
    in >> str;

    if(str.indexOf("/version/") != -1)
    {
        str.remove("/version/");

        QFile out("version/version.txt");
        QString text;
        if(out.open(QIODevice::ReadOnly))
        {
            QTextStream stream(&out);
            text = stream.readAll();
        }

        version = text;

        if(str == text)
        {
            slotSendClient("/match/");
        }
        else
        {
            slotSendClient("/doNotMatch/");
            QThread::msleep(30);
            mainPath = "version/Skype-" + text + "/";
            updateProgram(mainPath);
            qDebug("wellHow");
            slotSendClient("/wellHow/");
        }
    }
    else if(str.indexOf("/downloadFile/") != -1)
    {
        str.remove("/downloadFile/");
        QFile out(mainPath + str);
        if(out.open(QIODevice::ReadOnly))
        {
            QByteArray dataFile = out.readAll();

            socket->write(dataFile + "endFile");
        }
    }
    else if(str.indexOf("/fileTest/") != -1)
    {
        str.remove("/fileTest/");
        updateFiles.append(str);
    }
    else if(str.indexOf("/wellHow/") != -1)
    {
        removeFile();
    }
}

void Updater::onDisconnected()
{
    socket->close();
    quit();
    deleteLater();
}

void Updater::slotSendClient(QString str)
{
    QByteArray  arrBlock;

    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out << str;

    socket->write(arrBlock);
    socket->flush();
}
