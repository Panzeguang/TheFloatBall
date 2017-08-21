#include "MAIN.H"

unsigned char RX_DATA[8];

unsigned char RX_OK_FLAG;
unsigned char RX_OVER_FLAG;
unsigned char RX_OVER_TIME;
unsigned char RX_NUM = 0;
unsigned char MOTOR_RUN_FLAG = 0;
unsigned char SENSOR_FLAG = 0;
unsigned char MOTOR_STAUS = 0;

unsigned int cwStep = 0;
unsigned int ccwStep = 0;
unsigned char reverseFlag = 0;
unsigned char lastMotion = CCW; //1: CW, 0:CCW

static unsigned char MsgPacket = 0;
static unsigned char uartBufProtect = 0; 
static unsigned char uart0RxBuf[20];
static unsigned char RxNum;

UARTBUF_TXD	uart0TxdBuf;	// 发送数据缓冲区


void UART_INIT()
{
    CLI;
    UCSR0A &= 0X00;
    UCSR0B |= 0X98;     //接收和接收中断使能
    UCSR0C |= 0X06;
    UBRR0H = 0;
    UBRR0L = UBRL;
}

int Uart0RxDataDeal(const char * cMsg)
{
    int tempStep = 0;
    int ret = 0;
    
    if(strcmp(cMsg,"FLUOMotorCW") == 0)
    {
        if(GetSensorState() == TOP)
        {
            uart0SendData("FLUOMotorCWOK\r\n",15);
            MOTOR_RUN_FLAG = 1;
        }
        else
            MOTOR_RUN_FLAG = 1;
        ret = 1;
    }
    else if(strcmp(cMsg,"FLUOMotorCCW") == 0)  
    {
        if(GetSensorState() == BOTTOM)
        {
            uart0SendData("FLUOMotorCCWOK\r\n",16);
            MOTOR_RUN_FLAG = 0;
        }
        else
            MOTOR_RUN_FLAG = 4;
        ret = 1;
    }
    else if(strcmp(cMsg,"FLUOMotorInit") == 0)
    {
        if(GetSensorState() == TOP)
        {
            //do nothing
            //MOTOR_RUN_FLAG = 11;
            uart0SendData("At Top Position\r\n",17);
        }
        else
        {
            //MOTOR_RUN_FLAG = 5;
            MOTOR_RUN_FLAG = 11;
            MOTOR_STAUS = MOTORINIT;
            
            if(CW == lastMotion) 
            {
                reverseFlag = 1;
            }
        }
        uart0SendData("FLUOMotorInitRx\r\n",17);
        ret = 1;
    }
    else if(strcmp(cMsg,"FLUOMotorRun") == 0)
    {
        if(GetSensorState() == TOP)
        {
            MOTOR_RUN_FLAG = 3;
        }
        else
        {
            MOTOR_RUN_FLAG = 5;
            MOTOR_STAUS = MOTORRUN;
        }
        uart0SendData("FLUOMotorRunRx\r\n",16);
        ret = 1;
    }
    else if(strcmp(cMsg,"S") == 0)
    {
        MOTOR_RUN_FLAG = 2;
        ret = 1;
    }
    else if(!(strncmp(cMsg, "PumpIN", 6)))
    {
        tempStep = atoi(cMsg+6);
        MOTOR_RUN_FLAG = 12;
        if((0 <= tempStep) && (tempStep <= 2000))
        {
            if(CCW == lastMotion)
            {
                reverseFlag = 1;
            }
            else
            {
                reverseFlag = 0;
            }
            cwStep = tempStep;
            //uart0SendData("PumpIN\r\n",8);
        }
        ret = 1;
    }
    else if(!(strncmp(cMsg, "PumpOUT", 7)))
    {
        tempStep = atoi(cMsg+7);
        MOTOR_RUN_FLAG = 13;
        if((0 <= tempStep) && (tempStep <= 2000))
        {
            if(CW == lastMotion)
            {
                reverseFlag = 1;
            }
            else
            {
                reverseFlag = 0;
            }
            ccwStep = tempStep;
            //uart0SendData("PumpOUT\r\n",9);
        }
        ret = 1;
    }
    else
    {
        ret =0;
    }
    return ret;
}


static void UartReceiveData(unsigned char cMsg)
{
    switch(MsgPacket)
    {
    case 0:
        if(cMsg == STX)
        {
            MsgPacket = 1;
        }
        break;
    case 1:
        if(cMsg == ETX)
        {
            uart0RxBuf[RxNum] = 0;
            RxNum = 0;
            MsgPacket = 2;
        }
        else if(cMsg == STX)
        {
            MsgPacket = 1;
            RxNum = 0;
        }
        else
        {
            uart0RxBuf[RxNum] = cMsg;
            RxNum++;
        }
        break;
    default:break;          
    }
}


unsigned char uart0SendData(const char * data, unsigned short len)
{
    // 指定长度数据写入缓冲区
    unsigned short i;
    unsigned int pTop, pEnd, p;
    uartBufProtect = 1;
    pTop = uart0TxdBuf.pHead;
    pEnd = uart0TxdBuf.pEnd; 
    for(i = 0; i < len; i++)
    {
        p = pEnd;
        pEnd ++;
        if(pEnd == UARTBUF_TXD_LEN)
        {
            pEnd = 0;
        }
        if(pEnd != pTop)
        {
            uart0TxdBuf.buffer[p] = (* (data+i));
        }
        else
        {
            pEnd = p;
            break;
        }
    }
    uart0TxdBuf.pEnd = pEnd;
    uartBufProtect = 0;
    return 1;
}


void USART0_TRANSMIT(void)
{
    unsigned char pTop,pEnd;
    unsigned char cTxDat;
    if(uartBufProtect)  return;
    switch(MsgPacket)
    {
    case 0: 
        pTop = uart0TxdBuf.pHead;
        pEnd = uart0TxdBuf.pEnd;
        if(pTop != pEnd)
        {
            if(UCSR0A & 0x20)
            {
                cTxDat = uart0TxdBuf.buffer[pTop];
                UDR0 = cTxDat;
                pTop++;
                if(pTop == UARTBUF_TXD_LEN)
                    pTop = 0;
                uart0TxdBuf.pHead = pTop;
            }
        }         
        break;
    case 1: break;
    case 2:
        Uart0RxDataDeal((char const *)uart0RxBuf);
        MsgPacket = 0;
        break;
    default:break;            
    }
}



#pragma vector = USART_RX_vect
__interrupt void UART_RX_ISR()
{  
    unsigned char cMsg;
    cMsg = UDR0;
    UartReceiveData(cMsg);
}

/*************************************************************
Function Name: num2str
Description: converte interger to string
Input Para: num - interger to to converted
Output Para: strDest - the pointer to converted string
Return:  -1 - strDest is NULL pointer
others - number of digits of input interger
*************************************************************/
int num2str(int num, char *strDest)
{
    if(strDest == NULL)
    {
        return -1;
    }
    
    char *strTemp = strDest;
    char ch;
    char str[20];
    int Num = num;
    int index = 0;
    int length = 0;
    
    if( num < 0)
    {
        Num = -num;
        *strTemp++ = '-';
        length++;
    }
    
    while(0 < Num)
    {
        ch = Num % 10;
        *(str+index) = ch + '0';
        Num /= 10;
        index++;
        length++;
    }
    index--;
    
    //逆序输出
    for(; 0<=index; index--)
    {
        *strTemp++ = str[index];
    }
    //*strTemp++ = '\0';
    
    return length;
}
