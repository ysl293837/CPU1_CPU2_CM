#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "../../../ipc_protocol.h"
#include "ipc_cpu1.h"
#include "../Closed_loop/speed_pi.h"
#include "../key/key.h"

static volatile IPC_MessageQueue_t g_ipcQueue;           // CPU1 与 CPU2 共用的消息队列对象
static volatile IPC_MessageQueue_t g_cmIpcQueue;         // CPU1 与 CM 共用的消息队列对象
static volatile uint16_t g_telemetryTick = 0U;           // ePWM 遥测分频计数器
static volatile bool g_telemetryPending = false;         // 已到遥测发送周期的标志

volatile uint32_t g_ipc_tx_count = 0UL;                  // CPU1→CPU2 成功发送的帧数
volatile uint32_t g_ipc_tx_drop_count = 0UL;             // CPU1→CPU2 因队列忙丢弃的帧数
volatile uint32_t g_cm_ipc_tx_count = 0UL;               // CPU1→CM 成功发送的帧数
volatile uint32_t g_cm_ipc_tx_drop_count = 0UL;          // CPU1→CM 因队列忙丢弃的帧数
volatile uint32_t g_cm_ipc_cmd_rx_count = 0UL;           // CM→CPU1 收到的有效命令数
volatile uint32_t g_cm_ipc_cmd_error_count = 0UL;        // CM→CPU1 收到的无效命令数
volatile uint32_t g_cm_ipc_cmd_ack_tx_count = 0UL;       // CPU1→CM 成功返回的命令应答数
volatile uint32_t g_cm_ipc_cmd_ack_drop_count = 0UL;     // CPU1→CM 丢弃的命令应答数

extern volatile uint16_t g_motor_run_enabled;            // 电机闭环运行使能，由主程序定义
extern volatile uint16_t g_motor_estop_active;           // 电机急停状态，由主程序定义
extern float uq;                                         // q 轴电压指令，由主程序定义

