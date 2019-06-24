#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tcpclient.h"
#include "bsp.h"
#include <QTextBrowser>
#include <QScrollBar>
#include "crc16.h"
#include <QThread>
#include <QTime>
#include <QtMath>
#include <QTimer>
#include <QLineSeries>
#include <QtCharts>
#include <QChartView>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QValueAxis>

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
    void updateIoState();

private slots:
    void on_tcpEstablishButton_clicked();

    void on_tcpSendButton_clicked();

    void on_setAllRelayButton_clicked();

    void on_runSystemButton_clicked();

    void on_relayButton_1_clicked();

    void on_relayButton_2_clicked();

    void on_relayButton_3_clicked();

    void on_relayButton_4_clicked();

    void on_relayButton_5_clicked();

    void on_relayButton_6_clicked();

    void on_relayButton_7_clicked();

    void on_relayButton_8_clicked();

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

    QLineSeries *series;
    QChart *chart;
    //QChartView *chartView;
    QGraphicsScene *graphScene;
    QGraphicsView *graphView;
    QValueAxis *axisX;
    QValueAxis *axisY;
signals:
    void sendCmdSignal(QByteArray cmd);
};

#endif // MAINWINDOW_H
