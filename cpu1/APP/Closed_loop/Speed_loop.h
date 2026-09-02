#ifndef __SPEED_LOOP_H
#define __SPEED_LOOP_H


#include "Encoder.h"

#include <stdbool.h>
#include <math.h>






// PID控制结构体
typedef struct 
{
  float    SetPoint;           // 目标速度 (r/s)
  float    LastError;          // 前一次误差    
  float    SumError;           // 累计误差
  float    Proportion;         // Kp系数
  float    Integral;           // Ki系数
  float    Derivative;         // Kd系数
  float    Output;             // PID输出
}PID_Typedef;

extern PID_Typedef speedPID;  // 声明，不是定义

// 函数声明
void PMSM_PID_Init(void);
void PMSM_PID_SetTargetSpeed(float target_speed_rps);
float PMSM_PID_GetTargetSpeed(void);
void PMSM_PID_Enable(bool enable);
void PMSM_PID_Update(void);
float PMSM_PID_GetOutput(void);
void PMSM_PID_Reset(void);
float PMSM_GetCurrentSpeed(void);
void PMSM_SetPIDParams(float kp, float ki, float kd);
float PMSM_PID_GetKp(void);
float PMSM_PID_GetKi(void);
float PMSM_PID_GetKd(void);

#endif /* PMSM_CONTROL_H */