/* 执行 CPU2 或 CM 下发的电机命令 */
static uint32_t IPC_CPU1_ExecuteCommand(const IPC_Message_t *message)
{
    float value;                                         // 存放 IPC 中恢复出的单个浮点参数
    float kp;                                            // 临时保存新的比例系数
    float ki;                                            // 临时保存新的积分系数
    float kd;                                            // 临时保存新的微分系数

    switch(message->command)                             // 根据命令号选择电机控制动作
    {
        case IPC_CMD_MOTOR_START:                        // 处理启动闭环命令
            if(g_motor_estop_active != 0U)               // 急停有效时禁止启动
            {
                return IPC_RESULT_ESTOP_ACTIVE;          // 向发送端返回急停拒绝结果
            }
            g_motor_run_enabled = 1U;                    // 允许 ePWM ISR 运行速度环
            PMSM_PID_Enable(true);                       // 使能速度 PI 控制器
            return IPC_RESULT_OK;                        // 返回启动成功结果

        case IPC_CMD_MOTOR_STOP:                         // 处理正常停止命令
            g_motor_run_enabled = 0U;                    // 禁止 ePWM ISR 输出速度环结果
            Key_SetCommandSpeed(0.0f);                   // 将目标速度同步清零
            PMSM_PID_Reset();                            // 清除 PI 积分和历史误差
            uq = 0.0f;                                   // 立即将 q 轴电压指令清零
            return IPC_RESULT_OK;                        // 返回停止成功结果

        case IPC_CMD_MOTOR_ESTOP:                        // 处理急停命令
            g_motor_estop_active = 1U;                   // 锁定急停状态
            g_motor_run_enabled = 0U;                    // 禁止闭环输出
            Key_SetCommandSpeed(0.0f);                   // 将目标速度清零
            PMSM_PID_Reset();                            // 清除 PI 内部状态
            uq = 0.0f;                                   // 立即撤销 q 轴电压指令
            return IPC_RESULT_OK;                        // 返回急停执行成功结果

        case IPC_CMD_MOTOR_CLEAR_ESTOP:                  // 处理清除急停命令
            g_motor_estop_active = 0U;                   // 解除急停锁定
            g_motor_run_enabled = 0U;                    // 清除急停后仍保持停止
            PMSM_PID_Reset();                            // 清除停止期间的 PI 状态
            uq = 0.0f;                                   // 保持 q 轴电压为零
            return IPC_RESULT_OK;                        // 返回清除急停成功结果

        case IPC_CMD_MOTOR_RESET_PID:                    // 处理 PI 复位命令
            PMSM_PID_Reset();                            // 清除 PI 的积分、误差和输出
            uq = 0.0f;                                   // 清零当前 q 轴电压输出
            return IPC_RESULT_OK;                        // 返回 PI 复位成功结果

        case IPC_CMD_MOTOR_SET_TARGET:                   // 处理目标转速命令
            if(g_motor_estop_active != 0U)               // 急停有效时禁止修改速度
            {
                return IPC_RESULT_ESTOP_ACTIVE;          // 向发送端返回急停拒绝结果
            }
            memcpy(&value, &message->dataw1, sizeof(value)); // 从 dataw1 恢复 IEEE-754 浮点速度
            Key_SetCommandSpeed(value);                  // 限幅后写入按键和速度 PI 的目标速度
            return IPC_RESULT_OK;                        // 返回改速成功结果

        case IPC_CMD_MOTOR_SET_KP:                       // 处理比例系数命令
            if(g_motor_estop_active != 0U)               // 急停有效时禁止改参
            {
                return IPC_RESULT_ESTOP_ACTIVE;          // 向发送端返回急停拒绝结果
            }
            memcpy(&value, &message->dataw1, sizeof(value)); // 从 dataw1 恢复新的 Kp
            kp = value;                                  // 保存新的比例系数
            PMSM_SetPIDParams(kp, speedPID.Integral, speedPID.Derivative); // 保留 Ki/Kd 并更新 Kp
            return IPC_RESULT_OK;                        // 返回设置成功结果

        case IPC_CMD_MOTOR_SET_KI:                       // 处理积分系数命令
            if(g_motor_estop_active != 0U)               // 急停有效时禁止改参
            {
                return IPC_RESULT_ESTOP_ACTIVE;          // 向发送端返回急停拒绝结果
            }
            memcpy(&value, &message->dataw1, sizeof(value)); // 从 dataw1 恢复新的 Ki
            ki = value;                                  // 保存新的积分系数
            PMSM_SetPIDParams(speedPID.Proportion, ki, speedPID.Derivative); // 保留 Kp/Kd 并更新 Ki
            return IPC_RESULT_OK;                        // 返回设置成功结果

        case IPC_CMD_MOTOR_SET_KD:                       // 处理微分系数命令
            if(g_motor_estop_active != 0U)               // 急停有效时禁止改参
            {
                return IPC_RESULT_ESTOP_ACTIVE;          // 向发送端返回急停拒绝结果
            }
            memcpy(&value, &message->dataw1, sizeof(value)); // 从 dataw1 恢复新的 Kd
            kd = value;                                  // 保存新的微分系数
            PMSM_SetPIDParams(speedPID.Proportion, speedPID.Integral, kd); // 保留 Kp/Ki 并更新 Kd
            return IPC_RESULT_OK;                        // 返回设置成功结果

        default:                                         // 处理未定义的命令号
            return IPC_RESULT_BAD_COMMAND;               // 返回未知命令结果
    }
}

/* 向 CPU2 返回电机命令执行结果 */
static void IPC_CPU1_SendAck(uint32_t command, uint32_t result)
{
    IPC_Message_t response;                              // 创建返回 CPU2 的应答消息

    response.command = IPC_CMD_MOTOR_ACK;                // 标记此帧为命令应答
    response.address = 0UL;                              // 本消息不传递共享 RAM 地址
    response.dataw1 = command;                           // 返回原始命令号
    response.dataw2 = result;                            // 返回 CPU1 执行结果

    (void)IPC_sendMessageToQueue(IPC_CPU1_L_CPU2_R,      // 向 CPU2 队列发送应答
                                 &g_ipcQueue,             // 使用 CPU1↔CPU2 队列对象
                                 IPC_ADDR_CORRECTION_DISABLE, // 地址字段未使用，不做地址修正
                                 &response,               // 传入待发送的应答帧
                                 IPC_NONBLOCKING_CALL);   // 队列忙时不阻塞 CPU1
}

