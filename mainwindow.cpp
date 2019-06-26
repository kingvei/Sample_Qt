#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->srvIPLineEdit->setText("192.168.1.220"); //服务器默认IP
    ui->srvPortLineEdit->setText("5000"); //服务器默认端口
    ui->tcpEstablishButton->setText("连接");
    ui->tcpSendButton->setEnabled(false);
    ui->runSystemButton->setText(tr("启动"));
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
    this->timer->start(100); //启动定时器
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
    delete timer;

    delete this->graphView;
    delete this->graphScene;
    for(int i=0; i<ADC_CH_NUM; i++)
    {
        delete this->axisX[i];
        delete this->axisY[i];
        delete this->chart[i];
        delete this->series[i];
    }
}

void MainWindow::processTcpReceivedMsg(QByteArray msg)
{
    qDebug() << "MainWindow::processTcpReceivedMsg() threadID is :" << QThread::currentThreadId();
    board->decodeMsg(msg);

    //ui->tcpRecvText->moveCursor(QTextCursor::End);
    //ui->tcpRecvText->insertPlainText(msg);
    QString sysTime = QTime::currentTime().toString("hh:mm:ss");
    ui->tcpRecvText->setText(sysTime + "  Receiving Board Data...\n");
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
 * 运行系统
 */
void MainWindow::on_runSystemButton_clicked()
{
    char buf[9] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x03, 0, (char)0xF0, (char)0xFC, '\0'};
    if(ui->runSystemButton->text() == tr("启动"))
    {
        buf[5] = 0x01;
        ui->runSystemButton->setText(tr("关闭"));
    }
    else
    {
        buf[5] = 0x02;
        ui->runSystemButton->setText(tr("启动"));
    }
    quint16 crc = crc16(buf+4, 4);
    buf[2] = (char)(crc & 0xFF);
    buf[3] = (char)(crc >> 8);

    QByteArray cmd = QByteArray::fromRawData(buf, 8);
    emit sendCmdSignal(cmd);
    tcpClient->send(cmd);
}

void MainWindow::on_resetSystemButton_clicked()
{
    char buf[9] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x03, (char)0x03, (char)0xF0, (char)0xFC, '\0'};

    quint16 crc = crc16(buf+4, 4);
    buf[2] = (char)(crc & 0xFF);
    buf[3] = (char)(crc >> 8);

    QByteArray cmd = QByteArray::fromRawData(buf, 8);
    emit sendCmdSignal(cmd);
    tcpClient->send(cmd);
}

void MainWindow::on_calibrateTimeButton_clicked()
{
    QDate newDate = QDate::currentDate();
    QTime newTime = QTime::currentTime();
    char buf[15] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x04, (char)(newDate.year()-2000), (char)newDate.month(), (char)newDate.day(), (char)newDate.dayOfWeek(),
                    (char)newTime.hour(), (char)newTime.minute(), (char)newTime.second(), (char)0xF0, (char)0xFC, '\0'};

    quint16 crc = crc16(buf+4, 10);
    buf[2] = (char)(crc & 0xFF);
    buf[3] = (char)(crc >> 8);

    QByteArray cmd = QByteArray::fromRawData(buf, 14);
    emit sendCmdSignal(cmd);
    tcpClient->send(cmd);
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

    char buf[9] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x02, (char)num, (char)0xF0, (char)0xFC, '\0'};
    quint16 crc = crc16(buf+4, 4);
    buf[2] = (char)(crc & 0xFF);
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
    char buf[9] = {(char)0xAA, (char)0xBB, 0, 0, (char)0x01, (char)a, (char)0xF0, (char)0xFC, '\0'};
    quint16 crc = crc16(buf+4, 4);
    buf[2] = (char)(crc & 0xFF);
    buf[3] = (char)(crc >> 8);

    QByteArray cmd = QByteArray::fromRawData(buf, 8);
    emit sendCmdSignal(cmd);
    tcpClient->send(cmd);
}

