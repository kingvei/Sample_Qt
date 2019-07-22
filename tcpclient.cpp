#include "tcpclient.h"
#include <QMessageBox>
#include "mainwindow.h"

TcpClient::TcpClient()
{
    this->status = false;
    this->tcpSocket = new QTcpSocket;
    this->srvIP = new QHostAddress;

    connect(tcpSocket, &QTcpSocket::connected, [=]{status = true;});
    connect(tcpSocket, &QTcpSocket::disconnected, [=]{status = false;});
}

TcpClient::~TcpClient()
{
    delete tcpSocket;
    delete srvIP;
}

void TcpClient::establish(QString ip, QString port)
{
    if(!status) //未建立连接
    {
        if(!srvIP->setAddress(ip))
        {
            QMessageBox::warning(nullptr, tr("Error"), tr("Server IP address is wrong!"));
            return;
        }
        tcpSocket->connectToHost(*srvIP, port.toUShort());
        connect(tcpSocket, &QTcpSocket::readyRead, this, &TcpClient::receive);
        status = true;
    }
    else
    {
        tcpSocket->disconnectFromHost(); //发出disconnected信号
        status = false;
    }
}


void TcpClient::receive(void)
{
    //qDebug() << "TcpClient::receive() threadID is:" << QThread::currentThreadId();
    while(tcpSocket->bytesAvailable()>0)
    {
        qint64 size = tcpSocket->bytesAvailable();

        QByteArray msg;
        msg.resize(int(size));
        tcpSocket->read(msg.data(), size);
        this->receivedMsg.push_back(msg);
        emit tcpReceiveSignal(msg);
    }
}

void TcpClient::send(QByteArray msg)
{
    tcpSocket->write(msg, msg.size());
    qDebug() << "TcpClient::send() threadID is:" << QThread::currentThreadId();
}
