#include "MAIN.H"

//2015-08-07
//电机速度更改
//采集点数改为1300点

//define STEP_NUM and MICROSTEP_PER_STEP by lm 
#define STEP_NUM  2000UL
#define MICROSTEP_PER_STEP 32UL
#define COMPENSATION_STEP   (30*MICROSTEP_PER_STEP)  //电机反向时补偿

extern unsigned char RUN_FLAG;
extern unsigned long MotorStep;
extern unsigned long MotorMaxStep;
extern unsigned char MOTOR_RUN_FLAG;
extern unsigned char MOTOR_STAUS;
extern unsigned char SpeedFlag;

extern unsigned int cwStep;
extern unsigned int ccwStep;
extern unsigned char reverseFlag;
extern unsigned char lastMotion;
int i;

int currStep = 0;//0表示柱塞0位，正数柱塞相对0位，向下为正

extern int num2str(int num, char *strDest);

void MOTOR_CW()
{
    static unsigned char WorkStep = 0;
    switch(WorkStep)
    {
    case 0:
        MotorMaxStep = STEP_NUM * MICROSTEP_PER_STEP;   //changed by lm
        MotorStep = 0;
        MotorSpeed = MIN_SPEED;
        MOTOR_DIR = CW;
        MOTOR_EN = 0;
        SpeedFlag = 0;
        WorkStep = 1;
        break;
    case 1:
        if(MotorStep >= MotorMaxStep)  // 6400L * 16 
        {
            MOTOR_EN = 1;
            MOTOR_RUN_FLAG = 0;
            WorkStep = 0;
            SpeedFlag = 0;
            uart0SendData("FLUOMotorCWOK\r\n",15);
            if(MotorStep >= MotorMaxStep)
            {
                MotorStep = 0;
                MotorMaxStep = 0;
                uart0SendData("LEFT SENSOR ERROR\r\n",18);
            }
        }
        break;
    default:break;
    }
}

void MOTOR_CCW()
{
    static unsigned char WorkStep = 0;
    switch(WorkStep)
    {
    case 0:          
        //MotorMaxStep = 6400L * 16;
        MotorMaxStep = STEP_NUM * MICROSTEP_PER_STEP; //changed by lm
        MotorStep = 0;
        MotorSpeed = MIN_SPEED;
        MOTOR_DIR = CCW;
        MOTOR_EN = 0;
        SpeedFlag = 0;
        WorkStep = 1;
        break;
    case 1:
        if(MotorStep >= MotorMaxStep)
        {
            MOTOR_EN = 1;
            MOTOR_RUN_FLAG = 0;
            WorkStep = 0;
            SpeedFlag = 0;
            uart0SendData("FLUOMotorCCWOK\r\n",16);
            //if(MotorStep >= MotorMaxStep)
            //{
            MotorStep = 0;
            MotorMaxStep = 0;
            //   uart0SendData("RIGHT SENSOR ERROR\r\n",19);
            //}
        }
        else if(TOP == GetSensorState())
        {
            MOTOR_EN = 1;
            MOTOR_RUN_FLAG = 0;
            WorkStep = 0;
            SpeedFlag = 0;
            uart0SendData("FLUOMotorCCWOK\r\n",16);
            
            MotorStep = 0;
            MotorMaxStep = 0;
            uart0SendData("At Top Location\r\n",17);
        }
        else
        {
        }
        break;
    default:break;
    }
}


static void MOTOR_STOP()
{
    MOTOR_EN = 1;
    MOTOR_RUN_FLAG = 0;
    MotorMaxStep = 0;
    MotorStep = 0;
    SpeedFlag = 0;
    uart0SendData("FLUOMotorStopOK\r\n",17);  
    SetValveIdle();    //add by lm
}

