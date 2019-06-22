#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tcpclient.h"
#include "bsp.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_tcpEstablishButton_clicked();
    void on_tcpSendButton_clicked();

    void processTcpReceivedMsg(QByteArray);

private:
    Ui::MainWindow *ui;
    QString srvIP;
    QString srvPort;
    QString cltData; //客户端要发送的数据
    QString srvData; //客户端接收的数据
    TcpClient *tcpClient;
    //SampleBoard board;
};

#endif // MAINWINDOW_H
