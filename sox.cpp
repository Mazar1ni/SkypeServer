#include "sox.h"
#include "socketthread.h"
#include <QDebug>
#include <cstdlib>
#include <stdio.h>

Sox::Sox(SocketThread* soc, QObject *parent) : QProcess(parent), socket(soc)
{
    //connect(this, SIGNAL(finished(int)), this, SLOT(test(int)));
}

void Sox::removeNoise()
{
    QString program = ".//sox/sox.exe";
    QStringList arguments;
    arguments << "-r" << "4000" << "-b" << "16" << "-c" << "1"
              << "-L" << "-e" << "signed-integer" << ".//sox/files/test.raw"
    << ".//sox/files/file.wav";
    start(program, arguments);

    if(waitForFinished())
    {
        QString program = ".//sox/sox.exe";
        QStringList arguments;
        arguments << ".//sox/files/file.wav"
                  << ".//sox/files/low.wav" << "lowpass" << "100";

        start(program, arguments);
    }

    if(waitForFinished())
    {
        QString program = ".//sox/sox.exe";
        QStringList arguments;
        arguments << ".//sox/files/low.wav"
                  << ".//sox/files/high.wav" << "highpass" << "600";

        start(program, arguments);
    }

    if(waitForFinished())
    {
        QString program = ".//sox/sox.exe";
        QStringList arguments;
        arguments << ".//sox/files/high.wav"
                  << ".//sox/files/high1.wav" << "vol" << "30";

        start(program, arguments);
    }

    if(waitForFinished())
    {
        QString program = ".//sox/sox.exe";
        QStringList arguments;
        arguments << ".//sox/files/high1.wav" << "-r" << "4000" << "-b" << "16" << "-c" << "1"
                  << "-L" << "-e" << "signed-integer"
                  << ".//sox/files/clear.raw";

        start(program, arguments);
    }

    if(waitForFinished())
    {
        emit(sendSound());
        //QMetaObject::invokeMethod(socket, "sendSound", Qt::AutoConnection);
    }

//    QString program = "python";
//    QStringList arguments;
//    arguments << "2.py";
//    start(program, arguments);

//    system("C:/Project/SkypeServerUbuntu/Release/sox/sox.exe -r 4000 -b 16 -c 1 -L -e signed-integer C:/Project/SkypeServerUbuntu/Release/sox/files/test.raw C:/Project/SkypeServerUbuntu/Release/sox/files/file.wav");

//    system("C:/Project/SkypeServerUbuntu/Release/sox/sox.exe C:/Project/SkypeServerUbuntu/Release/sox/files/file.wav C:/Project/SkypeServerUbuntu/Release/sox/files/low.wav lowpass 100");

//    system("C:/Project/SkypeServerUbuntu/Release/sox/sox.exe C:/Project/SkypeServerUbuntu/Release/sox/files/low.wav C:/Project/SkypeServerUbuntu/Release/sox/files/high.wav highpass 600");
//    system("C:/Project/SkypeServerUbuntu/Release/sox/sox.exe C:/Project/SkypeServerUbuntu/Release/sox/files/high.wav C:/Project/SkypeServerUbuntu/Release/sox/files/high1.wav vol 50");
//    system("C:/Project/SkypeServerUbuntu/Release/sox/sox.exe C:/Project/SkypeServerUbuntu/Release/sox/files/file.wav -r 4000 -b 16 -c 1 -L -e signed-integer C:/Project/SkypeServerUbuntu/Release/sox/files/clear.raw");

//    system("/root/test/sox/sox.exe -r 4000 -b 16 -c 1 -L -e signed-integer "
//       "/root/test/sox/files/test.raw /root/test/sox/files/file.wav");

//    system("/root/test/sox/sox.exe /root/test/sox/files/file.wav /root/test/sox/files/low.wav lowpass 100");

//    system("/root/test/sox/sox.exe /root/test/sox/files/low.wav /root/test/sox/files/high.wav highpass 600");

//    system("/root/test/sox/sox.exe /root/test/sox/files/high.wav /root/test/sox/files/high1.wav vol 50");

//    system("/root/test/sox/sox.exe /root/test/sox/files/file.wav -r 4000 "
//           "-b 16 -c 1 -L -e signed-integer /root/test/sox/files/clear.raw");

//    FILE *popen_result;
//    char buff[512];
//    popen_result = popen("C:/Project/SkypeServerUbuntu/Release/sox/sox.exe -r 4000 -b 16 -c 1 -L -e signed-integer C:/Project/SkypeServerUbuntu/Release/sox/files/test.raw C:/Project/SkypeServerUbuntu/Release/sox/files/file.wav", "r");
//    while(fgets(buff, sizeof(buff), popen_result)!=NULL){

//    }
//    pclose(popen_result);

//    popen_result = popen("C:/Project/SkypeServerUbuntu/Release/sox/sox.exe C:/Project/SkypeServerUbuntu/Release/sox/files/file.wav "
//                         "C:/Project/SkypeServerUbuntu/Release/sox/files/low.wav lowpass 100", "r");
//    while(fgets(buff, sizeof(buff), popen_result)!=NULL){

//    }
//    pclose(popen_result);

//    popen_result = popen("C:/Project/SkypeServerUbuntu/Release/sox/sox.exe C:/Project/SkypeServerUbuntu/Release/sox/files/low.wav "
//                         "C:/Project/SkypeServerUbuntu/Release/sox/files/high.wav highpass 600", "r");
//    while(fgets(buff, sizeof(buff), popen_result)!=NULL){

//    }
//    pclose(popen_result);

//    popen_result = popen("C:/Project/SkypeServerUbuntu/Release/sox/sox.exe C:/Project/SkypeServerUbuntu/Release/sox/files/high.wav "
//                         "C:/Project/SkypeServerUbuntu/Release/sox/files/high1.wav vol 50", "r");
//    while(fgets(buff, sizeof(buff), popen_result)!=NULL){

//    }
//    pclose(popen_result);

//    popen_result = popen("C:/Project/SkypeServerUbuntu/Release/sox/sox.exe C:/Project/SkypeServerUbuntu/Release/sox/files/file.wav -r 4000 -b 16 -c 1 -L -e signed-integer C:/Project/SkypeServerUbuntu/Release/sox/files/clear.raw", "r");
//    while(fgets(buff, sizeof(buff), popen_result)!=NULL){

//    }
//    pclose(popen_result);

//    QMetaObject::invokeMethod(socket, "sendSound", Qt::DirectConnection);

//    deleteLater();
}

