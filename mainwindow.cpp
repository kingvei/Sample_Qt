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
    ui->runSystemButton->setText(tr("启动系统"));
    //ui->runSystemButton->setEnabled(false);
    ui->resetSystemButton->setText(tr("复位系统"));

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
    //this->timer->start(100); //启动定时器
    connect(this->timer, &QTimer::timeout, this, &MainWindow::updateUI);

    this->configLineChart();

    //TCP连接时UI的配置
    connect(tcpClient->tcpSocket, &QTcpSocket::connected, [=]
    {
        ui->tcpEstablishButton->setText("断开");
        ui->runSystemButton->setEnabled(true);
        ui->resetSystemButton->setEnabled(true);
        ui->tcpSendButton->setEnabled(true);

        this->timer->start(100); //启动定时器
    });

    //TCP断开时UI的配置
    connect(tcpClient->tcpSocket, &QTcpSocket::disconnected, [=]
    {
        ui->tcpEstablishButton->setText("连接");
        ui->runSystemButton->setText("启动系统");
        ui->runSystemButton->setEnabled(false);
        ui->resetSystemButton->setText("复位系统");
        ui->resetSystemButton->setEnabled(false);
        ui->tcpSendButton->setEnabled(false);

        this->timer->stop(); //关闭定时器，否则can1显示区的光标会一直移动到最后面
    });

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
    if(ui->runSystemButton->text() == tr("启动系统"))
    {
        buf[5] = 0x01;
        this->timer->start(100);
        ui->runSystemButton->setText(tr("关闭系统"));
    }
    else
    {
        buf[5] = 0x02;
        this->timer->stop(); //关闭定时器，否则can1显示区的光标会一直移动到最后面
        ui->runSystemButton->setText(tr("启动系统"));
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
    //向下位机发送命令
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

    this->updateDinState();
    this->updateAdcChart();
    this->updateCanData();
    this->updateRs485Data();

//    if(tcpClient->status == true)
//    {
//        ui->runSystemButton->setEnabled(true);
//        ui->resetSystemButton->setEnabled(true);
//    }
//    else
//    {
//        ui->runSystemButton->setText("启动系统");
//        ui->runSystemButton->setEnabled(false);
//        ui->resetSystemButton->setText("复位系统");
//        ui->resetSystemButton->setEnabled(false);
//    }
}

void MainWindow::updateCanData()
{
    bool isCan1Hex = ui->isCan1HexCheckBox->isChecked();
    QString str("");
    for(int i=0; i<board->can1Data.size(); i++)
    {
        CanDataType can = board->can1Data[i];
        if(can.ide == 0) str += "StdID 0x";
        else str += "ExtID 0x";
        str += QString::number(can.id, 16) + ": ";
        if(isCan1Hex)
        {
            for(int k=0; k<can.dlc; k++)
            {
                str.push_back((can.data[k]>>4) + '0');
                str.push_back((can.data[k]&0x0F) + '0');
                str.push_back(' ');
            }
            str += "\n";
            if(can.id == 0x10) str += '\n'; //方便调试
        }
        else
        {
            str += QString::fromLatin1((const char*)can.data, can.dlc) + "\n";
        }
    }
    ui->can1Text->moveCursor(QTextCursor::End);
    ui->can1Text->insertPlainText(str);
    ui->can1MsgNumLabel->setText("接收:" + QString::number(this->board->can1MsgNum) + "帧");
    board->can1Data.clear();

    str.clear();
    bool isCan2Hex = ui->isCan2HexCheckBox->isChecked();
    for(int i=0; i<board->can2Data.size(); i++)
    {
        CanDataType can = board->can2Data[i];
        if(can.ide == 0) str += "StdID 0x";
        else str += "ExtID 0x";
        str += QString::number(can.id, 16) + ": ";
        if(isCan2Hex)
        {
            for(int k=0; k<can.dlc; k++)
            {
                str.push_back((can.data[k]>>4)+'0');
                str.push_back((can.data[k] & 0x0F) + '0');
                str.push_back(' ');
            }
            str += "\n";
            if(can.id == 0x10) str += '\n'; //方便调试
        }
        else
        {
            str += QString::fromLatin1((const char*)can.data, can.dlc) + "\n";
        }
    }
    ui->can2Text->moveCursor(QTextCursor::End);
    ui->can2Text->insertPlainText(str);
    ui->can2MsgNumLabel->setText("接收:" + QString::number(this->board->can2MsgNum) + "帧");
    board->can2Data.clear();
}

