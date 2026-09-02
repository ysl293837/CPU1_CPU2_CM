#include <stdint.h>

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "APP/ipc/ipc_cpu2.h"
#include "APP/pc_serial/pc_serial_cpu2.h"

/* 1. 初始化 CPU2 串口服务和 IPC 转发 */
void main(void)
{
    int32_t encoderCount;                                 // 保存从 CPU1 收到的编码器计数
    float currentSpeed;                                   // 保存从 CPU1 收到的机械转速

    Device_init();                                        // 初始化 CPU2 时钟和基础设备配置
    Device_initGPIO();                                    // 初始化 CPU2 可控制的 GPIO
    Interrupt_initModule();                               // 初始化 PIE 中断模块
    Interrupt_initVectorTable();                          // 初始化 PIE 中断向量表

    IPC_CPU2_Init();                                      // 初始化 CPU2↔CPU1 IPC 消息队列
    Board_init();                                         // 执行 SysConfig 生成的 SCIA 和中断初始化
    Interrupt_enable(INT_mySCI1_RX);                      // 使能 SCIA 接收中断通道
    PC_Serial_CPU2_Init();                                // 初始化串口命令和遥测服务
    IPC_CPU2_SyncWithCPU1();                              // 在 FLAG31 等待 CPU1 完成启动

    EINT;                                                 // 打开 CPU 全局可屏蔽中断
    ERTM;                                                 // 打开实时调试中断响应

    /* 2. 主循环处理串口、应答和遥测输出 */
    for(;;)                                               // 持续运行 CPU2 通信服务
    {
        PC_Serial_CPU2_ProcessRx();                       // 拼接并解析上位机输入命令
        PC_Serial_CPU2_ProcessAcks();                     // 输出 CPU1 返回的命令应答

        if(IPC_CPU2_TakeTelemetry(&encoderCount, &currentSpeed) && // 读取一帧新 CPU1 遥测
           PC_Serial_CPU2_StreamDue())                    // 判断是否达到串口输出周期
        {
            PC_Serial_CPU2_SendTelemetry(encoderCount, currentSpeed); // 输出遥测到上位机
        }
    }
}
