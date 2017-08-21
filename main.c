#include "MAIN.H"


extern void TIMER_INIT(void);
void dealRxData(void);
void dealTxData(void);

#define BUFFER_SIZE    256
typedef struct SerialBuffer{
    unsigned int front;
    unsigned int rear;
    unsigned char data[BUFFER_SIZE];
}SerialBuffer_t;
SerialBuffer_t rxRingFifo; 
SerialBuffer_t txRingFifo;

char numToStr[10];
char tempStep[10];
unsigned char MOTOR_RUN_FLAG = 0;
bool bADCDone = false;
bool uartRxProcessEnable = false;

#define BELOW_WARNING_LINE          1
#define ABOVE_WARNING_BELOW_DEAD    2
#define ABOVE_DEAD_LINE             3


extern volatile bool timerTriggerFlag;
extern unsigned int timer1Cnt;
       unsigned int BuzzerCnt;
        bool BuzzerFlag;

#define STX 0X02
#define ETX 0X03
unsigned char tempRec[20];

typedef enum PUMP_MOTION{
    STOP = 1,
    IN,
    OUT,
    ZERO,
    CLEAN
}PUMP_MOTION_e;
PUMP_MOTION_e pumpState = STOP;

void PORT_INIT(void)
{  

    DDRD |= (1 << 3);   //PD3  LD1as output
     DDRD |= (1 << 5);   //PD5  LD2as output
      DDRD |= (1 << 6);    //PD6  LD3as output
       DDRD |= (1 << 7);     //PD7  LD4as output
    
        DDRB |= (1 << 5);     //PB5  Buzzer as output
       
      // DDRB &= 0xDF;      //PB5  Buzzer as input 屏蔽蜂鸣器
    
    DDRC &= 0xFD;    //PC1 as input
     DDRC &= 0xFB;    //PC2 as input
      DDRC &= 0xEF;    //PC4 as input
      
      
        PORTB = 0xFF;    //常态是高电平
        
        PORTC = 0xFF;    //常态是高电平
        PORTD = 0xFF;    //常态是高电平 ，lEd是灭的状态 ，低电平有效
        

//    DDRD |= (1 << 5) | (1 << 6) | (1 << 7);
//    DDRD &= 0XE3;
//    PORTD |= (1 << 2) | (1 << 3) | (1 << 4);    
}


void UART_INIT()
{
    UCSR0B = 0x00;
    UCSR0A = 0x00;
    UCSR0C |= 0x06;    
    UBRR0H = 0;
    UBRR0L = UBRL;
    //UCSR0B |= 0X98;     //接收和接收中断使能
    //UCSR0B = 0x18; 
    UCSR0B = 0x98;  //使能发送和接收，使能接收中断
}

void SendDataInfoFifo(char* data)
{
    if(NULL != data)
    {
        while('\0' != *data)
        {
            txRingFifo.data[txRingFifo.front] = *data++;
            txRingFifo.front = (txRingFifo.front+1) % BUFFER_SIZE;
        }
    }
}

//反转字符串  
char *reverse(char *s)
{  
    char temp;  
    char *p = s;    //p指向s的头部  
    char *q = s;    //q指向s的尾部  
    while(*q)  
        ++q;  
    q--;  
    
    //交换移动指针，直到p和q交叉  
    while(q > p)  
    {  
        temp = *p;  
        *p++ = *q;  
        *q-- = temp;  
    }
    return s;  
}

char *my_itoa(int n)
{  
    int i = 0;  
    //static char numToStr[5];      //必须为static变量，或者是全局变量
    
    do      //从个位开始变为字符，直到最高位，最后应该反转  
    {  
        numToStr[i++] = (n%10) + '0';
        n = n/10;  
    }while(n > 0); 
    
    numToStr[i] = '\0';    //最后加上字符串结束符  
    return reverse(numToStr);    
}