void Sox::test(int)
{
    if(count == 0)
    {
        QString program = ".//sox/sox.exe";
        QStringList arguments;
        arguments << ".//sox/files/file.wav"
                  << ".//sox/files/low.wav" << "lowpass" << "100";

        start(program, arguments);
    }

    if(count == 1)
    {
        QString program = ".//sox/sox.exe";
        QStringList arguments;
        arguments << ".//sox/files/low.wav"
                  << ".//sox/files/high.wav" << "highpass" << "600";

        start(program, arguments);
    }

    if(count == 2)
    {
        QString program = ".//sox/sox.exe";
        QStringList arguments;
        arguments << ".//sox/files/high.wav"
                  << ".//sox/files/high1.wav" << "vol" << "30";

        start(program, arguments);
    }

    if(count == 3)
    {
        QString program = ".//sox/sox.exe";
        QStringList arguments;
        arguments << ".//sox/files/high1.wav" << "-r" << "4000" << "-b" << "16" << "-c" << "1"
                  << "-L" << "-e" << "signed-integer"
                  << ".//sox/files/clear.raw";

        start(program, arguments);
    }

//    if(count == 0)
//    {
//        QString program = ".//sox/sox.exe";
//        QStringList arguments;
//        arguments << "sox/files/file.ogg" << "sox/files/clear.ogg" << "noisered" << "sox/noise.prof" << "0.5";

//        start(program, arguments);
//    }
//    if(count == 1)
//    {
//        QString program = ".//sox/sox.exe";
//        QStringList arguments;
//        arguments << "sox/files/clear.ogg" << "-r" << "44100" << "-b" << "16"
//    << "-c" << "1" << "-L" << "-e" << "signed-integer" << "sox/files/fin.wav";

//        start(program, arguments);
//    }
//    if(count == 2)
//    {
//        QString program = ".//sox/sox.exe";
//        QStringList arguments;
//        arguments << "sox/files/fin.wav" << "--bits" << "16" << "--encoding"
//    << "signed-integer" << "--endian" << "little" << "sox/files/finished.raw";

//        start(program, arguments);
//    }
    if(count == 4)
    {
        //emit(sendSound());
        deleteLater();
    }

    count++;
}
