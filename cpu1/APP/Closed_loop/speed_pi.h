#ifndef APP_CLOSED_LOOP_SPEED_PI_H
#define APP_CLOSED_LOOP_SPEED_PI_H

#include <stdbool.h>
#include <stdint.h>

/* 1. 定义速度 PI 控制器的参数和状态 */
typedef struct
{
    float SetPoint;                                       // 目标机械转速，单位 r/s
    float LastError;                                      // 上一次速度误差，用于微分项
    float SumError;                                       // 速度误差累加值，用于积分项
    float Proportion;                                     // 比例系数 Kp
    float Integral;                                       // 积分系数 Ki
    float Derivative;                                     // 微分系数 Kd
    float Output;                                         // 限幅后的 PI 输出，作为 q 轴电压指令
    float OutputMin;                                      // PI 输出下限
    float OutputMax;                                      // PI 输出上限
    float FilterAlpha;                                    // 一阶低通滤波系数，范围 0 至 1
    uint16_t UpdatePrescaler;                             // 每隔多少个 PWM 周期更新一次 PI
    uint16_t UpdateCounter;                               // 当前 PI 分频计数器
    bool Enabled;                                         // 速度 PI 使能状态
} PID_Typedef;

extern PID_Typedef speedPID;                              // 声明全局速度 PI 控制器对象

void PMSM_PID_Init(void);                                 // 初始化速度 PI 参数和内部状态
void PMSM_SetPIDParams(float kp, float ki, float kd);     // 设置速度 PI 的 Kp、Ki、Kd
void PMSM_PID_SetOutputLimit(float min_output, float max_output); // 设置 PI 输出上下限
void PMSM_PID_SetFilterAlpha(float alpha);                // 设置速度反馈一阶低通滤波系数
void PMSM_PID_SetUpdatePrescaler(uint16_t prescaler);     // 设置速度环更新分频
void PMSM_PID_SetTargetSpeed(float target_speed_rps);     // 设置目标机械转速
float PMSM_PID_GetTargetSpeed(void);                      // 获取当前目标机械转速
void PMSM_PID_Enable(bool enable);                        // 使能或关闭速度 PI
void PMSM_PID_Update(void);                               // 按分频更新一次速度 PI 输出
float PMSM_PID_GetOutput(void);                           // 获取当前速度 PI 输出
void PMSM_PID_Reset(void);                                // 清零 PI 误差、积分和输出状态
float PMSM_GetFilteredSpeed(void);                        // 获取低通滤波后的机械转速
float PMSM_PID_GetIntegralTerm(void);                     // 获取当前积分项输出

#endif /* APP_CLOSED_LOOP_SPEED_PI_H */
