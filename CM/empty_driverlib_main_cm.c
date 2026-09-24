//#############################################################################
//
// FILE:   empty_driverlib_main_cm.c
//
// TITLE:  CM EtherCAT and IPC application
//
//! \addtogroup driver_example_list
//! <h1> CM EtherCAT and IPC Application </h1>
//
// C2000Ware v26.00.00.00
//
// Copyright (C) 2024 Texas Instruments Incorporated - http://www.ti.com
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of Texas Instruments Incorporated nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//#############################################################################

#include <string.h>

#include "cpu.h"
#include "driverlib_cm.h"
#include "cm.h"
#include "ethercat_slave_cm_hal.h"
#include "applInterface.h"
#include "../ipc_protocol.h"
#include "ecat_motor_app.h"

volatile uint16_t g_ecat_hw_init_status = ESC_HW_INIT_FAIL; // 保存 ESC 硬件初始化结果
volatile uint32_t g_cm_ipc_rx_count = 0UL;                 // CM 收到的 CPU1 遥测帧数
volatile uint32_t g_cm_ipc_error_count = 0UL;              // CM 收到的未知 IPC 帧数
volatile uint32_t g_cm_ipc_status_rx_count = 0UL;          // CM 收到的 CPU1 状态帧数
volatile uint32_t g_cm_ipc_cmd_tx_count = 0UL;             // CM 成功转发到 CPU1 的命令数
volatile uint32_t g_cm_ipc_cmd_drop_count = 0UL;           // CM 因 IPC 队列忙丢弃的命令数
volatile uint32_t g_cm_ipc_cmd_ack_count = 0UL;            // CM 收到的 CPU1 命令应答数
volatile uint32_t g_cm_ipc_cmd_invalid_count = 0UL;        // CM 拒绝的无效 EtherCAT 命令数
volatile int32_t g_cm_last_encoder_count = 0L;             // 最近从 CPU1 收到的编码器计数
volatile float g_cm_last_current_speed = 0.0f;             // 最近从 CPU1 收到的机械转速
volatile uint32_t g_cm_motor_status_flags = 0UL;           // 最近从 CPU1 收到的电机状态位
volatile float g_cm_target_speed = 0.0f;                   // 最近从 CPU1 收到的目标速度
volatile uint32_t g_cm_last_command = 0UL;                 // 最近转发给 CPU1 的命令号
volatile uint32_t g_cm_last_command_result = IPC_RESULT_PENDING; // 最近命令的执行结果
volatile uint32_t g_ecat_pdo_update_count = 0UL;           // 遥测 TxPDO 影子区更新次数
volatile uint32_t g_ecat_status_update_count = 0UL;        // 状态 TxPDO 影子区更新次数
volatile uint32_t g_ecat_command_apply_count = 0UL;        // RxPDO 命令成功应用次数
volatile uint16_t g_ecat_al_state = 0U;                    // 当前 EtherCAT 应用层状态机状态

volatile ECAT_MotorTelemetryTxPDO_t g_ecat_motor_telemetry_txpdo = {0L, 0.0f}; // CPU1 遥测的 EtherCAT 影子 TxPDO
volatile ECAT_MotorCommandRxPDO_t g_ecat_motor_command_rxpdo = {0UL, 0UL, 0UL, 0UL}; // 等待 SSC 写入的 EtherCAT 命令影子 RxPDO
volatile ECAT_MotorStatusTxPDO_t g_ecat_motor_status_txpdo = // 等待 SSC 读取的电机状态影子 TxPDO
{
    0UL,                                                   // 初始电机状态位
    0.0f,                                                  // 初始目标速度
    0UL,                                                   // 初始最近命令号
    IPC_RESULT_PENDING,                                    // 初始命令结果为等待状态
    0UL,                                                   // 初始命令发送计数
    0UL                                                    // 初始命令丢弃计数
};

#define ECAT_AL_STATUS_REGISTER 0x0130U                   // ESC 应用层状态寄存器偏移地址