void MainWindow::on_relayButton_0_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_0->text() == tr("闭合1")){
        res = 0x80;
        ui->relayButton_0->setText(tr("断开1"));
    }else{
        res = 0x40;
        ui->relayButton_0->setText(tr("闭合1"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_1_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_1->text() == tr("闭合2")){
        res = 0x81;
        ui->relayButton_1->setText(tr("断开2"));
    }else{
        res = 0x41;
        ui->relayButton_1->setText(tr("闭合2"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_2_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_2->text() == tr("闭合3")){
        res = 0x82;
        ui->relayButton_2->setText(tr("断开3"));
    }else{
        res = 0x42;
        ui->relayButton_2->setText(tr("闭合3"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_3_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_3->text() == tr("闭合4")){
        res = 0x83;
        ui->relayButton_3->setText(tr("断开4"));
    }else{
        res = 0x43;
        ui->relayButton_3->setText(tr("闭合4"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_4_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_4->text() == tr("闭合5")){
        res = 0x84;
        ui->relayButton_4->setText(tr("断开5"));
    }else{
        res = 0x44;
        ui->relayButton_4->setText(tr("闭合5"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_5_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_5->text() == tr("闭合6")){
        res = 0x85;
        ui->relayButton_5->setText(tr("断开6"));
    }else{
        res = 0x45;
        ui->relayButton_5->setText(tr("闭合6"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_6_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_6->text() == tr("闭合7")){
        res = 0x86;
        ui->relayButton_6->setText(tr("断开7"));
    }else{
        res = 0x46;
        ui->relayButton_6->setText(tr("闭合7"));
    }
    this->setSingleRelay(res);
}

void MainWindow::on_relayButton_7_clicked()
{
    quint8 res = 0;
    if(ui->relayButton_7->text() == tr("闭合8")){
        res = 0x87;
        ui->relayButton_7->setText(tr("断开8"));
    }else{
        res = 0x47;
        ui->relayButton_7->setText(tr("闭合8"));
    }
    this->setSingleRelay(res);
}

void MainWindow::updateUI()
{
    // update time
    QDate date(board->rtc.year, board->rtc.month, board->rtc.day);
    QTime time(board->rtc.hour, board->rtc.minute, board->rtc.second);
    QString rtcTime = date.toString("yyyy-MM-dd") + " " + time.toString("hh:mm:ss");
    ui->rtcLineEdit->setText(rtcTime);
    QString sysTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->sysTimeLabel->setText(sysTime);

    this->updateState();
    this->updateAdcChart();
//    this->updateCanData();
//    this->updateRs485Data();
}

void MainWindow::updateCanData()
{
    for(int i=0; i<board->can1Data.size(); i++)
    {
        QString str;
        CanDataType can = board->can1Data[i];
        if(can.ide == 0) str = "stdID: ";
        else str = "ExtID: ";
        str += QString::number(can.id, 10) + ": ";
        str += QString::fromRawData((const QChar*)can.data, can.dlc);
        for(int k=0; k<can.dlc; k++)
        {
            str += QString(char(can.data[k]));
        }
        str += "\n";

        ui->can1Text->moveCursor(QTextCursor::End);
        ui->can1Text->insertPlainText(str);
    }

    for(int i=0; i<board->can2Data.size(); i++)
    {
        QString str;
        CanDataType can = board->can2Data[i];
        str += "ID: " + QString::number(can.id, 10) + " ";
        for(int k=0; k<can.dlc; k++)
        {
            str += QString(char(can.data[k]));
        }
        str += "\n";

        ui->can2Text->moveCursor(QTextCursor::End);
        ui->can2Text->insertPlainText(str);
    }
}

void MainWindow::updateRs485Data()
{
    QString str = QString(board->rs485Data);
    ui->rs485Text->moveCursor(QTextCursor::End);
    ui->rs485Text->insertPlainText(str);
}

void MainWindow::updateState()
{
    quint8 curState = board->din;
    if(curState & 0x01) {
        ui->stateLabel_0->setText("DIN1: 闭合");
    } else {
        ui->stateLabel_0->setText("DIN1: 断开");
    }

    if(curState & (0x01<<1)) {
        ui->stateLabel_1->setText("DIN2: 闭合");
    } else {
        ui->stateLabel_1->setText("DIN2: 断开");
    }

    if(curState & (0x01<<2)) {
        ui->stateLabel_2->setText("DIN3: 闭合");
    } else {
        ui->stateLabel_2->setText("DIN3: 断开");
    }

    if(curState & (0x01<<3)) {
        ui->stateLabel_3->setText("DIN4: 闭合");
    } else {
        ui->stateLabel_3->setText("DIN4: 断开");
    }

    if(curState & (0x01<<4)) {
        ui->stateLabel_4->setText("DIN5: 闭合");
    } else {
        ui->stateLabel_4->setText("DIN5: 断开");
    }

    if(curState & (0x01<<5)) {
        ui->stateLabel_5->setText("DIN6: 闭合");
    } else {
        ui->stateLabel_5->setText("DIN6: 断开");
    }

    if(curState & (0x01<<6)) {
        ui->stateLabel_6->setText("DIN7: 闭合");
    } else {
        ui->stateLabel_6->setText("DIN7: 断开");
    }

    if(curState & (0x01<<7)) {
        ui->stateLabel_7->setText("DIN8: 闭合");
    } else {
        ui->stateLabel_7->setText("DIN8: 断开");
    }
}


void MainWindow::updateAdcChart()
{
//    series[0]->clear();
//    for(int i=0; i<100; i++)
//        series[0]->append(i, board->adcData[0][i] * 5.0 / 65535);

    static QLineEdit *lineEdit[8] = {ui->adcValue_1, ui->adcValue_2, ui->adcValue_3, ui->adcValue_4,
                                    ui->adcValue_5, ui->adcValue_6, ui->adcValue_7, ui->adcValue_8};
    QVector<QVector<quint16>> adcValue = board->adcData;
//    int chNum = board->adcData.size();
//    int sampleTimes = board->adcData[0].size();
    int chNum = adcValue.size();
    int sampleTimes = (chNum==0) ? 0 : adcValue[0].size();
    for(int k=0; k<chNum; k++)
    {
//        series[k]->clear();
//        for(int i=0; i<sampleTimes; i++)
//            series[k]->append(i, board->adcData[k][i] * 5.0 / 65535);

        quint32 sum = 0;
        for(int i=0; i<sampleTimes; i++)
             sum += adcValue[k][i];
        lineEdit[k]->setText(QString::number(5.0*sum/sampleTimes/0xFFFF) + " V");
    }

//    //正弦波测试数据
//    static int x=0;
//    x++;
//    for(int k=0; k<ADC_CH_NUM; k++)
//    {
//        series[k]->clear();
//        for(int i=0; i<100; i++)
//            this->series[k]->append(i, 2.5+2.5*qSin(2*3.14159/(10.0*x)*i));
//    }
}

void MainWindow::configLineChart()
{
    //画布
    this->graphScene = new QGraphicsScene;
    this->graphView = new QGraphicsView(graphScene);
    graphView->setWindowTitle("ADC数据");
    graphView->setRenderHint(QPainter::Antialiasing);
    graphView->setSceneRect(0, 0, 1630, 1280);
    graphScene->setBackgroundBrush(QBrush(QColor(240, 240, 240)));

    for(int i=0; i<ADC_CH_NUM; i++)
    {
        this->series[i] = new QLineSeries;
        this->chart[i] = new QChart();
        chart[i]->legend()->hide(); //隐藏图例
        chart[i]->addSeries(series[i]);
        chart[i]->createDefaultAxes();
        chart[i]->setTitle("Simple line chart example");

        if(i<4)
            chart[i]->setGeometry(300*i+10, 10, 300, 260);
        else
            chart[i]->setGeometry(300*(i-4)+10, 270, 300, 260);
    }

    for(int i=0; i<ADC_CH_NUM; i++)
    {
        //设置坐标轴初始显示范围
        this->axisX[i] = new QValueAxis();//轴变量、数据系列变量，都不能声明为局部临时变量
        this->axisY[i] = new QValueAxis();//创建X/Y轴
        axisX[i]->setRange(0, 100); //设置X/Y显示的区间
        axisY[i]->setRange(-0.3, 5.3);

        chart[i]->setAxisX(axisX[i]);
        chart[i]->setAxisY(axisY[i]); //设置chart的坐标轴
        series[i]->attachAxis(axisX[i]); //连接数据集与坐标轴。特别注意：如果不连接，那么坐标轴和数据集的尺度就不相同
        series[i]->attachAxis(axisY[i]);
    }

    for(int i=0; i<ADC_CH_NUM; i++)  {
        graphScene->addItem(chart[i]);
    }

    graphView->move(50, 50); //窗体移动到某个点
    graphView->show();
}