void delay_ms(unsigned int num)
{
    int i,j;
    for(i=0;i<num;i++)
    {
        for(j=0; j<2000; j++)
        {
            asm("NOP");
        }
    }
}

unsigned char sss[] = "$3301\r\n";
void main( void )
{
    txRingFifo.rear = 0;
    txRingFifo.front = 0;
    
    rxRingFifo.rear = 0;
    rxRingFifo.front = 0;
    
   //  unsigned char  waterLevel_1 = 0;  //0是正常信号1是异常信号
    //  unsigned char  oldWaterLevel_1 = 0;
   // bool waterLevelChangedFlag_1 = false;  //废液警告线标记位
   // int scanCnt_1 = 0;                        //此三个标记 留着备用 废液可能要检测两个液面点 
    
    unsigned char  waterLevel_2 = 0;          //当前的实时状态
    unsigned char  oldWaterLevel_2 = 1;       //稳定时候的状态 1正常  0 异常
    bool waterLevelChangedFlag_2 = false;  //废液强制线标记位
    int scanCnt_2 = 0;
    
    unsigned char  waterLevel_3 = 0;
    unsigned char  oldWaterLevel_3 = 1;       //1正常  0 异常
    bool waterLevelChangedFlag_3 = false;  //稀释液强制线标记位
     int scanCnt_3 = 0;
     
    unsigned char  waterLevel_4 = 0;
    unsigned char  oldWaterLevel_4 = 1;      //1正常  0 异常
    bool waterLevelChangedFlag_4 = false;  //清洗液强制线标记位
     int scanCnt_4 = 0;
     int reportTime = 0;
    
     
     BuzzerCnt=0;
    
    //int uartSendTimer = 0;
    
    CLI;    //关全局中断
    PORT_INIT();
    UART_INIT();
    TIMER_INIT();
    SEI;    //开全局中断
    
   // LED = ~LED;
    
    SendDataInfoFifo("Hello,Atmega88\r\n");
     //int k=0;
        
    while(1)
    {             
        
        if(100 <= timer1Cnt)  //连接询问机制表示浮球板已经连接1s 发送一次
        {
            reportTime++  ;
            
         //  if (reportTime>ReportTime)
           //      reportTime=0; 
            
            timer1Cnt = 0;
             //1s查询一次
            //SendDataInfoFifo("$3300\r\n");
            //dealTxData(); 此处加上这行代码通信按字节发送异常情况
        }
         
         if(timerTriggerFlag) //不间断监测液面信号，10ms查询一次
        {
            timerTriggerFlag = false;
            
   /* {    if(waterLevel_1 != PINC_Bit1)
           {
               waterLevel_1 = PINC_Bit1;
                scanCnt_1 = 0;   
                waterLevelChangedFlag_1 = true;
            }

           if(waterLevelChangedFlag_1)
           {
                if(++scanCnt_1 == scanCnt_time)
                {
                    waterLevelChangedFlag_1 = false;
            oldWaterLevel_1=waterLevel_1;
                    scanCnt_1 = 0;
                }
           }
     } 
      
            if((!waterLevel_1)&&(waterLevelChangedFlag_1 == false))
              {   
                  PORTD |= 0xF8;
                PORTD &= 0xDF;  
               //   PIND_Bit5=0;
              SendDataInfoFifo("$3302\r\n");  //废液异常
              dealTxData();             
             }  
          else if((waterLevel_2)&&(!waterLevelChangedFlag_2))
              {       
                  //PORTD |= 0x08;  //关闭LD1 PD3
                    PORTD |= 0x20;  //关闭LD2     PD5
               //   PIND_Bit5=0;
              SendDataInfoFifo("$!3302\r\n");  //废液异常
              dealTxData();             
             }
             
            
        */
        /*    获取废液状态         */
          if(waterLevel_2 != PINC_Bit1)
           {
               waterLevel_2 = PINC_Bit1;
                             scanCnt_2 = 0;   
                waterLevelChangedFlag_2 = true;
            }

           if(waterLevelChangedFlag_2)
           
                if(++scanCnt_2 == scanCnt_time)
                {
                    waterLevelChangedFlag_2 = false;
                    oldWaterLevel_2 = waterLevel_2;
                    scanCnt_2 = 0;
                }   
          
            /*    获取稀释液状态         */      
           if(waterLevel_3 != PINC_Bit2)
            {
                waterLevel_3 = PINC_Bit2;
                scanCnt_3 = 0;   
                waterLevelChangedFlag_3 = true;
            }

           if(waterLevelChangedFlag_3)
           
                if(++scanCnt_3 == scanCnt_time)
                {
                    waterLevelChangedFlag_3 = false;
                    oldWaterLevel_3=waterLevel_3;
                    scanCnt_3 = 0;
                }
           
            /*    获取清洗液状态         */ 
           if(waterLevel_4 != PINC_Bit4)
           {
               waterLevel_4 = PINC_Bit4;
                scanCnt_4 = 0;   
                waterLevelChangedFlag_4 = true;
            }

           if(waterLevelChangedFlag_4)
          
                if(++scanCnt_4 == scanCnt_time)
                {
                    waterLevelChangedFlag_4 = false;
                    oldWaterLevel_4=waterLevel_4;
                    scanCnt_4 = 0;
                }
            
    } 
    
    
    
      /*    判断稀废液稳定的状态         */        
            if(!oldWaterLevel_2)
              {       
                  PORTD |= 0x08;  //关闭LD1 PD3
                    PORTD &= 0xDF;  //打开LD2     PD5
                    
                 /*   if(reportTime>=ReportTime)
                    {
                    SendDataInfoFifo("$3302\r\n");  //废液异常
                     dealTxData(); 
                    }
                    */
                    
               }
             
             
             else if(oldWaterLevel_2)
              {       
                    PORTD |= 0x20;  //关闭LD2     PD5
              //SendDataInfoFifo("$!3302\r\n");  //废液恢复正常
              //dealTxData();             
               }
                 
               /*    判断稀释液稳定的状态         */ 
           if(!oldWaterLevel_3)
              {    
                  PORTD |= 0x08;
                 PORTD &= 0xBF; 
              /*   if(reportTime>=ReportTime)
                 {                     
                 SendDataInfoFifo("$3303\r\n");   //清洗液异常
                 dealTxData();
                }
                 */
             }
             
             else if (oldWaterLevel_3)
              {    
                 PORTD |=  0x40;           //关闭LD3 PD6
             // SendDataInfoFifo("$!3303\r\n");   //清洗液恢复正常
             // dealTxData();             
             }
           
          
               /*    判断清洗液稳定的状态         */ 
          if(!oldWaterLevel_4)
              {  
                  PORTD |= 0x08;
                  PORTD &= 0x7F;
                /*  if(reportTime>=ReportTime)
                 {
                   SendDataInfoFifo("$3304\r\n");   //稀释液异常
                   dealTxData(); 
                 }
                  */
             }
             
             else if   (oldWaterLevel_4)
              { 
                  PORTD |= 0x80;            //关闭LD4 PD7
            //  SendDataInfoFifo("$!3304\r\n");   //稀释液恢复正常
              //dealTxData();             
              }
     

//向上位机报告当前的状态        
         if ((oldWaterLevel_4)&&oldWaterLevel_3&&oldWaterLevel_2)
             
         {    BuzzerFlag=false ;
              BuzzerCnt=0;
              PORTD|= 0xFF;
              PORTD &= 0xF7;
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$0$0$0\r\n"); //状态 1
              dealTxData(); 
                // for(unsigned char i = 0;i < 7;i++)
                // {
                 //       while(!(UCSR0A & 0X20));
                 //       UDR0 = sss[i];
                // }
              }
         }
         
         else
         {        BuzzerFlag=false ;
                // BuzzerCnt++;       
                //  BuzzerFlag=true;  
                 // if(BuzzerCnt>=10000)  //此处可以更改蜂鸣器的通断间隔
                //  BuzzerCnt=0 ;              
          } 
        
        if ((oldWaterLevel_2)&&oldWaterLevel_3&&(!oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$0$0$1\r\n"); //状态2
              dealTxData(); 
               
              }
         }
         if ((oldWaterLevel_2)&&(!oldWaterLevel_3)&&oldWaterLevel_4)
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$0$1$0\r\n"); //状态3
              dealTxData(); 
               
              }
         }
         if ((oldWaterLevel_2)&&(!oldWaterLevel_3)&&(!oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$0$1$1\r\n"); //状态4
              dealTxData(); 
               
              }
         }
          if ((!oldWaterLevel_2)&&(oldWaterLevel_3)&&(oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$1$0$0\r\n"); //状态5
              dealTxData(); 
               
              }
         }
           if ((!oldWaterLevel_2)&&(oldWaterLevel_3)&&(!oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$1$0$1\r\n"); //状态6
              dealTxData(); 
               
              }
         }
          if ((!oldWaterLevel_2)&&(!oldWaterLevel_3)&&(oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$1$1$0\r\n"); //状态7
              dealTxData(); 
               
              }
         }
        
          if ((!oldWaterLevel_2)&&(!oldWaterLevel_3)&&(!oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$1$1$1\r\n"); //状态 8
              dealTxData(); 
               
              }
         }
            if(reportTime>=ReportTime)
                reportTime=0;
                
       if(uartRxProcessEnable) //回复上位机查询的当前页面状态
        {
           /*  if(!strncmp((char const*)tempRec, "3201", 4))
            {  if((oldWaterLevel_4)&&oldWaterLevel_3&&oldWaterLevel_2)
              {
              SendDataInfoFifo("$3301\r\n");                                              
              dealTxData();             
              }
                if(!oldWaterLevel_2)
              {
              SendDataInfoFifo("$3302\r\n");
              dealTxData();             
               }
            
             if(!oldWaterLevel_3)
              {
              SendDataInfoFifo("$3303\r\n");
              dealTxData();             
               }
             
              if(!oldWaterLevel_4)
              {
              SendDataInfoFifo("$3304\r\n");
              dealTxData();             
               }
           
             }
         */   
            memset(tempRec,0,sizeof(tempRec)-1);
            
            uartRxProcessEnable = false; 
            
        }
            // dealTxData(); 
        
    
    }
}


