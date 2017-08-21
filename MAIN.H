#ifndef _MAIN_H_
#define _MAIN_H_


#include <iom88.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdbool.h"

#define SEI asm("SEI") //开全局中断
#define CLI asm("CLI") //关-------
#define WDR asm("WDR")
#define _nop_() asm("nop")

#define STX		          0X02
#define ETX		          0X03

#define SYS_FREQ        20000000UL
#define BAUD            38400
#define UBRL            (SYS_FREQ / 16 / BAUD - 1)

#define MotorSpeed  OCR1A
 
#define LED           PORTC_Bit2      //LOW EFFECT


#define scanCnt_time  50        //连续稳定判断次数为50*10ms次 稳定时间为500ms
#define ReportTime    10      //每隔10*1s钟汇报一次状态
#endif

