#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "foc_svpwm.h"
#include "speed_pi.h"
#include "Startup_stage.h"
#include "APP/key/key.h"
#include "APP/ecat/ecat_gpio.h"
#include "APP/ipc/ipc_cpu1.h"

float TWOPI = 6.2831853071795864769f;                    // 一圈对应的电角度弧度值
float ONEPI = 3.14159265358979323846f;                    // 半圈对应的电角度弧度值

uint16_t ADC1_RESULT[3] = {0U, 0U, 0U};                  // 预留 ADC1 三相采样结果
uint16_t ADC2_RESULT[3] = {0U, 0U, 0U};                  // 预留 ADC2 三相采样结果
uint16_t ADC3_RESULT[3] = {0U, 0U, 0U};                  // 预留 ADC3 三相采样结果

float vel_ff = 0.0f;                                     // 预留速度前馈量
float acc_ff = 0.0f;                                     // 预留加速度前馈量
float speed_ref_total = 0.0f;                            // 预留总速度给定
float uq_fb = 0.0f;                                      // 预留 q 轴反馈电压
float uq_ff = 0.0f;                                      // 预留 q 轴前馈电压
float K_VFF = 0.2f;                                      // 预留速度前馈系数
float K_AFF = 0.02f;                                     // 预留加速度前馈系数

#define ENCODER_COUNT_PER_REV 10000L                     // 编码器每机械圈计数值
#define EQEP_CAPTURE_CLK_HZ 200000000.0f                 // eQEP 捕获时钟频率
#define EQEP_UPEVNT_COUNT 4.0f                           // 每次速度捕获的单位事件数

int32_t current_count = 0;                               // 当前编码器位置计数
int32_t OverflowCount = 0;                               // 预留编码器溢出计数

float ud = 0.0f;                                         // d 轴电压指令
float uq = 0.0f;                                         // q 轴电压指令
float Motor_Electric_Angle = 0.0f;                       // 当前电角度
float Motor_mechanical_Angle = 0.0f;                     // 当前机械角度

float target_position = 0.0f;                            // 预留目标位置
float current_position = 0.0f;                           // 预留当前位置
float target_speed = 5.0f;                               // 预留目标转速
float current_speed = 0.0f;                              // 当前机械转速

volatile uint16_t g_motor_run_enabled = 0U;              // 闭环电机运行使能标志
volatile uint16_t g_motor_estop_active = 1U;              // 上电默认处于急停保护状态

__interrupt void epwm1ISR(void);                         // 声明 ePWM1 控制周期中断函数

/* 1. 将电角度限制到 0 至 2π */
static float wrap_angle_0_to_2pi(float angle_rad)
{
    while(angle_rad >= TWOPI)                             // 处理超过一整圈的正角度
    {
        angle_rad -= TWOPI;                               // 减去一整圈使角度回到范围内
    }

    while(angle_rad < 0.0f)                               // 处理小于零的负角度
    {
        angle_rad += TWOPI;                               // 加上一整圈使角度回到范围内
    }

    return angle_rad;                                     // 返回归一化后的电角度
}

