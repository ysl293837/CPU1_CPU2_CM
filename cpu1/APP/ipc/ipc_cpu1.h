#ifndef IPC_CPU1_H
#define IPC_CPU1_H

#include <stdint.h>

extern volatile uint32_t g_ipc_tx_count;                  // CPU1→CPU2 成功发送帧数
extern volatile uint32_t g_ipc_tx_drop_count;             // CPU1→CPU2 丢弃帧数
extern volatile uint32_t g_cm_ipc_tx_count;               // CPU1→CM 成功发送帧数
extern volatile uint32_t g_cm_ipc_tx_drop_count;          // CPU1→CM 丢弃帧数
extern volatile uint32_t g_cm_ipc_cmd_rx_count;           // CM→CPU1 有效命令数
extern volatile uint32_t g_cm_ipc_cmd_error_count;        // CM→CPU1 无效命令数
extern volatile uint32_t g_cm_ipc_cmd_ack_tx_count;       // CPU1→CM 成功应答数
extern volatile uint32_t g_cm_ipc_cmd_ack_drop_count;     // CPU1→CM 丢弃应答数

void IPC_CPU1_AssignSCIAtoCPU2(void);                     // 将 SCIA 和 GPIO28/29 控制权交给 CPU2
void IPC_CPU1_Init(void);                                 // 初始化 CPU1↔CPU2 IPC 消息队列
void IPC_CPU1_SyncWithCPU2(void);                         // 使用 FLAG31 与 CPU2 同步启动
void IPC_CPU1_CM_Init(void);                              // 初始化 CPU1↔CM IPC 队列和中断
void IPC_CPU1_CM_SyncWithCM(void);                        // 使用 FLAG31 与 CM 同步启动
void IPC_CPU1_TickFromISR(void);                          // 在 ePWM ISR 中生成低频遥测节拍
void IPC_CPU1_PollSend(int32_t encoderCount, float currentSpeed); // 在主循环发送 CPU2 和 CM 遥测
__interrupt void IPC_1_ISR(void);                         // 处理 CPU2→CPU1 IPC1 命令
__interrupt void IPC_CPU1_CM_1_ISR(void);                 // 处理 CM→CPU1 IPC1 命令

#endif /* IPC_CPU1_H */