static volatile IPC_MessageQueue_t g_cmIpcQueue;          // CM↔CPU1 共用的 IPC 消息队列对象
static volatile uint32_t g_cm_telemetry_sequence = 0UL;   // 遥测快照序号，奇数表示 ISR 正在写入
static volatile uint32_t g_cm_status_sequence = 0UL;      // 状态快照序号，奇数表示 ISR 正在写入
static uint32_t g_ecat_last_snapshot_sequence = 0UL;      // 最近映射到遥测 TxPDO 的快照序号
static uint32_t g_ecat_last_status_sequence = 0UL;        // 最近映射到状态 TxPDO 的快照序号
static uint32_t g_ecat_last_command_ack_count = 0UL;      // 最近映射的命令应答计数
static uint32_t g_ecat_last_command_tx_count = 0UL;       // 最近映射的命令发送计数
static uint32_t g_ecat_last_command_drop_count = 0UL;     // 最近映射的命令丢弃计数
static bool g_ecat_rxpdo_sequence_valid = false;          // 表示已经保存过 RxPDO 命令序号
static uint32_t g_ecat_last_rxpdo_sequence = 0UL;         // 最近已经执行的 RxPDO 命令序号

static void IPC_CM_1_ISR(void);                           // 声明 CPU1→CM IPC1 接收中断函数
static void IPC_CM_Init(void);                            // 声明 CM IPC 初始化函数
static void IPC_CM_SyncWithCPU1(void);                    // 声明 CM 与 CPU1 启动同步函数
static void ECAT_Telemetry_UpdateTxPDOShadow(void);       // 声明遥测 TxPDO 影子区更新函数
static void ECAT_Status_UpdateTxPDOShadow(void);          // 声明状态 TxPDO 影子区更新函数
static bool ECAT_MotorCommand_IsValid(uint32_t command);  // 声明 EtherCAT 命令合法性检查函数

/* 2. 初始化 CM 与 CPU1 的 IPC 队列和接收中断 */
static void IPC_CM_Init(void)
{
    IPC_init(IPC_CM_L_CPU1_R);                            // 清除 CM→CPU1 方向可能残留的 IPC 状态
    IPC_initMessageQueue(IPC_CM_L_CPU1_R,                 // 选择 CM↔CPU1 IPC 实例
                         &g_cmIpcQueue,                   // 绑定 CM↔CPU1 队列对象
                         IPC_INT1,                        // CM 接收 CPU1 消息使用本地 IPC1
                         IPC_INT1);                       // CPU1 接收 CM 命令使用远端 IPC1
    IPC_registerInterrupt(IPC_CM_L_CPU1_R,                // 指定 CM↔CPU1 IPC 实例
                          IPC_INT1,                       // 选择 CPU1→CM 的 IPC1 接收通道
                          IPC_CM_1_ISR);                  // 注册 CM 的实际中断处理函数
}

/* 3. 在启动阶段等待 CPU1 与 CM 都完成初始化 */
static void IPC_CM_SyncWithCPU1(void)
{
    IPC_sync(IPC_CM_L_CPU1_R, IPC_FLAG31);                // 使用 FLAG31 作为一次性的启动会合点
}

/* 4. 接收 CPU1 的遥测、状态和命令应答 */
static void IPC_CM_1_ISR(void)
{
    IPC_Message_t message;                                // 保存从 CPU1 队列读出的一帧消息
    float currentSpeed;                                   // 保存由消息位模式还原的浮点速度

    while(IPC_readMessageFromQueue(IPC_CM_L_CPU1_R,      // 持续读取直到 CPU1 队列已清空
                                   &g_cmIpcQueue,         // 使用 CM↔CPU1 队列对象
                                   IPC_ADDR_CORRECTION_DISABLE, // 不对未使用的地址字段修正
                                   &message,              // 将当前 IPC 帧写入本地变量
                                   IPC_NONBLOCKING_CALL)) // 无消息时结束读取循环
    {
        if(message.command == IPC_CMD_MOTOR_TELEMETRY)    // 处理 CPU1 周期遥测帧
        {
            g_cm_telemetry_sequence++;                    // 写入前将遥测序号改为奇数
            g_cm_last_encoder_count = (int32_t)message.dataw1; // 保存编码器计数
            memcpy(&currentSpeed, &message.dataw2, sizeof(currentSpeed)); // 从 dataw2 恢复 IEEE-754 速度
            g_cm_last_current_speed = currentSpeed;       // 保存当前机械转速
            g_cm_telemetry_sequence++;                    // 写入完成后将遥测序号改回偶数
            g_cm_ipc_rx_count++;                          // 记录收到的一帧 CPU1 遥测
        }
        else if(message.command == IPC_CMD_MOTOR_STATUS)  // 处理 CPU1 电机状态帧
        {
            g_cm_status_sequence++;                       // 写入前将状态序号改为奇数
            g_cm_motor_status_flags = message.dataw1;     // 保存运行和急停状态位
            memcpy(&currentSpeed, &message.dataw2, sizeof(currentSpeed)); // 从 dataw2 恢复目标速度
            g_cm_target_speed = currentSpeed;             // 保存 CPU1 当前目标速度
            g_cm_status_sequence++;                       // 写入完成后将状态序号改回偶数
            g_cm_ipc_status_rx_count++;                   // 记录收到的一帧 CPU1 状态
        }
        else if(message.command == IPC_CMD_MOTOR_ACK)     // 处理 CPU1 命令执行应答
        {
            g_cm_last_command = message.dataw1;           // 保存已执行的原始命令号
            g_cm_last_command_result = message.dataw2;    // 保存 CPU1 返回的执行结果
            g_cm_ipc_cmd_ack_count++;                     // 记录收到的命令应答次数
        }
        else
        {
            g_cm_ipc_error_count++;                       // 记录未识别的 IPC 消息帧
        }
    }

    IPC_ackFlagRtoL(IPC_CM_L_CPU1_R, IPC_FLAG1);          // 应答 CPU1→CM 队列对应的 FLAG1
}

