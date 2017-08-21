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
       
      // DDRB &= 0xDF;      //PB5  Buzzer as input ���η�����
    
    DDRC &= 0xFD;    //PC1 as input
     DDRC &= 0xFB;    //PC2 as input
      DDRC &= 0xEF;    //PC4 as input
      
      
        PORTB = 0xFF;    //��̬�Ǹߵ�ƽ
        
        PORTC = 0xFF;    //��̬�Ǹߵ�ƽ
        PORTD = 0xFF;    //��̬�Ǹߵ�ƽ ��lEd�����״̬ ���͵�ƽ��Ч
        

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
    //UCSR0B |= 0X98;     //���պͽ����ж�ʹ��
    //UCSR0B = 0x18; 
    UCSR0B = 0x98;  //ʹ�ܷ��ͺͽ��գ�ʹ�ܽ����ж�
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

//��ת�ַ���  
char *reverse(char *s)
{  
    char temp;  
    char *p = s;    //pָ��s��ͷ��  
    char *q = s;    //qָ��s��β��  
    while(*q)  
        ++q;  
    q--;  
    
    //�����ƶ�ָ�룬ֱ��p��q����  
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
    //static char numToStr[5];      //����Ϊstatic������������ȫ�ֱ���
    
    do      //�Ӹ�λ��ʼ��Ϊ�ַ���ֱ�����λ�����Ӧ�÷�ת  
    {  
        numToStr[i++] = (n%10) + '0';
        n = n/10;  
    }while(n > 0); 
    
    numToStr[i] = '\0';    //�������ַ���������  
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
    
   //  unsigned char  waterLevel_1 = 0;  //0�������ź�1���쳣�ź�
    //  unsigned char  oldWaterLevel_1 = 0;
   // bool waterLevelChangedFlag_1 = false;  //��Һ�����߱��λ
   // int scanCnt_1 = 0;                        //��������� ���ű��� ��Һ����Ҫ�������Һ��� 
    
    unsigned char  waterLevel_2 = 0;          //��ǰ��ʵʱ״̬
    unsigned char  oldWaterLevel_2 = 1;       //�ȶ�ʱ���״̬ 1����  0 �쳣
    bool waterLevelChangedFlag_2 = false;  //��Һǿ���߱��λ
    int scanCnt_2 = 0;
    
    unsigned char  waterLevel_3 = 0;
    unsigned char  oldWaterLevel_3 = 1;       //1����  0 �쳣
    bool waterLevelChangedFlag_3 = false;  //ϡ��Һǿ���߱��λ
     int scanCnt_3 = 0;
     
    unsigned char  waterLevel_4 = 0;
    unsigned char  oldWaterLevel_4 = 1;      //1����  0 �쳣
    bool waterLevelChangedFlag_4 = false;  //��ϴҺǿ���߱��λ
     int scanCnt_4 = 0;
     int reportTime = 0;
    
     
     BuzzerCnt=0;
    
    //int uartSendTimer = 0;
    
    CLI;    //��ȫ���ж�
    PORT_INIT();
    UART_INIT();
    TIMER_INIT();
    SEI;    //��ȫ���ж�
    
   // LED = ~LED;
    
    SendDataInfoFifo("Hello,Atmega88\r\n");
     //int k=0;
        
    while(1)
    {             
        
        if(100 <= timer1Cnt)  //����ѯ�ʻ��Ʊ�ʾ������Ѿ�����1s ����һ��
        {
            reportTime++  ;
            
         //  if (reportTime>ReportTime)
           //      reportTime=0; 
            
            timer1Cnt = 0;
             //1s��ѯһ��
            //SendDataInfoFifo("$3300\r\n");
            //dealTxData(); �˴��������д���ͨ�Ű��ֽڷ����쳣���
        }
         
         if(timerTriggerFlag) //����ϼ��Һ���źţ�10ms��ѯһ��
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
              SendDataInfoFifo("$3302\r\n");  //��Һ�쳣
              dealTxData();             
             }  
          else if((waterLevel_2)&&(!waterLevelChangedFlag_2))
              {       
                  //PORTD |= 0x08;  //�ر�LD1 PD3
                    PORTD |= 0x20;  //�ر�LD2     PD5
               //   PIND_Bit5=0;
              SendDataInfoFifo("$!3302\r\n");  //��Һ�쳣
              dealTxData();             
             }
             
            
        */
        /*    ��ȡ��Һ״̬         */
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
          
            /*    ��ȡϡ��Һ״̬         */      
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
           
            /*    ��ȡ��ϴҺ״̬         */ 
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
    
    
    
      /*    �ж�ϡ��Һ�ȶ���״̬         */        
            if(!oldWaterLevel_2)
              {       
                  PORTD |= 0x08;  //�ر�LD1 PD3
                    PORTD &= 0xDF;  //��LD2     PD5
                    
                 /*   if(reportTime>=ReportTime)
                    {
                    SendDataInfoFifo("$3302\r\n");  //��Һ�쳣
                     dealTxData(); 
                    }
                    */
                    
               }
             
             
             else if(oldWaterLevel_2)
              {       
                    PORTD |= 0x20;  //�ر�LD2     PD5
              //SendDataInfoFifo("$!3302\r\n");  //��Һ�ָ�����
              //dealTxData();             
               }
                 
               /*    �ж�ϡ��Һ�ȶ���״̬         */ 
           if(!oldWaterLevel_3)
              {    
                  PORTD |= 0x08;
                 PORTD &= 0xBF; 
              /*   if(reportTime>=ReportTime)
                 {                     
                 SendDataInfoFifo("$3303\r\n");   //��ϴҺ�쳣
                 dealTxData();
                }
                 */
             }
             
             else if (oldWaterLevel_3)
              {    
                 PORTD |=  0x40;           //�ر�LD3 PD6
             // SendDataInfoFifo("$!3303\r\n");   //��ϴҺ�ָ�����
             // dealTxData();             
             }
           
          
               /*    �ж���ϴҺ�ȶ���״̬         */ 
          if(!oldWaterLevel_4)
              {  
                  PORTD |= 0x08;
                  PORTD &= 0x7F;
                /*  if(reportTime>=ReportTime)
                 {
                   SendDataInfoFifo("$3304\r\n");   //ϡ��Һ�쳣
                   dealTxData(); 
                 }
                  */
             }
             
             else if   (oldWaterLevel_4)
              { 
                  PORTD |= 0x80;            //�ر�LD4 PD7
            //  SendDataInfoFifo("$!3304\r\n");   //ϡ��Һ�ָ�����
              //dealTxData();             
              }
     

//����λ�����浱ǰ��״̬        
         if ((oldWaterLevel_4)&&oldWaterLevel_3&&oldWaterLevel_2)
             
         {    BuzzerFlag=false ;
              BuzzerCnt=0;
              PORTD|= 0xFF;
              PORTD &= 0xF7;
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$0$0$0\r\n"); //״̬ 1
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
                 // if(BuzzerCnt>=10000)  //�˴����Ը��ķ�������ͨ�ϼ��
                //  BuzzerCnt=0 ;              
          } 
        
        if ((oldWaterLevel_2)&&oldWaterLevel_3&&(!oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$0$0$1\r\n"); //״̬2
              dealTxData(); 
               
              }
         }
         if ((oldWaterLevel_2)&&(!oldWaterLevel_3)&&oldWaterLevel_4)
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$0$1$0\r\n"); //״̬3
              dealTxData(); 
               
              }
         }
         if ((oldWaterLevel_2)&&(!oldWaterLevel_3)&&(!oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$0$1$1\r\n"); //״̬4
              dealTxData(); 
               
              }
         }
          if ((!oldWaterLevel_2)&&(oldWaterLevel_3)&&(oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$1$0$0\r\n"); //״̬5
              dealTxData(); 
               
              }
         }
           if ((!oldWaterLevel_2)&&(oldWaterLevel_3)&&(!oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$1$0$1\r\n"); //״̬6
              dealTxData(); 
               
              }
         }
          if ((!oldWaterLevel_2)&&(!oldWaterLevel_3)&&(oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$1$1$0\r\n"); //״̬7
              dealTxData(); 
               
              }
         }
        
          if ((!oldWaterLevel_2)&&(!oldWaterLevel_3)&&(!oldWaterLevel_4))
             
         {    
              if(reportTime>=ReportTime)
              {
              SendDataInfoFifo("*3301$1$1$1\r\n"); //״̬ 8
              dealTxData(); 
               
              }
         }
            if(reportTime>=ReportTime)
                reportTime=0;
                
       if(uartRxProcessEnable) //�ظ���λ����ѯ�ĵ�ǰҳ��״̬
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
        if(UCSR0A & (1<<UDRE0))   //UDRE0Ϊ5  ��UCSR0A&������00100000��
        {
            UDR0 = txRingFifo.data[txRingFifo.rear];
            txRingFifo.rear = (txRingFifo.rear + 1) % BUFFER_SIZE;
        }
    } 
}



