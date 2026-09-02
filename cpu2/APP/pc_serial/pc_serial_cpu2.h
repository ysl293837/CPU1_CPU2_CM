#ifndef PC_SERIAL_CPU2_H
#define PC_SERIAL_CPU2_H

#include <stdbool.h>
#include <stdint.h>

void PC_Serial_CPU2_Init(void);                           // 初始化 CPU2 SCIA 命令和遥测服务
void PC_Serial_CPU2_ProcessRx(void);                      // 拼接并解析上位机接收命令
void PC_Serial_CPU2_ProcessAcks(void);                    // 输出 CPU1 返回的命令应答
bool PC_Serial_CPU2_StreamDue(void);                      // 判断是否到达下一次遥测输出周期
void PC_Serial_CPU2_SendTelemetry(int32_t encoderCount, float currentSpeed); // 输出编码器和速度遥测
void PC_Serial_CPU2_SendStatus(void);                     // 输出完整诊断状态
__interrupt void PC_Serial_CPU2_RXISR(void);              // 读取 SCIA 硬件接收 FIFO
__interrupt void INT_mySCI1_RX_ISR(void);                 // 应答 SysConfig 生成的 SCIA 接收中断

#endif /* PC_SERIAL_CPU2_H */
