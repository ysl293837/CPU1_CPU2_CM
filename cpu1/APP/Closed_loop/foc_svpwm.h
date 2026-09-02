#ifndef __FOC_SVPWM_H
#define __FOC_SVPWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "driverlib.h"
#include "device.h"
#include "board.h"

#define PWM_FREQUENCY          (10000.0f)                 // ePWM 开关频率，单位 Hz
#define PWM_PERIOD_CYCLES      (10000U)                   // SysConfig 对应的 ePWM TBPRD 值
#define PWM_CMP_MIN            (100U)                     // CMPA 安全最小比较值
#define PWM_CMP_MAX            (PWM_PERIOD_CYCLES - 100U) // CMPA 安全最大比较值

#ifndef FOC_EPWM_U_BASE
#define FOC_EPWM_U_BASE        (myEPWM1_BASE)             // U 相使用 ePWM1
#endif

#ifndef FOC_EPWM_V_BASE
#define FOC_EPWM_V_BASE        (myEPWM2_BASE)             // V 相使用 ePWM2
#endif

#ifndef FOC_EPWM_W_BASE
#define FOC_EPWM_W_BASE        (myEPWM3_BASE)             // W 相使用 ePWM3
#endif

#define SQRT3                  (1.732051f)                // √3 常数
#define SQRT3_DIV2             (1.732051f / 2.0f)         // √3/2 常数
#define POLE_PAIRS             (5)                        // 电机极对数
#define UREF                   (65535.0f)                 // SVPWM 调制量对应的归一化母线参考值

extern float Motor_Electric_Angle;                        // 声明主程序维护的当前电角度

/* 1. 定义 αβ 电压结构 */
typedef struct
{
    float V_alpha;                                        // α 轴电压
    float V_beta;                                         // β 轴电压
} Volt_Typedef;

/* 2. 定义三相 SVPWM 作用时间结构 */
typedef struct
{
    float Ta;                                             // A 相作用时间
    float Tb;                                             // B 相作用时间
    float Tc;                                             // C 相作用时间
} Time_Typedef;

/* 3. 定义三相 ePWM CMPA 比较值结构 */
typedef struct
{
    uint16_t CCR1;                                        // U 相 ePWM1 CMPA 值
    uint16_t CCR2;                                        // V 相 ePWM2 CMPA 值
    uint16_t CCR3;                                        // W 相 ePWM3 CMPA 值
} PWM_Duty_CCR_Typedef;

extern PWM_Duty_CCR_Typedef PWM_Channel;                  // 声明三相 PWM 比较值对象
extern float UA;                                          // 声明 A 相参考电压
extern float UB;                                          // 声明 B 相参考电压
extern float UC;                                          // 声明 C 相参考电压

void FOC_InitVariables(void);                             // 初始化 FOC 中间变量和 PWM 初值
void FOC_CalculateElectricalAngle(void);                  // 计算电角度正弦和余弦
void FOC_VoltageCalculation(float ud, float uq);          // 将 dq 电压变换为三相参考电压
void FOC_GetSector(uint8_t *sector, float UA_in, float UB_in, float UC_in); // 计算 SVPWM 扇区
void FOC_CalculateSectorTimes(uint8_t sector, float UA_in, float UB_in, // 计算各扇区三相作用时间
                              float UC_in, float *T1, float *T2, float *T3);
void FOC_CalculateSectorTimes2(uint8_t sector, float UA_in, float UB_in, // 保留的兼容扇区时间接口
                               float UC_in, float *T1, float *T2, float *T3);
void FOC_CalculatePWMValues(float T1, float T2, float T3, float Ts); // 将作用时间转换为 CMPA 值
void FOC_SVPWM_Update(float ud, float uq, float Electric_angle); // 完成一次 FOC 到 SVPWM 更新
void FOC_ApplyPWM(void);                                  // 将三相 CMPA 值写入 ePWM 硬件

#ifdef __cplusplus
}
#endif

#endif /* __FOC_SVPWM_H */