/* 5. 检查 EtherCAT 下发的命令是否属于电机命令范围 */
static bool ECAT_MotorCommand_IsValid(uint32_t command)
{
    return ((command >= IPC_CMD_MOTOR_START) &&           // 命令号不得小于启动命令
            (command <= IPC_CMD_MOTOR_SET_KD));           // 命令号不得大于 Kd 设置命令
}

/* 6. 将一条 EtherCAT 命令转发给 CPU1 */
bool ECAT_MotorCommand_Request(uint32_t command, uint32_t dataw1,
                               uint32_t dataw2)
{
    IPC_Message_t message;                                // 创建待发送给 CPU1 的 IPC 命令帧

    if(!ECAT_MotorCommand_IsValid(command))               // 先拒绝未定义的命令号
    {
        g_cm_ipc_cmd_invalid_count++;                     // 记录无效 EtherCAT 命令
        return false;                                     // 不向 CPU1 转发无效命令
    }

    message.command = command;                            // 写入电机命令号
    message.address = 0UL;                                // 本工程不使用共享 RAM 地址字段
    message.dataw1 = dataw1;                              // 写入第一个 32 位命令参数
    message.dataw2 = dataw2;                              // 写入第二个 32 位命令参数

    if(!IPC_sendMessageToQueue(IPC_CM_L_CPU1_R,           // 将命令压入 CM→CPU1 队列
                               &g_cmIpcQueue,             // 使用 CM↔CPU1 队列对象
                               IPC_ADDR_CORRECTION_DISABLE, // 不对未使用的地址字段修正
                               &message,                  // 传入待发送的命令帧
                               IPC_NONBLOCKING_CALL))     // 队列忙时立即返回失败
    {
        g_cm_ipc_cmd_drop_count++;                        // 记录因队列忙导致的命令丢弃
        return false;                                     // 告知 SSC 或调用者本次命令未提交
    }

    g_cm_last_command = command;                          // 保存最近提交给 CPU1 的命令号
    g_cm_last_command_result = IPC_RESULT_PENDING;        // 等待 CPU1 通过 ACK 返回最终结果
    g_cm_ipc_cmd_tx_count++;                              // 记录一次成功的命令提交
    return true;                                          // 告知调用者命令已经进入 IPC 队列
}

/* 7. 将浮点命令参数编码为 IPC 位模式 */
bool ECAT_MotorCommand_RequestFloat(uint32_t command, float value)
{
    uint32_t valueBits;                                   // 保存浮点数的 IEEE-754 位模式

    memcpy(&valueBits, &value, sizeof(valueBits));        // 直接复制浮点位模式而不改变数值
    return ECAT_MotorCommand_Request(command, valueBits, 0UL); // 将位模式作为 dataw1 发送给 CPU1
}