/* 2. 初始化 CPU1 的外设、三核通信和电机控制 */
int main(void)
{
    Device_init();                                        // 初始化 CPU1 时钟和基础设备配置
    Device_initGPIO();                                    // 解锁并初始化 GPIO 基础状态
    Interrupt_initModule();                               // 初始化 PIE 中断模块
    Interrupt_initVectorTable();                          // 初始化 PIE 中断向量表

    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC); // 配置 PWM 前暂停所有 PWM 时基
    Board_init();                                         // 执行 SysConfig 生成的外设初始化

    /* 3. 配置 EtherCAT 硬件并交给 CM */
    ECAT_GPIO_init();                                     // 配置开发板 EtherCAT 专用引脚复用
    SysCtl_allocateSharedPeripheral(SYSCTL_PALLOCATE_ETHERCAT, 1U); // 将 EtherCAT 外设控制权分配给 CM

    /* 4. 建立 CPU1 与 CM 的 IPC 后启动 CM */
    IPC_CPU1_CM_Init();                                   // 初始化 CPU1↔CM 消息队列和接收中断
    Device_bootCM(BOOTMODE_BOOT_TO_FLASH_SECTOR0);        // 让 CM 从 Flash Sector 0 启动
    IPC_CPU1_CM_SyncWithCM();                             // 在 FLAG31 等待 CM 完成 IPC 初始化

    /* 5. 将 SCIA 交给 CPU2 并启动 CPU2 */
    IPC_CPU1_AssignSCIAtoCPU2();                          // 将 SCIA 和 GPIO28/29 的控制权交给 CPU2
    IPC_CPU1_Init();                                      // 初始化 CPU1↔CPU2 消息队列
    IPC_CPU1_SyncWithCPU2();                              // 启动 CPU2 并在 FLAG31 处同步

    /* 6. 清除保护状态并初始化 FOC */
    EQEP_enableModule(myEQEP0_BASE);                      // 使能编码器 eQEP 外设
    EQEP_setPosition(myEQEP0_BASE, 0U);                   // 将编码器位置清零作为机械零点
    EPWM_clearTripZoneFlag(myEPWM1_BASE,                  // 清除 ePWM1 的故障保护锁存标志
                           EPWM_TZ_INTERRUPT |
                           EPWM_TZ_FLAG_OST |
                           EPWM_TZ_FLAG_CBC);
    EPWM_clearTripZoneFlag(myEPWM2_BASE,                  // 清除 ePWM2 的故障保护锁存标志
                           EPWM_TZ_INTERRUPT |
                           EPWM_TZ_FLAG_OST |
                           EPWM_TZ_FLAG_CBC);
    EPWM_clearTripZoneFlag(myEPWM3_BASE,                  // 清除 ePWM3 的故障保护锁存标志
                           EPWM_TZ_INTERRUPT |
                           EPWM_TZ_FLAG_OST |
                           EPWM_TZ_FLAG_CBC);

    FOC_InitVariables();                                  // 初始化 FOC 的三相占空比和坐标变量
    FOC_ApplyPWM();                                       // 将初始安全占空比写入 ePWM 寄存器
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC); // 完成 PWM 配置后恢复时基计数
    Rotor_Positioning2(10000.0f, 0.0f, 500000U);          // 执行转子预定位以建立电角度零点

    /* 7. 初始化速度 PI 和按键目标速度 */
    EQEP_setPosition(myEQEP0_BASE, 0U);                   // 预定位结束后重新清零机械位置
    PMSM_PID_Init();                                      // 初始化速度 PI 内部状态
    PMSM_SetPIDParams(50.0f, 5.0f, 0.0f);                 // 设置速度 PI 的 Kp、Ki、Kd
    PMSM_PID_SetOutputLimit(-30000.0f, 30000.0f);         // 限制 PI 输出对应的 q 轴电压范围
    PMSM_PID_SetFilterAlpha(1.00f);                       // 设置速度测量滤波系数
    PMSM_PID_SetUpdatePrescaler(10U);                     // 每 10 次 PWM 中断更新一次速度 PI
    PMSM_PID_SetTargetSpeed(5.0f);                        // 写入初始目标机械转速
    PMSM_PID_Enable(true);                                // 使能速度 PI 算法
    uq = 0.0f;                                            // 启动前保持 q 轴电压为零

    Key_Init();                                           // 初始化四个按键输入 GPIO
    Key_SetCommandSpeed(5.0f);                            // 设置按键和 PI 共用的初始目标速度

    /* 8. 开启 ePWM 控制中断和全局中断 */
    Interrupt_register(INT_EPWM1, &epwm1ISR);             // 注册 ePWM1 周期中断服务函数
    Interrupt_enable(INT_EPWM1);                          // 使能 ePWM1 中断通道
    EPWM_clearEventTriggerInterruptFlag(myEPWM1_BASE);    // 清除可能残留的 ePWM1 中断标志

    EINT;                                                 // 打开 CPU 全局可屏蔽中断
    ERTM;                                                 // 打开实时调试中断响应

    /* 9. 主循环处理本地按键和低频 IPC 遥测 */
    while(1)                                              // 持续运行 CPU1 应用主循环
    {
        Key_Process();                                    // 轮询按键并更新目标速度
        IPC_CPU1_PollSend(current_count, current_speed);  // 向 CPU2 和 CM 发送周期遥测数据
    }
}

/* 10. 执行一次 FOC 电流控制周期 */
__interrupt void epwm1ISR(void)
{
    uint16_t qprd = 0U;                                   // 保存 eQEP 捕获周期
    int16_t dir = 0;                                      // 保存编码器运动方向

    current_count = (int32_t)EQEP_getPosition(myEQEP0_BASE); // 读取当前编码器位置
    qprd = EQEP_getCapturePeriodLatch(myEQEP0_BASE);      // 读取单位位置事件的捕获周期
    dir = EQEP_getDirection(myEQEP0_BASE);                // 读取编码器当前转动方向

    if(qprd > 0U)                                         // 捕获周期有效时才计算速度
    {
        current_speed = (float)dir *                      // 将方向乘入机械转速符号
                        (EQEP_CAPTURE_CLK_HZ * EQEP_UPEVNT_COUNT) /
                        ((float)qprd * (float)ENCODER_COUNT_PER_REV); // 根据捕获周期计算 r/s
    }

    Motor_mechanical_Angle = TWOPI *                      // 将编码器计数换算为机械角度
                             (float)current_count /
                             (float)ENCODER_COUNT_PER_REV;
    Motor_Electric_Angle = wrap_angle_0_to_2pi(           // 用极对数换算并归一化电角度
                               Motor_mechanical_Angle * (float)POLE_PAIRS);
    IPC_CPU1_TickFromISR();                               // 累计遥测分频节拍但不在 ISR 内发送消息

    if((g_motor_run_enabled != 0U) &&                     // 仅在运行使能有效时输出速度环控制量
       (g_motor_estop_active == 0U))                      // 急停未触发才允许驱动电机
    {
        PMSM_PID_Update();                                // 根据目标速度和当前速度更新 PI
        uq = PMSM_PID_GetOutput();                        // 获取 PI 输出作为 q 轴电压指令
    }
    else
    {
        PMSM_PID_Reset();                                 // 停止或急停时清除 PI 积分状态
        uq = 0.0f;                                        // 停止或急停时撤销转矩电压
    }

    FOC_SVPWM_Update(ud, uq, Motor_Electric_Angle);       // 将 dq 电压和电角度换算为三相 PWM
    FOC_ApplyPWM();                                       // 将计算的三相占空比写入 ePWM

    EPWM_clearEventTriggerInterruptFlag(myEPWM1_BASE);    // 清除本次 ePWM1 周期中断标志
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP3);        // 应答 PIE 第 3 组中断
}