static void MOTOR_RUN()
{
    static unsigned char WorkStep = 0;
    switch(WorkStep)
    {
    case 0:
        WDR;  
        //MotorMaxStep = 6400L * 16;
        MotorMaxStep = STEP_NUM * MICROSTEP_PER_STEP; //changed by lm
        MotorStep = 0;
        MotorSpeed = MIN_SPEED;
        MOTOR_DIR = CCW;
        MOTOR_EN = 0;
        SpeedFlag = 0;
        WorkStep = 1;          
        break;
    case 1:
        if(MotorSpeed == MIN_SPEED)// || MotorStep >= MotorMaxStep)
            //if(MotorStep >= MotorMaxStep && MotorSpeed == MIN_SPEED)
        {
            MOTOR_EN = 1;
            WorkStep = 2;
            SpeedFlag = 0;
            MotorMaxStep = 0;
            MotorStep = 0;
        }
        break;
    case 2:
        //MotorMaxStep = 6400L * 16;
        MotorMaxStep = STEP_NUM * MICROSTEP_PER_STEP; //changed by lm
        MotorStep = 0;
        MotorSpeed = MIN_SPEED;
        MOTOR_DIR = CW;
        MOTOR_EN = 0;
        SpeedFlag = 0;
        WorkStep = 3;
        break;
    case 3:
        if(MotorSpeed == MIN_SPEED)  // 6400L * 16 
        {
            MOTOR_EN = 1;
            MOTOR_RUN_FLAG = 0;
            WorkStep = 0;
            SpeedFlag = 0;
            MotorMaxStep = 0;
            MotorStep = 0;
            uart0SendData("FLUOMotorRunOK\r\n",16);
        }
        break;
    default:break;
    } 
}


static void MOTOR_INIT()
{
    static unsigned char WorkStep = 0;
    switch(WorkStep)
    {
    case 0:                   
        if(TOP == GetSensorState())
        {
            MOTOR_EN = 1;
            MOTOR_RUN_FLAG = 0;
            WorkStep = 0;
            SpeedFlag = 0; 
            MotorStep = 0;
            MotorMaxStep = 0;
            currStep = 0;            
            uart0SendData("At Top Location\r\n",17);
        }
        else
        {           
            MotorMaxStep = (1 == reverseFlag)?
                STEP_NUM * MICROSTEP_PER_STEP + COMPENSATION_STEP:
                STEP_NUM * MICROSTEP_PER_STEP ; //changed by lm
                MotorStep = 0;
                MotorSpeed = MIN_SPEED;
                MOTOR_DIR = CCW;
                MOTOR_EN = 0;
                SpeedFlag = 0;
                WorkStep = 1; 
        }
        break;    
    case 1:
        if(TOP == GetSensorState())
        {
            MOTOR_EN = 1;
            MOTOR_RUN_FLAG = 0;
            WorkStep = 0;
            SpeedFlag = 0; 
            MotorStep = 0;
            MotorMaxStep = 0;
            lastMotion = CCW;
            currStep = 0;
            uart0SendData("At Top Location\r\n",17);
        }
        else if(MotorStep >= MotorMaxStep)
        {
            MOTOR_EN = 1;
            WorkStep = 0;
            MotorStep = 0;
            MotorMaxStep = 0;
            MOTOR_RUN_FLAG = 0;
            SpeedFlag = 0;
            lastMotion = CCW;
            uart0SendData("$3True\r\n",8);
            uart0SendData("FLUOMotorInitOK\r\n",17);
        }
        break;
    default:break;
    }
}

//static void MOTOR_INIT()
//{
//  static unsigned char WorkStep = 0;
//  switch(WorkStep)
//  {
//  case 0:                   
//          //MotorMaxStep = 6400L * 15.6;
//          MotorMaxStep = STEP_NUM * MICROSTEP_PER_STEP; //changed by lm
//          MotorStep = 0;
//          MotorSpeed = MIN_SPEED;
//          MOTOR_DIR = CW;
//          MOTOR_EN = 0;
//          SpeedFlag = 0;
//          WorkStep = 1; 
//          //WorkStep = 4; //changed by lm
//          break;          
//  case 1:
//          if(MIDDLE_SENSOR)
//          {
//            WorkStep = 2;
//            uart0SendData("$2True\r\n",8);
//          }
//          break;
//  case 2:
//          if(MotorStep >= MotorMaxStep)
//          {
//            MOTOR_EN = 1;
//            WorkStep = 3; 
//            MotorStep = 0;
//            MotorMaxStep = 0;
//            SpeedFlag = 0;
//            uart0SendData("$1True\r\n",8);
//          }
//          break;
//  case 3:
//          //MotorMaxStep = 6400L * 16;
//          MotorMaxStep = STEP_NUM * MICROSTEP_PER_STEP; //changed by lm
//          MotorStep = 0;
//          MotorSpeed = MIN_SPEED;
//          MOTOR_DIR = CCW;
//          MOTOR_EN = 0;
//          SpeedFlag = 0;
//          WorkStep = 4;
//          break;
//  case 4: 
//          if(MotorStep >= MotorMaxStep)
//          {
//            MOTOR_EN = 1;
//            WorkStep = 0;
//            MotorStep = 0;
//            MotorMaxStep = 0;
//            MOTOR_RUN_FLAG = 0;
//            SpeedFlag = 0;
//            uart0SendData("$3True\r\n",8);
//            uart0SendData("FLUOMotorInitOK\r\n",17);
//          }
//          break;
//  default:break;
//  }
//}

