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
    //ui->runSystemButton->setEnabled(false);

    this->setWindowTitle(tr("采集控制系统"));
    this->srvIP.clear();
    this->srvPort.clear();
    this->srvData.clear();
    this->cltData.clear();
    this->board = new SampleBoard;

    //tcp接收线程
    this->tcpClient = new TcpClient;
    this->thread = new QThread;
    tcpClient->moveToThread(thread);
    connect(this->tcpClient, &TcpClient::tcpReceiveSignal, this, &MainWindow::processTcpReceivedMsg);
    //connect(this, &MainWindow::sendCmdSignal, this->tcpClient, &TcpClient::send); //不能放在主线程 todo
    thread->start();

    //定时器，用于更新UI
    this->timer = new QTimer;
    this->timer->start(500); //启动定时器
    connect(this->timer, &QTimer::timeout, this, &MainWindow::updateUI);

    this->configLineChart();
    this->board->din = 0xF0;

    connect(this, &MainWindow::destroyed, this, &MainWindow::dealClose);
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

    //todo 关闭子窗体
//    this->timer->stop();
//    this->graphScene->clear();
//    this->graphView->close();
}

/*
 * 使用checkbox复选框设置所有继电器状态
 */
void MainWindow::on_setAllRelayButton_clicked()
{
    quint8 num = 0;
    if(ui->relayCheckBox_0->isChecked()) num += 0x01;
    if(ui->relayCheckBox_1->isChecked()) num += 0x01 << 1;
    if(ui->relayCheckBox_2->isChecked()) num += 0x01 << 2;
    if(ui->relayCheckBox_3->isChecked()) num += 0x01 << 3;
    if(ui->relayCheckBox_4->isChecked()) num += 0x01 << 4;
    if(ui->relayCheckBox_5->isChecked()) num += 0x01 << 5;
    if(ui->relayCheckBox_6->isChecked()) num += 0x01 << 6;
    if(ui->relayCheckBox_7->isChecked()) num += 0x01 << 7;

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

void MainWindow::on_relayButton_0_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_0->text() == tr("闭合")){
        res = 0x80;
        ui->relayButton_0->setText(tr("断开"));
    }else{
        res = 0x40;
        ui->relayButton_0->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_1_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_1->text() == tr("闭合")){
        res = 0x81;
        ui->relayButton_1->setText(tr("断开"));
    }else{
        res = 0x41;
        ui->relayButton_1->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_2_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_2->text() == tr("闭合")){
        res = 0x82;
        ui->relayButton_2->setText(tr("断开"));
    }else{
        res = 0x42;
        ui->relayButton_2->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_3_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_3->text() == tr("闭合")){
        res = 0x83;
        ui->relayButton_3->setText(tr("断开"));
    }else{
        res = 0x43;
        ui->relayButton_3->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_4_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_4->text() == tr("闭合")){
        res = 0x84;
        ui->relayButton_4->setText(tr("断开"));
    }else{
        res = 0x44;
        ui->relayButton_4->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_5_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_5->text() == tr("闭合")){
        res = 0x85;
        ui->relayButton_5->setText(tr("断开"));
    }else{
        res = 0x45;
        ui->relayButton_5->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_6_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_6->text() == tr("闭合")){
        res = 0x86;
        ui->relayButton_6->setText(tr("断开"));
    }else{
        res = 0x46;
        ui->relayButton_6->setText(tr("闭合"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_7_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_7->text() == tr("闭合")){
        res = 0x87;
        ui->relayButton_7->setText(tr("断开"));
    }else{
        res = 0x47;
        ui->relayButton_7->setText(tr("闭合"));
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

    this->updateState();
    this->updateAdcChart();
    this->updateCanData();
    this->updateRs485Data();
}

void MainWindow::updateAdcChart()
{
    static int x=0;
    x++;
    series->clear();
    for(int i=0; i<100; i++)
    {
        series->append(i, 2.5+2.5*qSin(2*3.14159/(10.0*x)*i));
    }
}

void MainWindow::updateCanData()
{

}

void MainWindow::updateRs485Data()
{

}

void MainWindow::updateState()
{
    if(board->din & 0x01) {
        ui->stateLabel_0->setText("1号: 闭合");
    } else {
        ui->stateLabel_0->setText("1号: 断开");
    }

    if(board->din & (0x01<<1)) {
        ui->stateLabel_1->setText("2号: 闭合");
    } else {
        ui->stateLabel_1->setText("2号: 断开");
    }

    if(board->din & (0x01<<2)) {
        ui->stateLabel_2->setText("3号: 闭合");
    } else {
        ui->stateLabel_2->setText("3号: 断开");
    }

    if(board->din & (0x01<<3)) {
        ui->stateLabel_3->setText("4号: 闭合");
    } else {
        ui->stateLabel_3->setText("4号: 断开");
    }

    if(board->din & (0x01<<4)) {
        ui->stateLabel_4->setText("5号: 闭合");
    } else {
        ui->stateLabel_4->setText("5号: 断开");
    }

    if(board->din & (0x01<<5)) {
        ui->stateLabel_5->setText("6号: 闭合");
    } else {
        ui->stateLabel_5->setText("6号: 断开");
    }

    if(board->din & (0x01<<6)) {
        ui->stateLabel_6->setText("7号: 闭合");
    } else {
        ui->stateLabel_6->setText("7号: 断开");
    }

    if(board->din & (0x01<<7)) {
        ui->stateLabel_7->setText("8号: 闭合");
    } else {
        ui->stateLabel_7->setText("8号: 断开");
    }
}

void MainWindow::configLineChart()
{
    //画布
    this->graphScene = new QGraphicsScene;
    this->graphView = new QGraphicsView(graphScene);
    graphView->setWindowTitle("ADC数据");
    graphView->setRenderHint(QPainter::Antialiasing);
    graphView->setSceneRect(0, 0, 630, 280);
    graphScene->setBackgroundBrush(QBrush(QColor(240, 240, 240)));

    this->series = new QLineSeries;
    this->chart = new QChart();
    chart->legend()->hide(); //隐藏图例
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->setTitle("Simple line chart example");
    chart->setGeometry(10, 10, 300, 260);

    //设置坐标轴初始显示范围
    this->axisX = new QValueAxis();//轴变量、数据系列变量，都不能声明为局部临时变量
    this->axisY = new QValueAxis();//创建X/Y轴
    axisX->setRange(0, 100); //设置X/Y显示的区间
    axisY->setRange(-0.3, 5.3);
    chart->setAxisX(axisX);
    chart->setAxisY(axisY); //设置chart的坐标轴
    series->attachAxis(axisX); //连接数据集与坐标轴。特别注意：如果不连接，那么坐标轴和数据集的尺度就不相同
    series->attachAxis(axisY);

    graphScene->addItem(chart);
    graphView->move(50, 50); //窗体移动到某个点
    graphView->show();
}