/* 向 CM 返回电机命令执行结果 */
static void IPC_CPU1_SendAckToCM(uint32_t command, uint32_t result)
{
    IPC_Message_t response;                              // 创建返回 CM 的应答消息

    response.command = IPC_CMD_MOTOR_ACK;                // 标记此帧为命令应答
    response.address = 0UL;                              // 本消息不传递共享 RAM 地址
    response.dataw1 = command;                           // 返回原始命令号
    response.dataw2 = result;                            // 返回 CPU1 执行结果

    if(IPC_sendMessageToQueue(IPC_CPU1_L_CM_R,           // 向 CM 队列发送应答
                              &g_cmIpcQueue,              // 使用 CPU1↔CM 队列对象
                              IPC_ADDR_CORRECTION_DISABLE, // 地址字段未使用，不做地址修正
                              &response,                  // 传入待发送的应答帧
                              IPC_NONBLOCKING_CALL))      // 队列忙时不阻塞 CPU1
    {
        g_cm_ipc_cmd_ack_tx_count++;                     // 记录成功发送的 CM 应答数
    }
    else
    {
        g_cm_ipc_cmd_ack_drop_count++;                   // 记录因队列忙丢弃的 CM 应答数
    }
}

/* 向 CM 上报运行状态和目标速度 */
static void IPC_CPU1_SendStatusToCM(void)
{
    IPC_Message_t message;                               // 创建 CPU1→CM 状态消息
    uint32_t statusFlags = 0UL;                           // 组合运行和急停状态位
    float targetSpeed = Key_GetCommandSpeed();           // 读取当前受限后的目标机械转速

    if(g_motor_run_enabled != 0U)                        // 判断闭环是否允许运行
    {
        statusFlags |= IPC_MOTOR_STATUS_RUN_ENABLED;     // 写入运行使能状态位
    }

    if(g_motor_estop_active != 0U)                       // 判断急停是否有效
    {
        statusFlags |= IPC_MOTOR_STATUS_ESTOP_ACTIVE;    // 写入急停状态位
    }

    message.command = IPC_CMD_MOTOR_STATUS;              // 标记此帧为电机状态帧
    message.address = 0UL;                               // 本消息不传递共享 RAM 地址
    message.dataw1 = statusFlags;                        // 在 dataw1 中放置状态位
    memcpy(&message.dataw2, &targetSpeed, sizeof(targetSpeed)); // 在 dataw2 中放置目标速度位模式

    if(IPC_sendMessageToQueue(IPC_CPU1_L_CM_R,           // 向 CM 队列发送状态帧
                              &g_cmIpcQueue,              // 使用 CPU1↔CM 队列对象
                              IPC_ADDR_CORRECTION_DISABLE, // 地址字段未使用，不做地址修正
                              &message,                   // 传入待发送的状态帧
                              IPC_NONBLOCKING_CALL))      // 队列忙时不阻塞 CPU1
    {
        g_cm_ipc_tx_count++;                             // 记录成功发送的 CM 帧数
    }
    else
    {
        g_cm_ipc_tx_drop_count++;                        // 记录因队列忙丢弃的 CM 帧数
    }
}

/* 将 SCIA 和 GPIO28/29 的控制权交给 CPU2 */
void IPC_CPU1_AssignSCIAtoCPU2(void)
{
    GPIO_setPinConfig(GPIO_28_SCIA_RX);                  // 将 GPIO28 复用为 SCIA 接收引脚
    GPIO_setPadConfig(28U, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP); // 为 RX 配置上拉输入
    GPIO_setQualificationMode(28U, GPIO_QUAL_ASYNC);     // RX 使用异步采样以适应串口波特率
    GPIO_setPinConfig(GPIO_29_SCIA_TX);                  // 将 GPIO29 复用为 SCIA 发送引脚
    GPIO_setPadConfig(29U, GPIO_PIN_TYPE_STD);           // 为 TX 配置标准输出焊盘
    GPIO_setQualificationMode(29U, GPIO_QUAL_ASYNC);     // TX 使用异步资格模式
    SysCtl_selectCPUForPeripheralInstance(SYSCTL_CPUSEL_SCIA, // 选择 SCIA 的外设控制核
                                          SYSCTL_CPUSEL_CPU2); // 将 SCIA 分配给 CPU2
    GPIO_setControllerCore(28U, GPIO_CORE_CPU2);         // 将 GPIO28 控制权交给 CPU2
    GPIO_setControllerCore(29U, GPIO_CORE_CPU2);         // 将 GPIO29 控制权交给 CPU2
}