#pragma vector = USART_RX_vect
__interrupt void UART_RX_ISR()
{      
    static bool startFlag = false;
    static char cnt = 0;
    unsigned char cMsg;
    cMsg = UDR0;
        
    if(STX == cMsg)
    {
        startFlag = true;
        cnt = 0;
    }
    else if(ETX == cMsg)
    {
        startFlag = false;        
        uartRxProcessEnable = true;        
        cnt = 0;
    }
    else
    {
        if(startFlag)
        {
            tempRec[cnt++] = cMsg;
        }
    }
}


void dealRxData(void)
{
//    if(!strncmp((char const*)tempRec, "GetLiquildLevelState", 21))
//    {
//        switch(waterLevel)
//        {
//        case 0:
//            SendDataInfoFifo("LiquidLevel-1\r\n");//below warning line    
//            break;
//        case 1:
//            SendDataInfoFifo("LiquidLevel-2\r\n");//below warning line    
//            break;
//        case 2:
//            break;
//        default:
//            break;
//        }
//    }

}


void dealTxData(void)
{
    while (txRingFifo.rear != txRingFifo.front)
 
    {
        if(UCSR0A & (1<<UDRE0))   //UDRE0为5  （UCSR0A&二进制00100000）
        {
            UDR0 = txRingFifo.data[txRingFifo.rear];
            txRingFifo.rear = (txRingFifo.rear + 1) % BUFFER_SIZE;
        }
    } 
}



