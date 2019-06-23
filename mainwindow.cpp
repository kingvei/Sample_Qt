#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTextBrowser>
#include <QScrollBar>
#include "crc16.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->srvIPLineEdit->setText("192.168.1.220"); //服务器默认IP
    ui->srvPortLineEdit->setText("5000"); //服务器默认端口
    ui->tcpEstablishButton->setText("连接");
    ui->tcpSendButton->setEnabled(false);

    ui->runSystemButton->setText(tr("启动系统"));



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
    tcpClient->tcpSend(cltData.toLatin1());
}


/*
 * 使用checkbox设置所有继电器状态
 */
void MainWindow::on_setAllRelayButton_Box_clicked()
{
    quint8 num = 0;
    if(ui->relayCheckBox_1->isChecked()) num += 0x01;
    if(ui->relayCheckBox_2->isChecked()) num += 0x01 << 1;
    if(ui->relayCheckBox_3->isChecked()) num += 0x01 << 2;
    if(ui->relayCheckBox_4->isChecked()) num += 0x01 << 3;
    if(ui->relayCheckBox_5->isChecked()) num += 0x01 << 4;
    if(ui->relayCheckBox_6->isChecked()) num += 0x01 << 5;
    if(ui->relayCheckBox_7->isChecked()) num += 0x01 << 6;
    if(ui->relayCheckBox_8->isChecked()) num += 0x01 << 7;

    char cmd[8] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x02, (char)num, (char)0xFF, (char)0xFC};
    quint16 crc = crc16(cmd+4, 4);
    cmd[2] = (char)(crc & 0x0F);
    cmd[3] = (char)(crc >> 8);

    emit sendCmdSignal(QByteArray(cmd));
}

/*
 * 输入十六进制数设置所有继电器状态
 */
void MainWindow::on_setAllRelayButton_Num_clicked()
{
    QString str = ui->relayNumLineEdit->text();
    if(str.size() != 4)
        return;
    if(str[0]!='0' || (str[1]!='x' && str[1]!='X') ||
       str[2]<'0' || str[2]>'9' || str[3]<'0' || str[3]>'9')
        return;

    quint8 num = (str[2].toLatin1() -'0')*16 + (str[3].toLatin1() - '0');

    char cmd[8] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x02, (char)num, (char)0xFF, (char)0xFC};
    quint16 crc = crc16(cmd+4, 4);
    cmd[2] = (char)(crc & 0x0F);
    cmd[3] = (char)(crc >> 8);

    emit sendCmdSignal(QByteArray(cmd));
}

/*
 * 运行系统
 */
void MainWindow::on_runSystemButton_clicked()
{
    char cmd[8] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x03, 0, (char)0xFF, (char)0xFC};
    if(ui->runSystemButton->text() == tr("启动系统"))
    {
        cmd[5] = 0x01;
        ui->runSystemButton->setText(tr("关闭系统"));
    }
    else
    {
        cmd[5] = 0x01;
        ui->runSystemButton->setText(tr("启动系统"));
    }
    quint16 crc = crc16(cmd+4, 4);
    cmd[2] = (char)(crc & 0x0F);
    cmd[3] = (char)(crc >> 8);

    emit sendCmdSignal(QByteArray(cmd));
}

/*
 * 设置单个继电器状态
 */
inline void MainWindow::setSingleRelay(quint8 a)
{
    char cmd[8] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x01, (char)a, (char)0xFF, (char)0xFC};
    quint16 crc = crc16(cmd+4, 4);
    cmd[2] = (char)(crc & 0x0F);
    cmd[3] = (char)(crc >> 8);

    emit sendCmdSignal(QByteArray(cmd));
}

void MainWindow::on_relayButton_1_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_1->text() == tr("闭合")){
        res = 0x80;
        ui->relayButton_1->setText(tr("断开"));
    }else{
        res = 0x40;
        ui->relayButton_1->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_2_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_2->text() == tr("闭合")){
        res = 0x81;
        ui->relayButton_2->setText(tr("断开"));
    }else{
        res = 0x41;
        ui->relayButton_2->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_3_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_3->text() == tr("闭合")){
        res = 0x82;
        ui->relayButton_3->setText(tr("断开"));
    }else{
        res = 0x42;
        ui->relayButton_3->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_4_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_4->text() == tr("闭合")){
        res = 0x83;
        ui->relayButton_4->setText(tr("断开"));
    }else{
        res = 0x43;
        ui->relayButton_4->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_5_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_5->text() == tr("闭合")){
        res = 0x84;
        ui->relayButton_5->setText(tr("断开"));
    }else{
        res = 0x44;
        ui->relayButton_5->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_6_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_6->text() == tr("闭合")){
        res = 0x85;
        ui->relayButton_6->setText(tr("断开"));
    }else{
        res = 0x45;
        ui->relayButton_6->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_7_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_7->text() == tr("闭合")){
        res = 0x86;
        ui->relayButton_7->setText(tr("断开"));
    }else{
        res = 0x46;
        ui->relayButton_7->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_8_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_8->text() == tr("闭合")){
        res = 0x87;
        ui->relayButton_8->setText(tr("断开"));
    }else{
        res = 0x47;
        ui->relayButton_8->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}
