#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tcpclient.h"
#include "bsp.h"
#include <QTextBrowser>
#include <QScrollBar>
#include "crc16.h"
#include <QThread>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QtMath>
#include <QTimer>
#include <QLineSeries>
#include <QtCharts>
#include <QChartView>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QValueAxis>
#include <QLabel>
#include <QPixmap>
#include <QVector>
#include <QScrollArea>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void processTcpReceivedMsg(QByteArray);
    inline void setSingleRelay(quint8);
    void dealClose();
    void configLineChart();

    void updateUI();
    void updateAdcChart();
    void updateCanData();
    void updateRs485Data();
    void updateDinState();

private slots:
    void on_tcpEstablishButton_clicked();

    void on_tcpSendButton_clicked();

    void on_setAllRelayButton_clicked();

    void on_runSystemButton_clicked();

    void on_relayButton_0_clicked();

    void on_relayButton_1_clicked();

    void on_relayButton_2_clicked();

    void on_relayButton_3_clicked();

    void on_relayButton_4_clicked();

    void on_relayButton_5_clicked();

    void on_relayButton_6_clicked();

    void on_relayButton_7_clicked();

    void on_resetSystemButton_clicked();

    void on_calibrateTimeButton_clicked();

private:
    Ui::MainWindow *ui;
    QString srvIP;
    QString srvPort;
    QString cltData; //客户端要发送的数据
    QString srvData; //客户端接收的数据
    SampleBoard *board;
    TcpClient *tcpClient;
    QThread *thread;
    QTimer *timer;

//    QVector<QLineSeries*> series;
//    QVector<QChart*> chart;
//    QVector<QValueAxis*> axisX;
//    QVector<QValueAxis*> axisY;
    QLineSeries * series[ADC_CH_NUM];
    QChart * chart[ADC_CH_NUM];
    QValueAxis *axisX[ADC_CH_NUM];
    QValueAxis *axisY[ADC_CH_NUM];

    QGraphicsScene *graphScene;
    QGraphicsView *graphView;

//    QPixmap ledOnImg;
//    QPixmap ledOffImg;
signals:
    void sendCmdSignal(QByteArray cmd);
};

#endif // MAINWINDOW_H
