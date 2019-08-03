#include "bsp.h"
#include "mainwindow.h"

SampleBoard::SampleBoard()
{
    this->clear();
}

SampleBoard::~SampleBoard()
{

}

void SampleBoard::clear()
{
    memset(this->head, 0, sizeof(this->head));
    this->crc16 = 0;
    this->num = 0;
    this->len = 0;
    this->adcLen = 0;
    this->can1Len = 0;
    this->can2Len = 0;
    this->rs485Len = 0;

    rtc.clear();
    this->din = 0;
    this->adcData.clear();
    this->can1Data.clear();
    this->can2Data.clear();
    this->rs485Data.clear();

    this->adcChartData.resize(8); //8个队列用于存储折线图数据

    this->can1MsgNum = this->can2MsgNum = this->rs485MsgNum = 0;
}

int SampleBoard::decodeMsg(QByteArray msg)
{
    int index = 0;
    while(index < msg.size())
    {
        int nextIndex = decodeFrame(msg, index);
        if(nextIndex < 0) //不再存在符合条件的帧
            break;
        index = nextIndex;
    }

    return 0;
}

/*
 * @param: None
 * @ret: -1,没有找到帧头或者帧尾
 *       -2,消息长度小于最小帧长度或者帧头定义的总长度
 *       0,找到消息
 */
int SampleBoard::decodeFrame(QByteArray &msg, int index)
{
    int startPos = msg.indexOf(0xAA, index);
    if(startPos == -1) //没有找到0xAA
        return -1;
    if(msg.size()-startPos < MIN_FRAME_SIZE)
        return -2;

    if((quint8)msg[startPos]!=0xAA || (quint8)msg[startPos+1]!=0xBB || (quint8)msg[startPos+2]!=0xCC || (quint8)msg[startPos+3]!=0xDD)
        return -1;

    quint16 tmpLen = (quint8)msg[startPos+8] + ((quint8)msg[startPos+9]<<8);
    if(msg.size()-startPos < tmpLen)
        return -2;
    if(((quint8)msg[startPos+tmpLen-2]!=0xF0) || ((quint8)msg[startPos+tmpLen-1]!=0xFC)) //没有找到帧尾
        return -1;

    //偏移地址[4][5]为CRC值。下位机预留位置，但是未计算CRC值。
    //this->crc16 = (quint8)msg[4] + ((quint8)msg[5]<<8);

    int pos = startPos + 6;
    this->num = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8); //序号
    pos += 2;
    this->len = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8); //数据包总长度
    pos += 2;
    this->adcLen = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8);
    pos += 2;
    this->can1Len = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8);
    pos += 2;
    this->can2Len = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8);
    pos += 2;
    this->rs485Len = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8);
    pos += 2;
    this->rtc.year = (quint8)msg[pos++] + 2000;
    this->rtc.month = (quint8)msg[pos++];
    this->rtc.day = (quint8)msg[pos++];
    this->rtc.weekday = (quint8)msg[pos++];
    this->rtc.hour = (quint8)msg[pos++];
    this->rtc.minute = (quint8)msg[pos++];
    this->rtc.second = (quint8)msg[pos++];
    this->din = (quint8)msg[pos++];

    //提取ADC数据
    int adcPos = pos;
    QVector<QVector<quint16>> tempAdcData;
    for(int i=0; i<8; i++)
    {
        QVector<quint16> channel;
        quint16 adcValue = 0;
        qreal avgAdcValue = 0; //记录每一帧ADC数据的平均值，用于绘制折现图
        for(int j=0; j<adcLen/8; j++)
        {
            if(j%2==0)
                adcValue = (quint8)msg[adcPos++];
            else
            {
                adcValue += (quint8)msg[adcPos++] << 8;
                channel.push_back(adcValue);
                avgAdcValue += adcValue;
            }
        }
        tempAdcData.push_back(channel);//this->adcData.push_back(channel);

        //存储折线图数据
        avgAdcValue /= (adcLen / 16.0); //8通道，2字节
        if(adcChartData[i].size() >= LINE_CHART_LENGTH)
        {
            adcChartData[i].pop_front();
            adcChartData[i].push_back(avgAdcValue);
        }
        else
            adcChartData[i].push_back(avgAdcValue);
    }
    this->adcData = tempAdcData;
    pos += adcLen;

    //提取CAN1数据
    int can1Pos = pos;
    if(3000*1024 - can1Data.size() < PACKET_CAN_SIZE) //3000KB，CAN1缓存区最大内存
        can1Data.clear();
    for(int i=0; i<can1Len/PACKET_CAN_SIZE; i++)
    {
        CanDataType data;
        char *ptr = const_cast<char*>(msg.data()) + can1Pos;
        data.id = *(reinterpret_cast<quint32*>(ptr));
        memcpy(&data.ide, ptr+4, PACKET_CAN_SIZE-4);
        this->can1Data.push_back(data);
        can1Pos += PACKET_CAN_SIZE;
    }
    pos += can1Len;
    this->can1MsgNum += can1Len / PACKET_CAN_SIZE;

    //提取CAN2数据
    int can2Pos = pos;
    if(3000*1024 - can2Data.size() < PACKET_CAN_SIZE) //3000KB，CAN2缓存区最大内存
        can2Data.clear();
    for(int i=0; i<can2Len/15; i++)
    {
        CanDataType data;
        char *ptr = const_cast<char*>(msg.data()) + can2Pos;
        data.id = *(reinterpret_cast<quint32*>(ptr));
        memcpy(&data.ide, ptr+4, PACKET_CAN_SIZE-4);
        this->can2Data.push_back(data); //this->can2Data.push_back(data);
        can2Pos += PACKET_CAN_SIZE;
    }
    pos += can2Pos;
    this->can2MsgNum += can2Len / PACKET_CAN_SIZE;

    //提取RS485数据
    int rs485Pos = pos;
    QByteArray tempRs485Data;
    for(int i=0; i<rs485Len; i++)
    {
        tempRs485Data.push_back(msg[rs485Pos+i]); //rs485Data.push_back(msg[rs485Pos+i]);
    }
    pos += rs485Len;
    this->rs485Data = tempRs485Data;

    return startPos + len;
}

