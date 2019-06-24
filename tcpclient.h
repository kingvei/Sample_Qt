#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QTcpSocket>
#include <QHostAddress>
#include <QWidget>
#include <QMainWindow>
#include <QThread>

class TcpClient : public QObject
{
    Q_OBJECT

public:
    TcpClient();
    ~TcpClient();

    bool status;
    QTcpSocket *tcpSocket;
    QHostAddress *srvIP;

    void establish(QString ip, QString port);
    void send(QByteArray msg);
    void receive(void);

signals:
    void tcpReceiveSignal(QByteArray msg);
};

#endif // TCPCLIENT_H
