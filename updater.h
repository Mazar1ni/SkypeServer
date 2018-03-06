#ifndef UPDATER_H
#define UPDATER_H

#include <QThread>
#include <QTcpSocket>

class Updater : public QThread
{
    Q_OBJECT
public:
    Updater(int descriptor, QObject* parent = nullptr);
    void run();
    void updateProgram(QString path);
    void removeFile();

private slots:
    void onReadyRead();
    void onDisconnected();
    void slotSendClient(QString str);

private:
    QTcpSocket* socket;
    int socketDescriptor;
    QString mainPath;
    QStringList updateFiles;
    QString version;
};

#endif // UPDATER_H
