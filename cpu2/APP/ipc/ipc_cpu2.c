#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driverlib.h"
#include "board.h"
#include "../../../ipc_protocol.h"
#include "ipc_cpu2.h"

#define IPC_ACK_FIFO_SIZE 8U                              // 缓存 CPU1 命令应答的最大帧数

static volatile IPC_MessageQueue_t g_ipcQueue;            // CPU2↔CPU1 共用的消息队列对象
static volatile uint32_t g_ackCommand[IPC_ACK_FIFO_SIZE]; // 命令应答 FIFO 的命令号缓存
static volatile uint32_t g_ackResult[IPC_ACK_FIFO_SIZE];  // 命令应答 FIFO 的结果缓存
static volatile uint16_t g_ackHead = 0U;                  // 命令应答 FIFO 的写入位置
static volatile uint16_t g_ackTail = 0U;                  // 命令应答 FIFO 的读取位置
static volatile uint16_t g_ackCount = 0U;                 // 命令应答 FIFO 中的有效帧数

volatile uint32_t g_ipc_rx_count = 0UL;                   // CPU2 收到的 CPU1 IPC 帧总数
volatile uint32_t g_ipc_last_command = 0UL;               // 最近收到的 CPU1 IPC 命令号
volatile int32_t g_received_encoder_count = 0L;           // 最近收到的编码器计数
volatile float g_received_current_speed = 0.0f;           // 最近收到的机械转速
volatile bool g_ipc_telemetry_pending = false;            // 表示主循环可读取一帧新遥测

/* 1. 初始化 CPU2 与 CPU1 的 IPC 消息队列 */
void IPC_CPU2_Init(void)
{
    IPC_initMessageQueue(IPC_CPU2_L_CPU1_R,               // 选择 CPU2↔CPU1 IPC 实例
                         &g_ipcQueue,                     // 绑定 CPU2↔CPU1 队列对象
                         IPC_INT0,                        // CPU2 接收 CPU1 消息使用本地 IPC0
                         IPC_INT1);                       // CPU1 接收 CPU2 消息使用远端 IPC1
}

/* 2. 在启动阶段与 CPU1 会合 */
void IPC_CPU2_SyncWithCPU1(void)
{
    IPC_sync(IPC_CPU2_L_CPU1_R, IPC_FLAG31);              // 在 FLAG31 等待 CPU1 完成初始化
}

/* 3. 向 CPU1 非阻塞发送一条电机命令 */
bool IPC_CPU2_SendCommand(uint32_t command, uint32_t dataw1, uint32_t dataw2)
{
    IPC_Message_t message;                                // 创建待发送的 IPC 消息帧

    message.command = command;                            // 写入电机命令号
    message.address = 0UL;                                // 本工程不使用共享 RAM 地址字段
    message.dataw1 = dataw1;                              // 写入第一个 32 位命令参数
    message.dataw2 = dataw2;                              // 写入第二个 32 位命令参数

    return IPC_sendMessageToQueue(IPC_CPU2_L_CPU1_R,     // 将命令压入 CPU2→CPU1 队列
                                  &g_ipcQueue,            // 使用 CPU2↔CPU1 队列对象
                                  IPC_ADDR_CORRECTION_DISABLE, // 不对未使用的地址字段修正
                                  &message,               // 传入待发送的消息帧
                                  IPC_NONBLOCKING_CALL);  // 队列忙时立即返回失败而不阻塞串口
}

/* 4. 从 ISR 安全取出最新遥测 */
bool IPC_CPU2_TakeTelemetry(int32_t *encoderCount, float *currentSpeed)
{
    bool available = false;                               // 记录本次是否取到新遥测

    DINT;                                                 // 临界区内禁止 IPC ISR 修改遥测变量
    if(g_ipc_telemetry_pending)                           // 判断是否已有新遥测帧
    {
        *encoderCount = g_received_encoder_count;         // 复制编码器计数给调用者
        *currentSpeed = g_received_current_speed;         // 复制机械转速给调用者
        g_ipc_telemetry_pending = false;                  // 标记本帧遥测已经被主循环消费
        available = true;                                 // 告知调用者本次读取成功
    }
    EINT;                                                 // 恢复全局中断响应

    return available;                                     // 返回是否取得了新遥测
}

/* 5. 从 ISR 安全取出一条 CPU1 命令应答 */
bool IPC_CPU2_TakeAck(uint32_t *command, uint32_t *result)
{
    bool available = false;                               // 记录本次是否取到命令应答

    DINT;                                                 // 临界区内禁止 IPC ISR 修改应答 FIFO
    if(g_ackCount != 0U)                                  // 判断应答 FIFO 是否非空
    {
        *command = g_ackCommand[g_ackTail];               // 读取最早进入 FIFO 的命令号
        *result = g_ackResult[g_ackTail];                 // 读取该命令对应的执行结果
        g_ackTail = (uint16_t)((g_ackTail + 1U) % IPC_ACK_FIFO_SIZE); // 移动 FIFO 读指针
        g_ackCount--;                                     // 减少 FIFO 有效帧数
        available = true;                                 // 告知调用者本次读取成功
    }
    EINT;                                                 // 恢复全局中断响应

    return available;                                     // 返回是否取得了命令应答
}

/* 6. 接收 CPU1 遥测和命令应答 */
__interrupt void IPC_0_ISR(void)
{
    IPC_Message_t message;                                // 保存从 CPU1 队列读出的单帧消息

    while(IPC_readMessageFromQueue(IPC_CPU2_L_CPU1_R,    // 持续读取直到 CPU1 队列已清空
                                   &g_ipcQueue,           // 使用 CPU2↔CPU1 队列对象
                                   IPC_ADDR_CORRECTION_DISABLE, // 不对未使用的地址字段修正
                                   &message,              // 接收一帧消息到本地变量
                                   IPC_NONBLOCKING_CALL)) // 无可读消息时结束循环
    {
        g_ipc_last_command = message.command;             // 保存最近收到的 IPC 命令号
        g_ipc_rx_count++;                                 // 记录收到的 IPC 帧数

        if(message.command == IPC_CMD_MOTOR_TELEMETRY)    // 处理 CPU1 周期遥测帧
        {
            float decodedSpeed;                           // 保存由 dataw2 还原出的浮点转速

            memcpy(&decodedSpeed, &message.dataw2, sizeof(decodedSpeed)); // 保留位模式还原 IEEE-754 转速
            g_received_encoder_count = (int32_t)message.dataw1; // 保存编码器计数
            g_received_current_speed = decodedSpeed;      // 保存当前机械转速
            g_ipc_telemetry_pending = true;               // 通知主循环有新遥测可发送
        }
        else if(message.command == IPC_CMD_MOTOR_ACK)     // 处理 CPU1 返回的命令执行结果
        {
            if(g_ackCount < IPC_ACK_FIFO_SIZE)            // FIFO 未满时缓存应答
            {
                g_ackCommand[g_ackHead] = message.dataw1; // 保存被确认的原始命令号
                g_ackResult[g_ackHead] = message.dataw2;  // 保存 CPU1 返回的执行结果
                g_ackHead = (uint16_t)((g_ackHead + 1U) % IPC_ACK_FIFO_SIZE); // 移动 FIFO 写指针
                g_ackCount++;                             // 增加 FIFO 有效帧数
            }
        }
    }

    IPC_ackFlagRtoL(IPC_CPU2_L_CPU1_R, IPC_FLAG0);        // 应答 CPU1→CPU2 队列对应的 FLAG0
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);        // 应答 CPU1-CPU2 IPC 所在 PIE 第 1 组
}