/* 8. 检查 RxPDO 序号并将新命令转发给 CPU1 */
bool ECAT_MotorCommand_ApplyRxPDO(
    const volatile ECAT_MotorCommandRxPDO_t *rxpdo)
{
    if(rxpdo == 0)                                        // 检查 SSC 传入的 RxPDO 指针是否有效
    {
        g_cm_ipc_cmd_invalid_count++;                     // 记录一次无效命令输入
        return false;                                     // 空指针无法解析命令
    }

    if(rxpdo->command == 0UL)                             // 零命令表示 SSC 尚未写入有效 RxPDO
    {
        return true;                                      // 空闲状态不需要向 CPU1 发送消息
    }

    if(g_ecat_rxpdo_sequence_valid &&                     // 已经执行过至少一条 RxPDO 命令
       (rxpdo->sequence == g_ecat_last_rxpdo_sequence))   // 本周期命令序号未改变
    {
        return true;                                      // EtherCAT 循环帧重复时不重复执行命令
    }

    if(!ECAT_MotorCommand_Request(rxpdo->command,         // 将新的 RxPDO 命令转发给 CPU1
                                  rxpdo->dataw1,
                                  rxpdo->dataw2))
    {
        return false;                                     // 队列忙或命令非法时保留原序号等待重试
    }

    g_ecat_last_rxpdo_sequence = rxpdo->sequence;         // 记录已经提交的命令序号
    g_ecat_rxpdo_sequence_valid = true;                   // 标记后续可进行重复序号过滤
    g_ecat_command_apply_count++;                         // 记录一次成功的 RxPDO 命令应用
    return true;                                          // 告知调用者命令已经成功提交
}

/* 9. 将 CPU1 遥测快照更新到 EtherCAT TxPDO 影子区 */
static void ECAT_Telemetry_UpdateTxPDOShadow(void)
{
    uint32_t sequenceBefore;                              // 读取快照前的遥测序号
    uint32_t sequenceAfter;                               // 读取快照后的遥测序号
    int32_t encoderCount;                                 // 临时保存完整的编码器计数
    float currentSpeed;                                   // 临时保存完整的机械转速

    do                                                    // 使用序号锁读取不被 ISR 打断的一致快照
    {
        sequenceBefore = g_cm_telemetry_sequence;         // 读取 ISR 写入前后的版本号
        if((sequenceBefore & 1UL) != 0UL)                 // 奇数表示 IPC ISR 正在更新数据
        {
            return;                                      // 下一个主循环周期再重试读取
        }

        encoderCount = g_cm_last_encoder_count;           // 读取当前编码器计数快照
        currentSpeed = g_cm_last_current_speed;           // 读取当前机械转速快照
        sequenceAfter = g_cm_telemetry_sequence;          // 再读一次版本号确认数据未变化
    }
    while((sequenceBefore != sequenceAfter) ||            // ISR 写入过数据时重新读取
          ((sequenceAfter & 1UL) != 0UL));                // ISR 仍在写入时重新读取

    if(sequenceAfter != g_ecat_last_snapshot_sequence)    // 仅在 CPU1 有新遥测时更新 TxPDO
    {
        g_ecat_motor_telemetry_txpdo.encoderCount = encoderCount; // 更新 SSC 可映射的编码器字段
        g_ecat_motor_telemetry_txpdo.currentSpeed = currentSpeed; // 更新 SSC 可映射的转速字段
        g_ecat_last_snapshot_sequence = sequenceAfter;    // 记录已经映射的遥测序号
        g_ecat_pdo_update_count++;                        // 记录一次遥测 TxPDO 更新
    }
}

