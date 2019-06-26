#include "bsp.h"

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
}

/*
 * @param: None
 * @ret: 1,没有找到帧头或者帧尾
 *       2,消息长度小于最小帧长度
 *       0,找到消息
 */
int SampleBoard::decodeMsg(QByteArray msg)
{
    int startPos = msg.indexOf(0xAA);
    if(startPos == -1) //没有找到0xAA
        return 1;
    if(msg.size()-startPos < MIN_FRAME_SIZE)
        return 2;

    if((quint8)msg[startPos]!=0xAA || (quint8)msg[startPos+1]!=0xBB || (quint8)msg[startPos+2]!=0xCC || (quint8)msg[startPos+3]!=0xDD)
        return 1;

    quint16 tmpLen = (quint8)msg[startPos+8] + ((quint8)msg[startPos+9]<<8);
    if(msg.size()-startPos < tmpLen)
        return 2;
    if(((quint8)msg[startPos+tmpLen-2]!=0xF0) || ((quint8)msg[startPos+tmpLen-1]!=0xFC)) //没有找到帧尾
        return 1;

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
        for(int j=0; j<adcLen/8; j++)
        {
            if(j%2==0)
                adcValue = (quint8)msg[adcPos++];
            else
            {
                adcValue += (quint8)msg[adcPos++] << 8;
                channel.push_back(adcValue);
            }
        }
        tempAdcData.push_back(channel);//this->adcData.push_back(channel);
    }
    this->adcData = tempAdcData;
    pos += adcLen;

    //提取CAN1数据
    int can1Pos = pos;
    QVector<CanDataType> tempCan1Data;
    for(int i=0; i<can1Len/PACKET_CAN_SIZE; i++)
    {
        CanDataType data;
        char *ptr = const_cast<char*>(msg.data()) + can1Pos;
        data.id = *(reinterpret_cast<quint32*>(ptr));
        memcpy(&data.ide, ptr+4, PACKET_CAN_SIZE-4);
        tempCan1Data.push_back(data); //this->can1Data.push_back(data);
        can1Pos += PACKET_CAN_SIZE;
    }
    this->can1Data = tempCan1Data;

    //提取CAN2数据
    int can2Pos = pos;
    QVector<CanDataType> tempCan2Data;
    for(int i=0; i<can2Len/15; i++)
    {
        CanDataType data;
        char *ptr = const_cast<char*>(msg.data()) + can2Pos;
        data.id = *(reinterpret_cast<quint32*>(ptr));
        memcpy(&data.ide, ptr+4, PACKET_CAN_SIZE-4);
        tempCan2Data.push_back(data); //this->can2Data.push_back(data);
        can2Pos += PACKET_CAN_SIZE;
    }
    this->can2Data = tempCan2Data;

    //提取RS485数据
    int rs485Pos = pos;
    QByteArray tempRs485Data;
    for(int i=0; i<rs485Len; i++)
    {
        tempRs485Data.push_back(msg[rs485Pos+i]); //rs485Data.push_back(msg[rs485Pos+i]);
    }
    this->rs485Data = tempRs485Data;

    return 0;
}


