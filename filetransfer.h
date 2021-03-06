#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <QThread>
#include <QSqlDatabase>
#include <QTcpSocket>
#include <QFile>

class FileTransfer : public QThread
{
    Q_OBJECT
public:
    explicit FileTransfer(int descriptor, QSqlDatabase db, QObject *parent = nullptr);
    void run();
    void endFile();
    void endUploadFile();

signals:

private slots:
    void onReadyRead();
    void onDisconnected();
    void slotSendClient(QString str);

private:
    QSqlDatabase DataBase;
    QTcpSocket* socket;
    int socketDescriptor;
    QFile uploadFile;
    QByteArray buff;

    QString id;
    QString nameFile;
    QString lastIconName;
    bool isCanSendData = false;
    QString sizeFile;
    int currentSizeFile = 0;
    QString typeFile;

};

#endif // FILETRANSFER_H
