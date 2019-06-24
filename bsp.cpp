#include "bsp.h"

SampleBoard::SampleBoard()
{
    //memset(this, 0, sizeof(SampleBoard));
}

SampleBoard::~SampleBoard()
{

}

/*
 * @param: None
 * @ret: -1,消息长度小于最小帧长度
 *       -2，没有找到帧头
 *       0,找到消息
 */
int SampleBoard::decodeMsg(QByteArray msg)
{
    int startPos = msg.indexOf(0xAA);
    if(msg.size()-startPos < MIN_FRAME_SIZE)
        return -1;
    if(startPos == -1)
        return -2;
    if((quint8)msg[startPos]!=0xAA || (quint8)msg[startPos+1]!=0xBB ||
       (quint8)msg[startPos+2]!=0xCC || (quint8)msg[startPos+3]!=0xDD)
        return -2;

    quint16 tmpLen = (quint8)msg[startPos+6] + ((quint8)msg[startPos+7]<<8);
    if(msg.size()-startPos < tmpLen)
        return -1;

    int pos = 4;
    this->num = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8);
    pos += 2;
    this->len = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8);
    pos += 2;
    this->adcLen = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8);
    pos += 2;
    this->can1Len = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8);
    pos += 2;
    this->can2Len = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8);
    pos += 2;
    this->rs485Len = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8);
    pos += 2;
    //this->crc16 = (quint8)msg[pos] + ((quint8)msg[pos+1]<<8);
    pos += 2;
    this->rtc.year = (quint8)msg[pos++] + 2000;
    this->rtc.month = (quint8)msg[pos++];
    this->rtc.day = (quint8)msg[pos++];
    this->rtc.hour = (quint8)msg[pos++];
    this->rtc.minute = (quint8)msg[pos++];
    this->rtc.second = (quint8)msg[pos++];
    this->din = (quint8)msg[pos++];

    int adcPos = pos;
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
        this->adcData.push_back(channel);
    }
    pos += adcLen;

    int can1Pos = pos;
    for(int i=0; i<can1Len/15; i++)
    {
        CanDataType data;
        char *ptr = const_cast<char*>(msg.data()) + can1Pos;
        data.id = *(reinterpret_cast<quint32*>(ptr));
        memcpy(&data.ide, ptr+4, PACKET_CAN_SIZE-4);
        this->can1Data.push_back(data);
        can1Pos += PACKET_CAN_SIZE;
    }

    int can2Pos = pos;
    for(int i=0; i<can2Len/15; i++)
    {
        CanDataType data;
        char *ptr = const_cast<char*>(msg.data()) + can2Pos;
        data.id = *(reinterpret_cast<quint32*>(ptr));
        memcpy(&data.ide, ptr+4, PACKET_CAN_SIZE-4);
        this->can2Data.push_back(data);
        can2Pos += PACKET_CAN_SIZE;
    }

    int rs485Pos = pos;
    for(int i=0; i<rs485Len; i++)
    {
        rs485Data.push_back(msg[rs485Pos+i]);
    }

    return 0;
}