/* 初始化 CPU1 与 CPU2 的消息队列 */
void IPC_CPU1_Init(void)
{
    IPC_initMessageQueue(IPC_CPU1_L_CPU2_R,              // 选择 CPU1↔CPU2 IPC 实例
                         &g_ipcQueue,                     // 绑定 CPU1↔CPU2 队列对象
                         IPC_INT1,                        // CPU1 接收 CPU2 消息使用本地 IPC1
                         IPC_INT0);                       // CPU2 接收 CPU1 消息使用远端 IPC0
}

/* 启动 CPU2 并完成一次启动同步 */
void IPC_CPU1_SyncWithCPU2(void)
{
    Device_bootCPU2(BOOT_MODE_CPU2);                     // 按配置的启动模式释放 CPU2
    IPC_sync(IPC_CPU1_L_CPU2_R, IPC_FLAG31);             // 与 CPU2 在 FLAG31 处会合
}

/* 初始化 CPU1 与 CM 的消息队列和接收中断 */
void IPC_CPU1_CM_Init(void)
{
    IPC_init(IPC_CPU1_L_CM_R);                           // 清除 CPU1→CM 方向遗留的 IPC 状态

    IPC_initMessageQueue(IPC_CPU1_L_CM_R,                // 选择 CPU1↔CM IPC 实例
                         &g_cmIpcQueue,                  // 绑定 CPU1↔CM 队列对象
                         IPC_INT1,                       // CPU1 接收 CM 消息使用本地 IPC1
                         IPC_INT1);                      // CM 接收 CPU1 消息使用远端 IPC1

    IPC_registerInterrupt(IPC_CPU1_L_CM_R,               // 指定 CPU1↔CM IPC 实例
                          IPC_INT1,                       // 选择 CM→CPU1 的 IPC1 中断通道
                          IPC_CPU1_CM_1_ISR);            // 注册 CPU1 的 CM 命令处理函数
}

/* 与 CM 完成一次启动同步 */
void IPC_CPU1_CM_SyncWithCM(void)
{
    IPC_sync(IPC_CPU1_L_CM_R, IPC_FLAG31);               // 在 FLAG31 等待 CM 的 IPC 初始化完成
}

/* 在 ePWM ISR 中产生低频遥测发送节拍 */
void IPC_CPU1_TickFromISR(void)
{
    g_telemetryTick++;                                   // 记录一次 ePWM 控制周期
    if(g_telemetryTick >= IPC_TELEMETRY_TICK_DIVIDER)    // 判断是否到达遥测分频周期
    {
        g_telemetryTick = 0U;                            // 重新开始下一轮分频计数
        g_telemetryPending = true;                       // 通知主循环发送一帧遥测
    }
}

