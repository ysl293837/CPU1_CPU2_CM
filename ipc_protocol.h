#ifndef IPC_PROTOCOL_H
#define IPC_PROTOCOL_H

#include <stdint.h>

/* 1. 定义 CPU1、CPU2 和 CM 共用的 IPC 命令号 */
#define IPC_CMD_MOTOR_TELEMETRY    0x1001U                // CPU1 发送编码器和速度遥测数据
#define IPC_CMD_MOTOR_STATUS       0x1002U                // CPU1 发送电机运行状态和目标速度
#define IPC_CMD_MOTOR_START        0x1101U                // 请求 CPU1 启动电机闭环
#define IPC_CMD_MOTOR_STOP         0x1102U                // 请求 CPU1 停止电机输出
#define IPC_CMD_MOTOR_ESTOP        0x1103U                // 请求 CPU1 进入急停状态
#define IPC_CMD_MOTOR_CLEAR_ESTOP  0x1104U                // 请求 CPU1 清除急停状态
#define IPC_CMD_MOTOR_RESET_PID    0x1105U                // 请求 CPU1 复位速度 PI 控制器
#define IPC_CMD_MOTOR_SET_TARGET   0x1106U                // 请求 CPU1 设置目标速度
#define IPC_CMD_MOTOR_SET_KP       0x1107U                // 请求 CPU1 设置速度 PI 比例参数
#define IPC_CMD_MOTOR_SET_KI       0x1108U                // 请求 CPU1 设置速度 PI 积分参数
#define IPC_CMD_MOTOR_SET_KD       0x1109U                // 请求 CPU1 设置速度 PI 微分参数
#define IPC_CMD_MOTOR_ACK          0x11FFU                // CPU1 返回命令执行结果

/* 2. 定义电机状态帧中的状态位 */
#define IPC_MOTOR_STATUS_RUN_ENABLED  (1UL << 0)          // 电机闭环运行使能状态位
#define IPC_MOTOR_STATUS_ESTOP_ACTIVE (1UL << 1)          // 电机急停状态位

/* 3. 定义 CPU1 命令执行结果 */
#define IPC_RESULT_OK               0U                    // 命令执行成功
#define IPC_RESULT_ESTOP_ACTIVE     1U                    // 急停有效，拒绝启动或改速
#define IPC_RESULT_BAD_COMMAND      2U                    // 未知命令
#define IPC_RESULT_BAD_VALUE        3U                    // 参数值无效
#define IPC_RESULT_PENDING          0xFFFFFFFFUL          // 命令已经提交，等待 CPU1 执行结果

#define IPC_TELEMETRY_TICK_DIVIDER 100U                   // ePWM 中断计数达到该值后发送一帧遥测

#endif /* IPC_PROTOCOL_H */
