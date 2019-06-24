#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->srvIPLineEdit->setText("192.168.1.105"); //服务器默认IP
    ui->srvPortLineEdit->setText("5000"); //服务器默认端口
    ui->tcpEstablishButton->setText("连接");
    ui->tcpSendButton->setEnabled(false);
    ui->runSystemButton->setText(tr("启动系统"));

    this->setWindowTitle(tr("采集控制系统"));
    this->srvIP.clear();
    this->srvPort.clear();
    this->srvData.clear();
    this->cltData.clear();
    this->board = new SampleBoard;

    this->tcpClient = new TcpClient;
    this->thread = new QThread;
    tcpClient->moveToThread(thread);
    connect(this->tcpClient, &TcpClient::tcpReceiveSignal, this, &MainWindow::processTcpReceivedMsg);
    //connect(this, &MainWindow::sendCmdSignal, this->tcpClient, &TcpClient::send);
    thread->start();

    this->timer = new QTimer;
    this->timer->start(20);
    connect(this->timer, &QTimer::timeout, this, &MainWindow::updateUI);

    connect(this, &MainWindow::destroyed, this, &MainWindow::dealClose);

    this->series = new QLineSeries;
    this->chart = new QChart();
    this->chartView = new QChartView(chart);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete tcpClient;
    delete board;
}

void MainWindow::processTcpReceivedMsg(QByteArray msg)
{
    qDebug() << "MainWindow::processTcpReceivedMsg() threadID is :" << QThread::currentThreadId();
    //board->decodeMsg(msg);

    ui->tcpRecvText->moveCursor(QTextCursor::End);
    ui->tcpRecvText->insertPlainText(msg);
}

void MainWindow::on_tcpEstablishButton_clicked()
{
    srvIP = ui->srvIPLineEdit->text();
    srvPort = ui->srvPortLineEdit->text();
    tcpClient->establish(srvIP, srvPort);

    connect(tcpClient->tcpSocket, &QTcpSocket::connected, [=]
    {
        ui->tcpEstablishButton->setText("断开");
        ui->tcpSendButton->setEnabled(true);
    });

    connect(tcpClient->tcpSocket, &QTcpSocket::disconnected, [=]
    {
        ui->tcpEstablishButton->setText("连接");
        ui->tcpSendButton->setEnabled(false);
    });
//    if(ui->tcpEstablishButton->text() == tr("连接"))
//    {
//        if(tcpClient->status == false) //未连接成功
//            return;
//        ui->tcpEstablishButton->setText("断开");
//        ui->tcpSendButton->setEnabled(true);
//    }
//    else
//    {
//        ui->tcpEstablishButton->setText("连接");
//        ui->tcpSendButton->setEnabled(false);
//    }
}

void MainWindow::on_tcpSendButton_clicked()
{
    cltData = ui->tcpSendText->toPlainText();
    tcpClient->send(cltData.toLatin1());
}

void MainWindow::dealClose()
{
    //退出子线程
    thread->quit();
    //回收资源
    thread->wait();
}

/*
 * 使用checkbox复选框设置所有继电器状态
 */
void MainWindow::on_setAllRelayButton_clicked()
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

    char buf[9] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x02, (char)num, (char)0xFF, (char)0xFC, '\0'};
    quint16 crc = crc16(buf+4, 4);
    buf[2] = (char)(crc & 0x0F);
    buf[3] = (char)(crc >> 8);

    QByteArray cmd = QByteArray::fromRawData(buf, 8);
    emit sendCmdSignal(cmd);
    tcpClient->send(cmd);
}

/*
 * 运行系统
 */