void MainWindow::updateRs485Data()
{
    QString str = QString(board->rs485Data);
    ui->rs485Text->moveCursor(QTextCursor::End);
    ui->rs485Text->insertPlainText(str);
}

void MainWindow::updateDinState()
{
    static QLabel *label[8] = {
        ui->stateLabel_0, ui->stateLabel_1, ui->stateLabel_2, ui->stateLabel_3,
        ui->stateLabel_4, ui->stateLabel_5, ui->stateLabel_6, ui->stateLabel_7
    };

    quint8 curState = board->din;
    for(int i=0; i<8; i++)
    {
        QString str = "DIN" + QString::number(i+1) + ": ";
        if(curState & (0x01<<i)) {
            label[i]->setText(str + "闭合");
        } else {
            label[i]->setText(str + "断开");
        }
    }
}


void MainWindow::updateAdcChart()
{
    static QLineEdit *adcValueLineEdit[8] = {
        ui->adcValue_1, ui->adcValue_2, ui->adcValue_3, ui->adcValue_4,
        ui->adcValue_5, ui->adcValue_6, ui->adcValue_7, ui->adcValue_8
    };
    static QLineEdit *vinLineEdit[8] = {
        ui->vin_1, ui->vin_2, ui->vin_3, ui->vin_4,
        ui->vin_5, ui->vin_6, ui->vin_7, ui->vin_8
    };

    int chNum = board->adcData.size();
    int sampleTimes = (chNum==0) ? 0 : board->adcData[0].size();

    for(int k=0; k<chNum; k++)
    {
        series[k]->clear();
        qreal sum = 0;
        for(int i=0; i<sampleTimes; i++)
        {
            qreal temp = board->adcData[k][i] * 5.0 / 65535;
            series[k]->append(i, temp);
            sum += temp;//sum += adcValue[k][i];
        }
        //lineEdit[k]->setText(QString::number(sum/sampleTimes) + " V");
        char buf[20] = {0};
        sprintf(buf, "%d: %6.4f V", k+1, sum/sampleTimes);
        adcValueLineEdit[k]->setText(buf);

        char buf1[20] = {0};
        sprintf(buf1, "%d: %06.4f", k+1, board->calInputVoltage(k, sum/sampleTimes));
        vinLineEdit[k]->setText(buf1);
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
    graphView->setSceneRect(0, 0, 1300, 630);
    graphScene->setBackgroundBrush(QBrush(QColor(240, 240, 240)));

    for(int i=0; i<ADC_CH_NUM; i++)
    {
        this->series[i] = new QLineSeries;
        this->chart[i] = new QChart();
        chart[i]->legend()->hide(); //隐藏图例
        chart[i]->addSeries(series[i]);
        chart[i]->createDefaultAxes();
        //chart[i]->setTitle("Simple line chart example");
        switch(i)
        {
            case 0:
                chart[i]->setTitle("ADC1, 0~15V");
                break;
            case 1:
                chart[i]->setTitle("ADC2, 0~15V");
                break;
            case 2:
                chart[i]->setTitle("ADC3, 0~24V");
                break;
            case 3:
                chart[i]->setTitle("ADC4, 0~24V");
                break;
            case 4:
                chart[i]->setTitle("ADC5, 互感器20A:20mA");
                break;
            case 5:
                chart[i]->setTitle("ADC6, 0~1000V(下)");
                break;
            case 6:
                chart[i]->setTitle("ADC7, 0~1000V(上)");
                break;
            case 7:
                chart[i]->setTitle("ADC8, 分流器250A:75mV");
                break;

            default:
                break;
        }

        if(i<4)
            chart[i]->setGeometry(300*i+10, 10, 350, 300);
        else
            chart[i]->setGeometry(300*(i-4)+10, 310, 350, 300);
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