/* 在 CPU1 主循环向 CPU2 和 CM 发送同源遥测 */
void IPC_CPU1_PollSend(int32_t encoderCount, float currentSpeed)
{
    IPC_Message_t message;                               // 创建待发送的遥测消息

    if(!g_telemetryPending)                              // 尚未到达遥测发送周期时直接返回
    {
        return;                                          // 保持主循环非阻塞
    }

    g_telemetryPending = false;                          // 消费本次待发送标志
    message.command = IPC_CMD_MOTOR_TELEMETRY;           // 标记此帧为编码器和速度遥测
    message.address = 0UL;                               // 本消息不传递共享 RAM 地址
    message.dataw1 = (uint32_t)encoderCount;             // 在 dataw1 放置有符号编码器计数
    memcpy(&message.dataw2, &currentSpeed, sizeof(currentSpeed)); // 在 dataw2 放置当前速度位模式

    if(IPC_sendMessageToQueue(IPC_CPU1_L_CPU2_R,         // 向 CPU2 发送串口遥测源数据
                              &g_ipcQueue,                // 使用 CPU1↔CPU2 队列对象
                              IPC_ADDR_CORRECTION_DISABLE, // 地址字段未使用，不做地址修正
                              &message,                   // 传入待发送的遥测帧
                              IPC_NONBLOCKING_CALL))      // 队列忙时不阻塞 CPU1
    {
        g_ipc_tx_count++;                                // 记录成功发送的 CPU2 帧数
    }
    else
    {
        g_ipc_tx_drop_count++;                           // 记录因队列忙丢弃的 CPU2 帧数
    }

    if(IPC_sendMessageToQueue(IPC_CPU1_L_CM_R,           // 向 CM 发送相同的遥测源数据
                              &g_cmIpcQueue,              // 使用 CPU1↔CM 队列对象
                              IPC_ADDR_CORRECTION_DISABLE, // 地址字段未使用，不做地址修正
                              &message,                   // 传入待发送的遥测帧
                              IPC_NONBLOCKING_CALL))      // 队列忙时不阻塞 CPU1
    {
        g_cm_ipc_tx_count++;                             // 记录成功发送的 CM 遥测帧数
    }
    else
    {
        g_cm_ipc_tx_drop_count++;                        // 记录因队列忙丢弃的 CM 遥测帧数
    }

    IPC_CPU1_SendStatusToCM();                           // 向 CM 额外上报运行状态和目标速度
}

/* 处理 CPU2 通过 IPC1 下发的电机命令 */
__interrupt void IPC_1_ISR(void)
{
    IPC_Message_t message;                               // 存放从 CPU2 队列读出的单条消息

    while(IPC_readMessageFromQueue(IPC_CPU1_L_CPU2_R,    // 持续读取 CPU2→CPU1 队列
                                   &g_ipcQueue,           // 使用 CPU1↔CPU2 队列对象
                                   IPC_ADDR_CORRECTION_DISABLE, // 地址字段未使用，不做地址修正
                                   &message,              // 保存读出的 IPC 消息
                                   IPC_NONBLOCKING_CALL)) // 队列为空时立即结束读取
    {
        if((message.command >= IPC_CMD_MOTOR_START) &&   // 判断是否为定义的电机命令
           (message.command <= IPC_CMD_MOTOR_SET_KD))
        {
            IPC_CPU1_SendAck(message.command,            // 返回原始命令号
                             IPC_CPU1_ExecuteCommand(&message)); // 执行命令并回传结果
        }
    }

    IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_FLAG1);       // 确认已处理 CPU2 发来的 IPC1 标志
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);        // 清除 CPU1↔CPU2 所在的 PIE Group 1
}

/* 处理 CM 通过 IPC1 下发的 EtherCAT 电机命令 */
__interrupt void IPC_CPU1_CM_1_ISR(void)
{
    IPC_Message_t message;                               // 存放从 CM 队列读出的单条消息

    while(IPC_readMessageFromQueue(IPC_CPU1_L_CM_R,      // 持续读取 CM→CPU1 队列
                                   &g_cmIpcQueue,         // 使用 CPU1↔CM 队列对象
                                   IPC_ADDR_CORRECTION_DISABLE, // 地址字段未使用，不做地址修正
                                   &message,              // 保存读出的 IPC 消息
                                   IPC_NONBLOCKING_CALL)) // 队列为空时立即结束读取
    {
        if((message.command >= IPC_CMD_MOTOR_START) &&   // 判断是否为定义的电机命令
           (message.command <= IPC_CMD_MOTOR_SET_KD))
        {
            g_cm_ipc_cmd_rx_count++;                     // 记录收到的有效 CM 命令
            IPC_CPU1_SendAckToCM(message.command,        // 返回原始命令号给 CM
                                 IPC_CPU1_ExecuteCommand(&message)); // 执行命令并回传结果
        }
        else
        {
            g_cm_ipc_cmd_error_count++;                  // 记录未定义的 CM 命令
        }
    }

    IPC_ackFlagRtoL(IPC_CPU1_L_CM_R, IPC_FLAG1);         // 确认已处理 CM 发来的 IPC1 标志
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP11);       // 清除 CM→CPU1 所在的 PIE Group 11
}