/*
 * Vout = -R3 * (Va / R1 + Vin / R2)
 * 与Va(-5Vref)连接的是R1
 * 与Vin连接的是R2
 * 与Vout连接的是R3
 * Vout是ADC测出的电压
 */
qreal SampleBoard::calculateAin(int chNum, qreal vout)
{
    qreal res = 0;

    switch(chNum)
    {
    case 0: /* 第1路0~15V */
        {
            qreal R1 = 10.0, R2 = 33.0, R3 = 10.0;
            qreal va = -5.0;
            qreal vin = -R2 * (va / R1 + vout / R3);
            res = vin;
        }
        break;
    case 1: /* 第2路0~15V */
        {
            qreal R1 = 10.0, R2 = 33.0, R3 = 10.0;
            qreal va = -5.0;
            qreal vin = -R2 * (va / R1 + vout / R3);
            res = vin;
        }
        break;
    case 2: /* 第1路0~24V */
        {
            qreal R1 = 10.0, R2 = 51.0, R3 = 10.0;
            qreal va = -5.0;
            qreal vin = -R2 * (va / R1 + vout / R3);
            res = vin;
        }
        break;
    case 3: /* 第2路0~24V */
        {
            qreal R1 = 10.0, R2 = 51.0, R3 = 10.0;
            qreal va = -5.0;
            qreal vin = -R2 * (va / R1 + vout / R3);
            res = vin;
        }
        break;
    case 4: /* 互感器20A:20mA */
        {
            qreal R1 = 10.0, R2 = 4.12, R3 = 10.0;
            qreal va = -5.0;
            qreal vin2 = -R2 * (va / R1 + vout / R3);
            qreal vin = vin2 / ((91 + 10) / 10);
            qreal RL = 10.0; //unit: Ohm
            res = vin / RL / 0.02 * 20.0; //一次侧电流大小，单位：安培
        }
        break;
    case 5: /* 第2路0~1000V */
        {
            qreal gain = (1.0 / 3) * (215.0 / 100);
            qreal vin = vout / gain; //ISO124隔离侧输入电压
            res = vin / 6.8 * (6.8 + 500 + 500); //高压
        }
        break;
    case 6: /* 第1路0~1000V */
        {
            qreal gain = (1.0 / 3) * (215.0 / 100);
            qreal vin = vout / gain; //ISO124隔离侧输入电压
            res = vin / 6.8 * (6.8 + 500 + 500); //高压
        }
        break;
    case 7: /* 分流器250A:75mV */
        {
            qreal gain = 8.2 * (49.4 / 4.99 + 1); //AD8221增益电阻4.99k
            qreal R1 = 9.09, R2 = 13.0, R3 = 9.09;
            qreal va = -5.0;
            qreal vin = -R2 * (va / R1 + vout / R3);
            res = vin / gain / 0.075 * 300; //通过的电流
            //res = vin / gain / 0.075 * 250; //通过的电流
        }
        break;

    default:
        break;
    }

    return res;
}

