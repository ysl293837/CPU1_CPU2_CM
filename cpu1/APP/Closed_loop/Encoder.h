/**
  ******************************************************************************
  * @file    pmsm_control.h
  * @brief   PMSM����ģ��ͷ�ļ�
  * @details �����������ٶȼ��㡢PID���ơ�FOC���ƵȺ�������
  ******************************************************************************
  * @attention
  *
  * ��Ȩ���� (c) 2025 STMicroelectronics
  * ��������Ȩ��
  *
  ******************************************************************************
  */

#ifndef __ENCODER_H
#define __ENCODER_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

// #include "tim.h"


#define ENCODER_PULSES_PER_REV    10000.0f    // ������ÿת������
#define SPEED_CALC_INTERVAL      1         // ÿ10���жϼ���һ���ٶ�


/* �ⲿ�������� --------------------------------------------------------------*/
// ��������ر���
extern uint64_t diff_ticks ;
extern float s_speed_r_per_sec;             // ��ǰת��
extern uint16_t call_count;                 // ���ô���������
extern int32_t saved_encoder_count;         // ��һ�εı���������ֵ
extern int32_t current_count;               // ��ǰ�ı���������ֵ
extern int32_t pulse_diff;                  // ����仯��
extern uint64_t saved_timestamp;            // ��һ�ε�ʱ���
extern uint64_t current_timestamp;          // ��ǰʱ���
extern float time_diff;                  // ʱ���
extern int32_t time_diff_int;									// ʱ���					-->΢��
extern uint64_t timestamp;                  // ʱ���
extern float Motor_Electric_Angle;        // �����Ƕ�



/* �������� ------------------------------------------------------------------*/


/* ��������غ��� */
void Save_current_T2(void);                    // ���ñ��������ٶȼ�����ز���
float Encoder_CalculateSpeed(void);          // ������ת�٣�ÿ10���жϼ���һ�Σ�
float Encoder_GetSpeed(void);                // ��ȡ��ǰ���ת��
uint64_t Get_Timer2_TimeStamp(void);           // ��ȡTIM2��ʱ��ʱ�����64λ��
float Calculate_Time_Diff(uint64_t start, uint64_t end);  // ��������ʱ���֮���ʱ���



#endif /* __PMSM_CONTROL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/




