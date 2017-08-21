#ifndef _MAIN_H_
#define _MAIN_H_


#include <iom88.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdbool.h"

#define SEI asm("SEI") //��ȫ���ж�
#define CLI asm("CLI") //��-------
#define WDR asm("WDR")
#define _nop_() asm("nop")

#define STX		          0X02
#define ETX		          0X03

#define SYS_FREQ        20000000UL
#define BAUD            38400
#define UBRL            (SYS_FREQ / 16 / BAUD - 1)

#define MotorSpeed  OCR1A
 
#define LED           PORTC_Bit2      //LOW EFFECT


#define scanCnt_time  50        //�����ȶ��жϴ���Ϊ50*10ms�� �ȶ�ʱ��Ϊ500ms
#define ReportTime    10      //ÿ��10*1s�ӻ㱨һ��״̬
#endif
