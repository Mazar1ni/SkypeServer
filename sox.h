#ifndef SOX_H
#define SOX_H

#include <QProcess>

class SocketThread;

class Sox : public QProcess
{
    Q_OBJECT
public:
    Sox(SocketThread* soc, QObject* parent = nullptr);

signals:
    void sendSound();

public slots:
    void removeNoise();
    void test(int);

private:
    int count = 0;
    SocketThread* socket;

};

#endif // SOX_H
