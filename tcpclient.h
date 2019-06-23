#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QTcpSocket>
#include <QHostAddress>
#include <QWidget>
#include <QMainWindow>

class TcpClient : public QObject
{
    Q_OBJECT

public:
    TcpClient();
    ~TcpClient();

    bool status;
    QTcpSocket *tcpSocket;
    QHostAddress *srvIP;

    void tcpEstablish(QString ip, QString port);
    void tcpSend(QByteArray msg);
private slots:
    void slotReceive(void);

signals:
    void tcpReceived(QByteArray);
};

#endif // TCPCLIENT_H
