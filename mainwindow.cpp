#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTextBrowser>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->srvIPLineEdit->setText("192.168.1.220"); //服务器默认IP
    ui->srvPortLineEdit->setText("5000"); //服务器默认端口
    ui->tcpEstablishButton->setText("连接");
    ui->tcpSendButton->setEnabled(false);

    this->board = new SampleBoard;

    this->srvIP.clear();
    this->srvPort.clear();
    this->srvData.clear();
    this->cltData.clear();
    this->tcpClient = new TcpClient;
    this->setWindowTitle(tr("采集控制系统"));
}

void MainWindow::processTcpReceivedMsg(QByteArray msg)
{
    board->decodeMsg(msg);

    ui->tcpRecvText->moveCursor(QTextCursor::End);
    ui->tcpRecvText->insertPlainText(msg);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete tcpClient;
    delete board;
}

void MainWindow::on_tcpEstablishButton_clicked()
{
    srvIP = ui->srvIPLineEdit->text();
    srvPort = ui->srvPortLineEdit->text();
    tcpClient->tcpEstablish(srvIP, srvPort);
    if(tcpClient->status == false)
        return;

    if(ui->tcpEstablishButton->text() == tr("连接"))
    {
        ui->tcpEstablishButton->setText("断开");
        ui->tcpSendButton->setEnabled(true);
        QObject::connect(this->tcpClient, SIGNAL(tcpReceived(QByteArray)), this, SLOT(processTcpReceivedMsg(QByteArray)));
    }
    else
    {
        ui->tcpEstablishButton->setText("连接");
        ui->tcpSendButton->setEnabled(false);
    }
}

void MainWindow::on_tcpSendButton_clicked()
{
    cltData = ui->tcpSendText->toPlainText();
    tcpClient->tcpSend(cltData);
}