/* 10. 将 CPU1 状态和命令统计更新到 EtherCAT TxPDO 影子区 */
static void ECAT_Status_UpdateTxPDOShadow(void)
{
    uint32_t sequenceBefore;                              // 读取快照前的状态序号
    uint32_t sequenceAfter;                               // 读取快照后的状态序号
    uint32_t motorStatus;                                 // 临时保存电机状态位
    float targetSpeed;                                    // 临时保存目标机械转速
    uint32_t lastCommand;                                 // 临时保存最近命令号
    uint32_t lastCommandResult;                           // 临时保存最近命令结果
    uint32_t commandTxCount;                              // 临时保存命令发送计数
    uint32_t commandDropCount;                            // 临时保存命令丢弃计数

    do                                                    // 使用序号锁读取不被 ISR 打断的一致状态快照
    {
        sequenceBefore = g_cm_status_sequence;            // 读取当前状态版本号
        if((sequenceBefore & 1UL) != 0UL)                 // 奇数表示 IPC ISR 正在更新状态
        {
            return;                                      // 下一个主循环周期再重试读取
        }

        motorStatus = g_cm_motor_status_flags;            // 读取运行和急停状态位
        targetSpeed = g_cm_target_speed;                  // 读取 CPU1 当前目标速度
        sequenceAfter = g_cm_status_sequence;             // 再读一次版本号确认数据未变化
    }
    while((sequenceBefore != sequenceAfter) ||            // ISR 写入过状态时重新读取
          ((sequenceAfter & 1UL) != 0UL));                // ISR 仍在写入时重新读取

    lastCommand = g_cm_last_command;                      // 获取最近提交或确认的命令号
    lastCommandResult = g_cm_last_command_result;         // 获取该命令最新执行结果
    commandTxCount = g_cm_ipc_cmd_tx_count;               // 获取累计成功转发的命令数
    commandDropCount = g_cm_ipc_cmd_drop_count;           // 获取累计丢弃的命令数

    if((sequenceAfter != g_ecat_last_status_sequence) ||  // CPU1 状态发生变化时更新
       (g_cm_ipc_cmd_ack_count != g_ecat_last_command_ack_count) || // CPU1 应答数量变化时更新
       (commandTxCount != g_ecat_last_command_tx_count) || // 命令提交数量变化时更新
       (commandDropCount != g_ecat_last_command_drop_count)) // 命令丢弃数量变化时更新
    {
        g_ecat_motor_status_txpdo.motorStatus = motorStatus; // 更新 SSC 可映射的电机状态
        g_ecat_motor_status_txpdo.targetSpeed = targetSpeed; // 更新 SSC 可映射的目标速度
        g_ecat_motor_status_txpdo.lastCommand = lastCommand; // 更新 SSC 可映射的最近命令号
        g_ecat_motor_status_txpdo.lastCommandResult = lastCommandResult; // 更新 SSC 可映射的命令结果
        g_ecat_motor_status_txpdo.commandTxCount = commandTxCount; // 更新 SSC 可映射的成功命令数
        g_ecat_motor_status_txpdo.commandDropCount = commandDropCount; // 更新 SSC 可映射的丢弃命令数
        g_ecat_last_status_sequence = sequenceAfter;      // 记录已经映射的状态序号
        g_ecat_last_command_ack_count = g_cm_ipc_cmd_ack_count; // 记录已经映射的应答计数
        g_ecat_last_command_tx_count = commandTxCount;    // 记录已经映射的发送计数
        g_ecat_last_command_drop_count = commandDropCount; // 记录已经映射的丢弃计数
        g_ecat_status_update_count++;                     // 记录一次状态 TxPDO 更新
    }
}

/* 11. 供 SSC 的 APPL_Application 每周期调用，桥接 EtherCAT 与 CPU1 */
void ECAT_MotorApplication_Process(void)
{
    (void)ECAT_MotorCommand_ApplyRxPDO(&g_ecat_motor_command_rxpdo); // 仅在 RxPDO 序号变化时转发一次命令
    ECAT_Telemetry_UpdateTxPDOShadow();                   // 刷新来自 CPU1 的编码器和转速数据
    ECAT_Status_UpdateTxPDOShadow();                      // 刷新运行状态、目标速度和命令应答

    if(g_ecat_hw_init_status == ESC_HW_INIT_SUCCESS)      // 仅在 ESC 可访问时读取 AL 状态机
    {
        g_ecat_al_state = ESC_readWord(ECAT_AL_STATUS_REGISTER) & 0x000FU; // 保存 INIT/PREOP/SAFEOP/OP 状态
    }
}

/* 12. 初始化 CM、IPC、ESC 硬件与完整 SSC 从站协议栈 */
void main(void)
{
    CM_init();                                            // 初始化 CM 内核、向量表和基础时钟
    IPC_CM_Init();                                        // 初始化 CM↔CPU1 消息队列和接收中断
    IPC_CM_SyncWithCPU1();                                // 在 FLAG31 等待 CPU1 启动 CM 后完成同步
    CPU_clearPRIMASK();                                   // 打开 CM 中断响应以接收 CPU1 遥测
    g_ecat_hw_init_status = ESC_initHW();                 // 初始化 ESC 硬件访问、时钟和寄存器接口

    if(g_ecat_hw_init_status == ESC_HW_INIT_SUCCESS)      // ESC 硬件成功初始化后才能启动 SSC 状态机
    {
        (void)MainInit();                                 // 初始化 SSC 的 AL、邮箱、CoE 和 PDO 状态机
        bRunApplication = TRUE;                           // 允许 SSC 主循环开始处理 EtherCAT 报文

        while(bRunApplication == TRUE)                    // 持续执行完整 EtherCAT 从站协议栈
        {
            MainLoop();                                   // 处理状态转换、SDO/CoE、PDO 和应用层回调
        }
    }

    while(1)                                              // ESC 初始化失败时保持 CM 可调试且仍接收 CPU1 IPC
    {
        ECAT_MotorApplication_Process();                  // 持续更新诊断影子数据，不启动失效的协议栈
    }
}
