#include "MAIN.H"

extern unsigned char SENSOR_FLAG;   
extern unsigned char RX_NUM;
extern unsigned char RX_OVER_FLAG;
extern unsigned char RX_OVER_TIME;
extern bool  BuzzerFlag;
extern unsigned int BuzzerCnt;


extern void SendDataInfoFifo(char* data);

volatile bool timerTriggerFlag = false;
unsigned int timer1Cnt = 0;
//unsigned int timer2Cnt = 0;


static void WDT_INIT()
{
    WDR;
    WDTCSR = 0x1D;  //0.5s
}


void TIMER_INIT(void)
{
    WDT_INIT();
    
    //T0_INIT 0.1MS
    TCCR0A = 0X02;
    //TCCR0B = 0X03;  //64分频
    //TCCR0B = 0X04;  //256分频
    TCCR0B = 0X05;  //1024分频
    TIMSK0 = 0x02; 
    TCNT0 = 0;
    //OCR0A = 31;  //period = 64*31/20M = 0.0992ms
    //OCR0A = 80;  //period = 256*80/20M = 1.024ms    
    OCR0A = 195;    //period = 1024*195/20M = 10ms
    
   //   OCR0A = 5;    //period = 1024*19/20M = 0.25ms
    
    
 TCCR2B = 0x00; //stop
 TCNT2 = 0xD9; //setup
 TCCR2A = 0x00; 
 TCCR2B = 0x06; //start  256分频
 TIMSK2 |= 0X01;
    
    
}

#pragma vector = TIMER0_COMPA_vect
__interrupt void TIMER0_ISR() //10ms中断一次
{ 
    
    timer1Cnt++;
    //timer2Cnt++;

//if(timer2Cnt==40)

       WDR;      //10ms feed dog   
       
     //  timer2Cnt=0  ;
    timerTriggerFlag = true;
  
}

 #pragma vector = TIMER2_OVF_vect
__interrupt void TIMER2_OVF_ISR() //0.125ms
{
     TCNT2 = 0xF5;
     
      BuzzerCnt++;         
      
    if (BuzzerFlag) 
   {   
       if(BuzzerCnt<=2000)          //液位出现异常情况响250s 休息250ms再响250ms休息250ms
         PORTB_Bit5= ~PORTB_Bit5;    //常态是高电平
            else if(BuzzerCnt>=4000)  //此处可以更改蜂鸣器的通断间隔
                   BuzzerCnt=0 ; 
            
   }
    
}
