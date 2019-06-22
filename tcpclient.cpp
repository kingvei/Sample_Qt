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
        QObject::connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(slotReceive()));
        status = true;
    }
    else
    {
        tcpSocket->disconnectFromHost(); //发出disconnected信号
        status = false;
    }
}


void TcpClient::slotReceive(void)
{
    while(tcpSocket->bytesAvailable()>0)
    {
        qint64 size = tcpSocket->bytesAvailable();

        QByteArray msg;
        msg.resize(int(size));
        tcpSocket->read(msg.data(), size);
        emit tcpReceived(msg);
    }
}

inline void TcpClient::tcpSend(QString msg)
{
    tcpSocket->write(msg.toLatin1(), msg.length());
}
