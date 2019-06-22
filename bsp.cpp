#include "bsp.h"

SampleBoard::SampleBoard()
{
    memset(this, 0, sizeof(SampleBoard));
    this->adcData.resize(8);
}

SampleBoard::~SampleBoard()
{

}

/*
 * @param: None
 * @ret: -1,没有找到完整的消息
 *       0,找到消息
 */
int SampleBoard::decodeMsg(const QByteArray &msg)
{
    unsigned char head[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    int startPos = msg.indexOf((char*)head);
    if(startPos == -1 || (msg.size()-startPos < MIN_FRAME_SIZE))
        return -1;

    quint16 tmpLen = msg[startPos+6] + (msg[startPos+7]<<8);
    if(msg.size()-startPos < tmpLen)
        return -1;

    int pos = 4;
    this->num = msg[pos] + (msg[pos+1]<<8);
    pos += 2;
    this->len = msg[pos] + (msg[pos+1]<<8);
    pos += 2;
    this->adcLen = msg[pos] + (msg[pos+1]<<8);
    pos += 2;
    this->can1Len = msg[pos] + (msg[pos+1]<<8);
    pos += 2;
    this->can2Len = msg[pos] + (msg[pos+1]<<8);
    pos += 2;
    this->rs485Len = msg[pos] + (msg[pos+1]<<8);
    pos += 2;
    //this->crc16 = msg[pos] + (msg[pos+1]<<8);
    pos += 2;
    this->rtc.year = msg[pos++];
    this->rtc.month = msg[pos++];
    this->rtc.date = msg[pos++];
    this->rtc.hours = msg[pos++];
    this->rtc.minutes = msg[pos++];
    this->rtc.seconds = msg[pos++];
    this->din = msg[pos++];

    int adcPos = pos;
    for(int i=0; i<8; i++)
    {
        QVector<quint16> channel;
        quint16 adcValue = 0;
        for(int j=0; j<adcLen/8; j++)
        {
            if(j%2==0)
                adcValue = msg[adcPos++];
            else
            {
                adcValue += msg[adcPos++] << 8;
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
        memcpy(&data.ide, ptr+4, 15-4);
        this->can1Data.push_back(data);
        can1Pos += 15;
    }

    int can2Pos = pos;
    for(int i=0; i<can2Len/15; i++)
    {
        CanDataType data;
        char *ptr = const_cast<char*>(msg.data()) + can2Pos;
        data.id = *(reinterpret_cast<quint32*>(ptr));
        memcpy(&data.ide, ptr+4, 15-4);
        this->can2Data.push_back(data);
        can2Pos += 15;
    }

    int rs485Pos = pos;
    for(int i=0; i<rs485Len; i++)
    {
        rs485Data.push_back(msg[rs485Pos+i]);
    }

    return 0;
}