void MainWindow::on_runSystemButton_clicked()
{
    char buf[9] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x03, 0, (char)0xFF, (char)0xFC, '\0'};
    if(ui->runSystemButton->text() == tr("启动系统"))
    {
        buf[5] = 0x01;
        ui->runSystemButton->setText(tr("关闭系统"));
    }
    else
    {
        buf[5] = 0x02;
        ui->runSystemButton->setText(tr("启动系统"));
    }
    quint16 crc = crc16(buf+4, 4);
    buf[2] = (char)(crc & 0x0F);
    buf[3] = (char)(crc >> 8);

    QByteArray cmd = QByteArray::fromRawData(buf, 8);
    emit sendCmdSignal(cmd);
    tcpClient->send(cmd);
}

/*
 * 设置单个继电器状态
 */
inline void MainWindow::setSingleRelay(quint8 a)
{
    char buf[9] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x01, (char)a, (char)0xFF, (char)0xFC, '\0'};
    quint16 crc = crc16(buf+4, 4);
    buf[2] = (char)(crc & 0x0F);
    buf[3] = (char)(crc >> 8);

    QByteArray cmd = QByteArray::fromRawData(buf, 8);
    emit sendCmdSignal(cmd);
    tcpClient->send(cmd);
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

void MainWindow::updateUI()
{
    // update time
    QString rtcTime = QString::number(board->rtc.year) + "-" + QString::number(board->rtc.month) + "-" + QString::number(board->rtc.day) + " "
                + QString::number(board->rtc.hour)+ ":" + QString::number(board->rtc.minute) + ":" + QString::number(board->rtc.second);
    ui->rtcLineEdit->setText(rtcTime);

    QTime time = QTime::currentTime();
    QDate date = QDate::currentDate();
    QString sysTime = QString::number(date.year()) + "-" + QString::number(date.month()) + "-" + QString::number(date.day()) + " "
                + QString::number(time.hour())+ ":" + QString::number(time.minute()) + ":" + QString::number(time.second());
    ui->sysTimeLabel->setText(sysTime);

//    this->updateIoState();
//    this->updateAdcChart();
//    this->updateCanData();
//    this->updateRs485Data();
}

void MainWindow::updateAdcChart()
{
    //构建图表的数据源
    series->clear();
    for(int i=0; i<100; i++) //todo
    {
        series->append(i, 10*qSin(i*3.14159/20.0));
    }

    //构建图表
    chart->removeAllSeries();
    chart->legend()->hide(); //隐藏图例
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->setTitle("ADC");

    //构建 QChartView，并设置抗锯齿、标题、大小
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setWindowTitle("ADC通道1");
    chartView->resize(400, 300);
    chartView->show();
}

void MainWindow::updateCanData()
{

}

void MainWindow::updateRs485Data()
{

}

void MainWindow::updateIoState()
{
//    if(board->din & 0x01) {
//        ui->ioStateLabel_1->setText("闭合");
//    } else {
//        ui->ioStateLabel_1->setText("断开");
//    }

//    if(board->din & (0x01<<1)) {
//        ui->ioStateLabel_2->setText("闭合");
//    } else {
//        ui->ioStateLabel_2->setText("断开");
//    }

//    if(board->din & (0x01<<2)) {
//        ui->ioStateLabel_3->setText("闭合");
//    } else {
//        ui->ioStateLabel_3->setText("断开");
//    }

//    if(board->din & (0x01<<3)) {
//        ui->ioStateLabel_4->setText("闭合");
//    } else {
//        ui->ioStateLabel_4->setText("断开");
//    }

//    if(board->din & (0x01<<4)) {
//        ui->ioStateLabel_5->setText("闭合");
//    } else {
//        ui->ioStateLabel_5->setText("断开");
//    }

//    if(board->din & (0x01<<5)) {
//        ui->ioStateLabel_6->setText("闭合");
//    } else {
//        ui->ioStateLabel_6->setText("断开");
//    }

//    if(board->din & (0x01<<6)) {
//        ui->ioStateLabel_7->setText("闭合");
//    } else {
//        ui->ioStateLabel_7->setText("断开");
//    }

//    if(board->din & (0x01<<7)) {
//        ui->ioStateLabel_8->setText("闭合");
//    } else {
//        ui->ioStateLabel_8->setText("断开");
//    }
}