static void MOTOR_HOME(void)
{
    static unsigned char WorkStep = 0;
    switch(WorkStep)
    {
    case 0:
        MotorSpeed = HOME_SPEED;
        MotorStep = 0;
        MotorMaxStep = 6400L * 2;
        MOTOR_DIR = CCW;
        MOTOR_EN = 0;
        WorkStep = 1;
        break;
    case 1:
        if(MotorStep >= MotorMaxStep)
        {
            MOTOR_EN = 1;
            WorkStep = 0;
            MotorStep = 0;
            MotorMaxStep = 0;
            SpeedFlag = 0;
            switch(MOTOR_STAUS)
            {
            case MOTORRUN:
                MOTOR_RUN_FLAG = 3;
                MOTOR_STAUS = 0;
                break;
            case MOTORINIT:
                MOTOR_RUN_FLAG = 11;
                MOTOR_STAUS = 0;
                break;
            default:break;
            }
        }
        break;
    default:break;
    
    }
}


static void MOTOR_CCW_USERDEF()
{
    static unsigned char WorkStep = 0;
    switch(WorkStep)
    {
    case 0:          
        MotorMaxStep = (1 == reverseFlag)?
            (ccwStep * MICROSTEP_PER_STEP + COMPENSATION_STEP):
            (ccwStep * MICROSTEP_PER_STEP );            
            MotorStep = 0;
            MotorSpeed = MIN_SPEED;
            MOTOR_DIR = CCW;
            MOTOR_EN = 0;
            SpeedFlag = 0;
            WorkStep = 1;
            break;
    case 1:
        if(MotorStep >= MotorMaxStep)
        {
            MOTOR_EN = 1;
            MOTOR_RUN_FLAG = 0;
            WorkStep = 0;
            SpeedFlag = 0;
            uart0SendData("MOTOR_CCW_USERDEF OK\r\n",22);
            lastMotion = CCW;
            
            MotorStep = 0;
            MotorMaxStep = 0;            
        }
        //          else if(TOP == GetSensorState())
        //          {
        //            MOTOR_EN = 1;
        //            MOTOR_RUN_FLAG = 0;
        //            WorkStep = 0;
        //            SpeedFlag = 0; 
        //            MotorStep = 0;
        //            MotorMaxStep = 0;
        //            lastMotion = CCW;
        //            uart0SendData("At Top Location\r\n",17);
        //          }
        else
        {
        }
        break;
    default:break;
    }
    
}

static void MOTOR_CW_USERDEF()
{
    static unsigned char WorkStep = 0;
    //char stepStr[10] ="";            
    switch(WorkStep)
    {
    case 0:
        MotorMaxStep = (1 == reverseFlag)?
            (cwStep * MICROSTEP_PER_STEP + COMPENSATION_STEP):
            (cwStep * MICROSTEP_PER_STEP );            
            MotorStep = 0;
            MotorSpeed = MIN_SPEED;
            MOTOR_DIR = CW;
            MOTOR_EN = 0;
            SpeedFlag = 0;
            WorkStep = 1;
            SetValveForPumpIn();  //add by lm
            break;
    case 1:
        if(MotorStep >= MotorMaxStep)  // 6400L * 16 
        {
            MOTOR_EN = 1;
            MOTOR_RUN_FLAG = 0;
            WorkStep = 0;
            SpeedFlag = 0;
            uart0SendData("MOTOR_CW_USERDEF OK\r\n",21);
            
            MotorMaxStep = 0;            
            lastMotion = CW;
//            currStep = 1234;//MotorStep >> 5; 
//            num2str(currStep, stepStr);
//            uart0SendData("Curr step:",10);
//            uart0SendData(stepStr,5);
//            uart0SendData("\r\n",2);
            
            SetValveIdle();    //add by lm
            MotorStep = 0;
        }
        break;
    default:break;
    }    
}


void MOTOR_ALL_RUN()
{
    switch(MOTOR_RUN_FLAG)
    {
    case 0 :  break;
    case 1 :  MOTOR_CW();break;
    case 2 :  MOTOR_STOP();break;
    case 3 :  MOTOR_RUN();break;
    case 4 :  MOTOR_CCW();break;
    case 5 :  MOTOR_HOME();break;
    case 11:  MOTOR_INIT();break;  //修改INIT和RUN，和CCW函数，到不到最下面
    case 12:  MOTOR_CW_USERDEF();break;   //进液
    case 13:  MOTOR_CCW_USERDEF();break;  //出液
    default:break;
    }
}