/*
 * Vout = -R3 * (Va / R1 + Vin / R2)
 * 与Va(-5Vref)连接的是R1
 * 与Vin连接的是R2
 * 与Vout连接的是R3
 * Vout是ADC测出的电压
 */
qreal SampleBoard::calculateCalibAin(int chNum, qreal vout)
{
    int board_num = 1; //表示适用的板子
    qreal res = 0;

    if(board_num == 1)
    {
        switch(chNum)
        {
        case 0: /* 第1路0~15V */
            {
                qreal R1 = 10.0, R2 = 33.0, R3 = 10.0;
                qreal va = -5.0;
                qreal vin = -R2 * (va / R1 + vout / R3);
                res = 1.0128 * vin - 0.0339; //校准
            }
            break;
        case 1: /* 第2路0~15V */
            {
                qreal R1 = 10.0, R2 = 33.0, R3 = 10.0;
                qreal va = -5.0;
                qreal vin = -R2 * (va / R1 + vout / R3);
                res = 1.0066 * vin - 0.0003; //校准
            }
            break;
        case 2: /* 第1路0~24V */
            {
                qreal R1 = 10.0, R2 = 51.0, R3 = 10.0;
                qreal va = -5.0;
                qreal vin = -R2 * (va / R1 + vout / R3);
                res = 1.0022 * vin - 0.0341; //校准
            }
            break;
        case 3: /* 第2路0~24V */
            {
                qreal R1 = 10.0, R2 = 51.0, R3 = 10.0;
                qreal va = -5.0;
                qreal vin = -R2 * (va / R1 + vout / R3);
                res = 1.0069 * vin - 0.039; //校准
            }
            break;
        case 4: /* 互感器20A:20mA */
            {
                qreal R1 = 10.0, R2 = 4.12, R3 = 10.0;
                qreal va = -5.0;
                qreal vin2 = -R2 * (va / R1 + vout / R3);
                qreal vin = vin2 / ((91 + 10) / 10);
                qreal RL = 10.0; //unit: Ohm
                res = vin / RL / 0.02 * 20.0; //一次侧电流大小，单位：安培
                res = 0.9954 * res + 0.1656;
            }
            break;
        case 5: /* 第2路0~1000V */
            {
                qreal gain = (1.0 / 3) * (215.0 / 100);
                qreal vin = vout / gain; //ISO124隔离侧输入电压
                res = vin / 6.8 * (6.8 + 500 + 500); //高压
                //分段拟合
                if(res < 500)
                    res = 1.0046452 * res + 0.2823941;
                else
                    res = 1.0044543 * res + 0.3224953;
            }
            break;
        case 6: /* 第1路0~1000V */
            {
                qreal gain = (1.0 / 3) * (215.0 / 100);
                qreal vin = vout / gain; //ISO124隔离侧输入电压
                res = vin / 6.8 * (6.8 + 500 + 500); //高压
                //分段拟合
                if(res < 500)
                    res = 1.0076824 * res + 0.2353542;
                else
                    res = 1.0076957 * res + 0.1922761;
            }
            break;
        case 7: /* 分流器250A:75mV */
            {
                qreal gain = 8.2 * (49.4 / 4.99 + 1); //AD8221增益电阻4.99k
                qreal R1 = 9.09, R2 = 13.0, R3 = 9.09;
                qreal va = -5.0;
                qreal vin = -R2 * (va / R1 + vout / R3);
                res = vin / gain / 0.075 * 300; //通过的电流 //res = vin / gain / 0.075 * 250; //通过的电流
                res = 1.0066636 * res - 0.7772165;
            }
            break;

        default:
            break;
        }
    }
    else if(board_num == 2)
    {
        switch(chNum)
        {
        case 0: /* 第1路0~15V */
            {
                qreal R1 = 10.0, R2 = 33.0, R3 = 10.0;
                qreal va = -5.0;
                qreal vin = -R2 * (va / R1 + vout / R3);
                res = 1.0113 * vin - 0.0562; //校准
            }
            break;
        case 1: /* 第2路0~15V */
            {
                qreal R1 = 10.0, R2 = 33.0, R3 = 10.0;
                qreal va = -5.0;
                qreal vin = -R2 * (va / R1 + vout / R3);
                res = 0.9999 * vin - 0.0154; //校准
            }
            break;
        case 2: /* 第1路0~24V */
            {
                qreal R1 = 10.0, R2 = 51.0, R3 = 10.0;
                qreal va = -5.0;
                qreal vin = -R2 * (va / R1 + vout / R3);
                res = 1.0035 * vin - 0.0404; //校准
            }
            break;
        case 3: /* 第2路0~24V */
            {
                qreal R1 = 10.0, R2 = 51.0, R3 = 10.0;
                qreal va = -5.0;
                qreal vin = -R2 * (va / R1 + vout / R3);
                res = 1.0088 * vin - 0.0244; //校准
            }
            break;
        case 4: /* 互感器20A:20mA */
            {
                qreal R1 = 10.0, R2 = 4.12, R3 = 10.0;
                qreal va = -5.0;
                qreal vin2 = -R2 * (va / R1 + vout / R3);
                qreal vin = vin2 / ((91 + 10) / 10);
                qreal RL = 10.0; //unit: Ohm
                res = vin / RL / 0.02 * 20.0; //一次侧电流大小，单位：安培
                res = 0.9839 * res + 0.0653;
            }
            break;
        case 5: /* 第2路0~1000V */
            {
                qreal gain = (1.0 / 3) * (215.0 / 100);
                qreal vin = vout / gain; //ISO124隔离侧输入电压
                res = vin / 6.8 * (6.8 + 500 + 500); //高压
                //分段拟合
                if(res < 500)
                    res = 1.0051754 * res + 0.0670882;
                else
                    res = 1.0074100 * res - 1.1412713;
            }
            break;
        case 6: /* 第1路0~1000V */
            {
                qreal gain = (1.0 / 3) * (215.0 / 100);
                qreal vin = vout / gain; //ISO124隔离侧输入电压
                res = vin / 6.8 * (6.8 + 500 + 500); //高压
                //分段拟合
                if(res < 500)
                    res = 1.0039345 * res - 0.0147680 ;
                else
                    res = 1.0055404 * res - 0.9493309;
            }
            break;
        case 7: /* 分流器250A:75mV */
            {
                qreal gain = 8.2 * (49.4 / 4.99 + 1); //AD8221增益电阻4.99k
                qreal R1 = 9.09, R2 = 13.0, R3 = 9.09;
                qreal va = -5.0;
                qreal vin = -R2 * (va / R1 + vout / R3);
                //校准
                res = vin / gain / 0.075 * 300; //通过的电流 //res = vin / gain / 0.075 * 250; //通过的电流
                res = 0.9984894 * res - 0.7751783;
            }
            break;

        default:
            break;
        }
    }

    return res;
}
