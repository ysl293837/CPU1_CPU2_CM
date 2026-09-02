#ifndef ECAT_MOTOR_APP_H
#define ECAT_MOTOR_APP_H

#include <stdbool.h>
#include <stdint.h>

/* 1. 定义准备由 SSC 映射的 EtherCAT RxPDO 命令格式 */
typedef struct
{
    uint32_t command;                                     // 0x7000:01，电机 IPC 命令号
    uint32_t dataw1;                                      // 0x7000:02，第一个 32 位命令参数
    uint32_t dataw2;                                      // 0x7000:03，第二个 32 位命令参数
    uint32_t sequence;                                    // 0x7000:04，命令变化序号
} ECAT_MotorCommandRxPDO_t;

/* 2. 定义准备由 SSC 映射的 EtherCAT 状态 TxPDO 格式 */
typedef struct
{
    uint32_t motorStatus;                                 // 0x6001:01，运行和急停状态位
    float targetSpeed;                                    // 0x6001:02，CPU1 当前目标机械转速
    uint32_t lastCommand;                                 // 0x6001:03，最近提交或确认的命令号
    uint32_t lastCommandResult;                           // 0x6001:04，CPU1 返回的命令结果
    uint32_t commandTxCount;                              // 0x6001:05，CM 成功转发的命令数
    uint32_t commandDropCount;                            // 0x6001:06，CM 丢弃的命令数
} ECAT_MotorStatusTxPDO_t;

bool ECAT_MotorCommand_Request(uint32_t command, uint32_t dataw1,
                               uint32_t dataw2);          // 将一个通用命令转发给 CPU1
bool ECAT_MotorCommand_RequestFloat(uint32_t command, float value); // 将一个浮点参数命令转发给 CPU1
bool ECAT_MotorCommand_ApplyRxPDO(
    const volatile ECAT_MotorCommandRxPDO_t *rxpdo);      // 解析新 RxPDO 并转发给 CPU1

#endif /* ECAT_MOTOR_APP_H */
