#ifndef BSP_H
#define BSP_H

#include <QByteArray>
#include <QVector>

#define MIN_FRAME_SIZE  18
#define PACKET_CAN_SIZE 15
#define ADC_CH_NUM      8
struct RTC
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    RTC(){year = month = day = hour = minute = second = 0;}
};

struct CanDataType
{
    quint32 id;
    quint8 ide;
    quint8 rtr;
    quint8 dlc;
    quint8 data[8];

    CanDataType(){memset(this, 0, sizeof(*this));}
};

class SampleBoard
{
public:
    SampleBoard();
    ~SampleBoard();

    int decodeMsg(QByteArray msg);
    int setSingleRelay();
    int setAllRelay();
    int turnOnBoard(void);
    int turnOffBoard(void);

    quint8 head[4];
    quint16 num;
    quint16 len;
    quint16 adcLen;
    quint16 can1Len;
    quint16 can2Len;
    quint16 rs485Len;
    quint16 crc16;

    RTC rtc;
    quint8 din;
    QVector<QVector<quint16>> adcData;
    QVector<CanDataType> can1Data;
    QVector<CanDataType> can2Data;
    QByteArray rs485Data;
};

#endif // BSP_H
