#ifndef IPC_CPU2_H
#define IPC_CPU2_H

#include <stdbool.h>
#include <stdint.h>

extern volatile uint32_t g_ipc_rx_count;                  // CPU2 收到的 CPU1 IPC 帧数
extern volatile uint32_t g_ipc_last_command;              // CPU2 最近收到的 CPU1 命令号
extern volatile int32_t g_received_encoder_count;         // CPU2 最近收到的编码器计数
extern volatile float g_received_current_speed;           // CPU2 最近收到的机械转速
extern volatile bool g_ipc_telemetry_pending;             // CPU2 是否有未消费的遥测帧

void IPC_CPU2_Init(void);                                 // 初始化 CPU2↔CPU1 IPC 消息队列
void IPC_CPU2_SyncWithCPU1(void);                         // 使用 FLAG31 与 CPU1 同步启动
bool IPC_CPU2_SendCommand(uint32_t command, uint32_t dataw1, uint32_t dataw2); // 向 CPU1 发送一条电机命令
bool IPC_CPU2_TakeTelemetry(int32_t *encoderCount, float *currentSpeed); // 取出一帧最新 CPU1 遥测
bool IPC_CPU2_TakeAck(uint32_t *command, uint32_t *result); // 取出一条 CPU1 命令应答
__interrupt void IPC_0_ISR(void);                         // 处理 CPU1→CPU2 IPC0 消息

#endif /* IPC_CPU2_H */
