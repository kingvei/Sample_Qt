#include "tcpclient.h"
#include <QMessageBox>
#include <QDebug>

TcpClient::TcpClient()
{
    this->status = false;
    tcpSocket = new QTcpSocket(this);
    srvIP = new QHostAddress;
}

TcpClient::~TcpClient()
{
    delete tcpSocket;
    delete srvIP;
}

void TcpClient::tcpEstablish(QString ip, QString port)
{
    if(!status) //未建立连接
    {
        if(!srvIP->setAddress(ip))
        {
            QMessageBox::warning(nullptr, tr("Error"), tr("Server IP address is wrong!"));
            return;
        }
        tcpSocket->connectToHost(*srvIP, port.toUShort());
        connect(tcpSocket, &QTcpSocket::readyRead, this, &TcpClient::tcpReceive);
        status = true;
    }
    else
    {
        tcpSocket->disconnectFromHost(); //发出disconnected信号
        status = false;
    }
}


void TcpClient::tcpReceive(void)
{
    while(tcpSocket->bytesAvailable()>0)
    {
        qint64 size = tcpSocket->bytesAvailable();

        QByteArray msg;
        msg.resize(int(size));
        tcpSocket->read(msg.data(), size);
        emit tcpReceiveSignal(msg);
    }
}

void TcpClient::tcpSend(QByteArray msg)
{
    tcpSocket->write(msg, msg.size());
}
