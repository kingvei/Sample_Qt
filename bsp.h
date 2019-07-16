#ifndef BSP_H
#define BSP_H

#include <QByteArray>
#include <QVector>
#include <QObject>

#define MIN_FRAME_SIZE  18
#define PACKET_CAN_SIZE 15
#define ADC_CH_NUM      8
struct RTC
{
    int year;
    int month;
    int day;
    int weekday; //Monday==1
    int hour;
    int minute;
    int second;

    RTC(){this->clear();}
    void clear(){year = month = day = weekday = hour = minute = second = 0;}
};

struct CanDataType
{
    quint32 id;
    quint8 ide;
    quint8 rtr;
    quint8 dlc;
    quint8 data[8];

    CanDataType(){this->clear();}
    void clear(){
        id = ide = rtr = dlc = 0;
        memset(data, 0, sizeof(data));
    }
};

class SampleBoard// : public QObject
{
    //Q_OBJECT

public:
    SampleBoard();
    ~SampleBoard();

    int decodeMsg(QByteArray msg);
    qreal calInputVoltage(int chNum, qreal vout);
    void clear();

    quint8 head[4];
    quint16 crc16;
    quint16 num;
    quint16 len;
    quint16 adcLen;
    quint16 can1Len;
    quint16 can2Len;
    quint16 rs485Len;

    RTC rtc;
    quint8 din;
    QVector<QVector<quint16>> adcData;
    QVector<CanDataType> can1Data;
    QVector<CanDataType> can2Data;
    QByteArray rs485Data;

    int can1MsgNum;
    int can2MsgNum;
    int rs485MsgNum;
//signals:
};

#endif // BSP_H
