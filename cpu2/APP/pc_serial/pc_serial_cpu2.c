#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driverlib.h"
#include "board.h"
#include "../../../ipc_protocol.h"
#include "../ipc/ipc_cpu2.h"
#include "pc_serial_cpu2.h"

#define PC_RX_FIFO_SIZE 128U                              // 串口接收软件 FIFO 的字节容量
#define PC_COMMAND_SIZE 96U                               // 单条上位机命令的最大字符数

static volatile char g_rx_fifo[PC_RX_FIFO_SIZE];          // 存储 SCIA 接收字符的软件 FIFO
static volatile uint16_t g_rx_head = 0U;                  // 接收 FIFO 的写入位置
static volatile uint16_t g_rx_tail = 0U;                  // 接收 FIFO 的读取位置
static char g_command[PC_COMMAND_SIZE];                   // 保存正在拼接的 ASCII 命令
static uint16_t g_command_length = 0U;                    // 当前命令已接收的字符数
static uint16_t g_stream_enabled = 1U;                    // 是否周期发送遥测数据
static uint16_t g_stream_period_ms = 10U;                 // 遥测输出周期，单位 ms
static uint16_t g_stream_elapsed_ms = 0U;                 // 遥测节拍累计时间，单位 ms

static uint32_t g_rx_byte_count = 0UL;                    // 串口累计接收字节数
static uint32_t g_rx_cmd_count = 0UL;                     // 串口累计解析命令数
static uint32_t g_rx_error_count = 0UL;                   // 串口接收、缓存或格式错误数
static uint16_t g_last_rx_char = 0U;                      // 最近接收的 ASCII 字符

/* 1. 发送一个串口字符 */
static void PC_Serial_CPU2_SendChar(char ch)
{
    SCI_writeCharBlockingNonFIFO(mySCI1_BASE, (uint16_t)ch); // 等待发送寄存器可用后写入字符
}

/* 2. 格式化并发送一行串口文本 */
static void PC_Serial_CPU2_Sendf(const char *format, ...)
{
    char text[160];                                       // 格式化后的临时文本缓冲区
    va_list args;                                         // 可变参数遍历对象
    int length;                                           // 格式化后的有效字符数
    int index;                                            // 逐字节发送时的索引

    va_start(args, format);                               // 初始化可变参数读取
    length = vsnprintf(text, sizeof(text), format, args); // 将格式化内容写入临时缓冲区
    va_end(args);                                         // 结束可变参数读取

    if(length <= 0)                                       // 没有生成有效文本时直接退出
    {
        return;                                           // 避免发送空文本
    }

    if(length >= (int)sizeof(text))                       // 格式化文本超出缓冲区时截断
    {
        length = (int)sizeof(text) - 1;                   // 保留一个字符串结束符位置
    }

    for(index = 0; index < length; index++)               // 依次发送格式化后的每个字符
    {
        PC_Serial_CPU2_SendChar(text[index]);             // 通过 SCIA 输出当前字符
    }
}

/* 3. 发送一个以零结尾的字符串 */
static void PC_Serial_CPU2_SendText(const char *text)
{
    while(*text != '\0')                                  // 遍历直到字符串结束符
    {
        PC_Serial_CPU2_SendChar(*text++);                 // 发送当前字符并移动字符串指针
    }
}

/* 4. 将浮点数转换为千分之一单位的整数 */
static int32_t PC_Serial_CPU2_FloatToMilli(float value)
{
    if(value >= 0.0f)                                     // 正数需要向上补偿半个最小单位
    {
        return (int32_t)(value * 1000.0f + 0.5f);         // 转为保留三位小数的整数
    }

    return (int32_t)(value * 1000.0f - 0.5f);             // 负数向下补偿半个最小单位
}

/* 5. 以固定三位小数发送浮点数 */
static void PC_Serial_CPU2_SendScaled(const char *name, float value)
{
    int32_t milli = PC_Serial_CPU2_FloatToMilli(value);   // 将浮点数转换为千分之一单位
    uint32_t magnitude;                                   // 保存绝对值部分
    const char *sign = "";                               // 默认不发送符号

    if(milli < 0L)                                        // 判断数值是否为负数
    {
        sign = "-";                                      // 为负数添加负号
        magnitude = (uint32_t)(-milli);                   // 保存负数的绝对值
    }
    else
    {
        magnitude = (uint32_t)milli;                      // 正数直接转换为无符号绝对值
    }

    PC_Serial_CPU2_Sendf("%s%s%lu.%03lu",                // 输出名称、符号和三位小数数值
                         name,
                         sign,
                         (unsigned long)(magnitude / 1000UL),
                         (unsigned long)(magnitude % 1000UL));
}

/* 6. 将一个接收字符写入软件 FIFO */
static bool PC_Serial_CPU2_FifoPush(char ch)
{
    uint16_t next = (uint16_t)((g_rx_head + 1U) % PC_RX_FIFO_SIZE); // 计算下一写入位置

    if(next == g_rx_tail)                                 // 写入位置追上读取位置表示 FIFO 已满
    {
        g_rx_error_count++;                               // 记录软件 FIFO 溢出错误
        return false;                                     // 丢弃当前字符并返回失败
    }

    g_rx_fifo[g_rx_head] = ch;                            // 将字符写入当前 FIFO 写位置
    g_rx_head = next;                                     // 推进 FIFO 写指针
    return true;                                          // 返回写入成功
}

/* 7. 从软件 FIFO 读取一个接收字符 */
static bool PC_Serial_CPU2_FifoPop(char *ch)
{
    bool available = false;                               // 记录 FIFO 是否存在可读字符

    DINT;                                                 // 临界区内禁止接收 ISR 修改 FIFO 写指针
    if(g_rx_tail != g_rx_head)                            // 读写指针不同表示 FIFO 非空
    {
        *ch = g_rx_fifo[g_rx_tail];                       // 读取最早写入的字符
        g_rx_tail = (uint16_t)((g_rx_tail + 1U) % PC_RX_FIFO_SIZE); // 推进 FIFO 读指针
        available = true;                                 // 标记本次读取成功
    }
    EINT;                                                 // 恢复全局中断响应

    return available;                                     // 返回是否取到了字符
}

/* 8. 输出 CPU1 返回的命令应答 */
static void PC_Serial_CPU2_SendAck(uint32_t command, uint32_t result)
{
    PC_Serial_CPU2_Sendf("ack:1,cmd:%lu,result:%lu\r\n", // 发送命令号和 CPU1 执行结果
                         (unsigned long)command,
                         (unsigned long)result);
}

/* 9. 输出一条串口错误信息 */
static void PC_Serial_CPU2_SendError(const char *message)
{
    PC_Serial_CPU2_Sendf("err:1,msg:%s\r\n", message);  // 按统一格式发送错误原因
}

/* 10. 将浮点参数编码后发送给 CPU1 */
static bool PC_Serial_CPU2_SendFloatCommand(uint32_t command, float value)
{
    uint32_t bits;                                        // 保存浮点数的 IEEE-754 位模式

    memcpy(&bits, &value, sizeof(bits));                  // 不做数值转换，直接复制浮点位模式
    return IPC_CPU2_SendCommand(command, bits, 0UL);      // 将位模式放入 dataw1 发送给 CPU1
}

/* 11. 解析一条完整的 ASCII 上位机命令 */
static void PC_Serial_CPU2_HandleCommand(char *command)
{
    float value;                                          // 保存单个浮点命令参数
    float kp;                                             // 保存批量 PID 命令中的 Kp
    float ki;                                             // 保存批量 PID 命令中的 Ki
    float kd;                                             // 保存批量 PID 命令中的 Kd
    char *end;                                            // 保存数值字符串解析后的结束位置

    g_rx_cmd_count++;                                     // 记录一条待解析的命令

    if(strcmp(command, "status") == 0)                   // 处理状态查询命令
    {
        PC_Serial_CPU2_SendStatus();                      // 立即发送当前遥测和诊断信息
    }
    else if(strcmp(command, "help") == 0)                // 处理帮助查询命令
    {
        PC_Serial_CPU2_SendText(                          // 输出支持的全部命令格式
            "cmd:status,help,start,stop,estop,clear_estop,reset_pid,"
            "target=x,pid=kp,ki,kd,kp=x,ki=x,kd=x,stream=0|1,period=ms\r\n");
    }
    else if((strcmp(command, "start") == 0) ||           // 处理 start 或 run=1 命令
            (strcmp(command, "run=1") == 0))
    {
        if(!IPC_CPU2_SendCommand(IPC_CMD_MOTOR_START, 0UL, 0UL)) // 将启动命令发给 CPU1
        {
            PC_Serial_CPU2_SendError("ipc_busy");        // IPC 队列忙时提示上位机重试
        }
    }
    else if((strcmp(command, "stop") == 0) ||            // 处理 stop 或 run=0 命令
            (strcmp(command, "run=0") == 0))
    {
        if(!IPC_CPU2_SendCommand(IPC_CMD_MOTOR_STOP, 0UL, 0UL)) // 将停止命令发给 CPU1
        {
            PC_Serial_CPU2_SendError("ipc_busy");        // IPC 队列忙时提示上位机重试
        }
    }
    else if((strcmp(command, "estop") == 0) ||           // 处理 estop 或 estop=1 命令
            (strcmp(command, "estop=1") == 0))
    {
        if(!IPC_CPU2_SendCommand(IPC_CMD_MOTOR_ESTOP, 0UL, 0UL)) // 将急停命令发给 CPU1
        {
            PC_Serial_CPU2_SendError("ipc_busy");        // IPC 队列忙时提示上位机重试
        }
    }
    else if((strcmp(command, "clear_estop") == 0) ||     // 处理清除急停命令
            (strcmp(command, "estop=0") == 0))
    {
        if(!IPC_CPU2_SendCommand(IPC_CMD_MOTOR_CLEAR_ESTOP, 0UL, 0UL)) // 将清除急停命令发给 CPU1
        {
            PC_Serial_CPU2_SendError("ipc_busy");        // IPC 队列忙时提示上位机重试
        }
    }
    else if(strcmp(command, "reset_pid") == 0)           // 处理速度 PI 复位命令
    {
        if(!IPC_CPU2_SendCommand(IPC_CMD_MOTOR_RESET_PID, 0UL, 0UL)) // 将 PI 复位命令发给 CPU1
        {
            PC_Serial_CPU2_SendError("ipc_busy");        // IPC 队列忙时提示上位机重试
        }
    }
    else if(strncmp(command, "target=", 7U) == 0)        // 处理目标速度命令
    {
        value = strtof(&command[7], &end);                // 将 target= 后的文本转换为浮点数
        if((*end != '\0') || !PC_Serial_CPU2_SendFloatCommand( // 检查文本格式并发送目标速度
                                  IPC_CMD_MOTOR_SET_TARGET, value))
        {
            PC_Serial_CPU2_SendError((*end != '\0') ? "bad_target" : "ipc_busy"); // 返回格式或队列错误
        }
    }
    else if(strncmp(command, "pid=", 4U) == 0)           // 处理一次设置 Kp、Ki、Kd 的命令
    {
        if(sscanf(&command[4], "%f,%f,%f", &kp, &ki, &kd) != 3) // 校验三个 PID 参数的格式
        {
            PC_Serial_CPU2_SendError("bad_pid");         // 参数数量或格式错误时提示上位机
        }
        else if(!PC_Serial_CPU2_SendFloatCommand(IPC_CMD_MOTOR_SET_KP, kp) || // 依次发送 Kp
                !PC_Serial_CPU2_SendFloatCommand(IPC_CMD_MOTOR_SET_KI, ki) || // 依次发送 Ki
                !PC_Serial_CPU2_SendFloatCommand(IPC_CMD_MOTOR_SET_KD, kd))   // 依次发送 Kd
        {
            PC_Serial_CPU2_SendError("ipc_busy");        // 任一 IPC 队列写入失败时提示上位机
        }
    }
    else if(strncmp(command, "kp=", 3U) == 0)            // 处理单独设置 Kp 命令
    {
        value = strtof(&command[3], &end);                // 解析 Kp 浮点参数
        if((*end != '\0') || !PC_Serial_CPU2_SendFloatCommand( // 检查文本格式并发送 Kp
                                  IPC_CMD_MOTOR_SET_KP, value))
        {
            PC_Serial_CPU2_SendError((*end != '\0') ? "bad_kp" : "ipc_busy"); // 返回格式或队列错误
        }
    }
    else if(strncmp(command, "ki=", 3U) == 0)            // 处理单独设置 Ki 命令
    {
        value = strtof(&command[3], &end);                // 解析 Ki 浮点参数
        if((*end != '\0') || !PC_Serial_CPU2_SendFloatCommand( // 检查文本格式并发送 Ki
                                  IPC_CMD_MOTOR_SET_KI, value))
        {
            PC_Serial_CPU2_SendError((*end != '\0') ? "bad_ki" : "ipc_busy"); // 返回格式或队列错误
        }
    }
    else if(strncmp(command, "kd=", 3U) == 0)            // 处理单独设置 Kd 命令
    {
        value = strtof(&command[3], &end);                // 解析 Kd 浮点参数
        if((*end != '\0') || !PC_Serial_CPU2_SendFloatCommand( // 检查文本格式并发送 Kd
                                  IPC_CMD_MOTOR_SET_KD, value))
        {
            PC_Serial_CPU2_SendError((*end != '\0') ? "bad_kd" : "ipc_busy"); // 返回格式或队列错误
        }
    }
    else if(strncmp(command, "stream=", 7U) == 0)        // 处理遥测开关命令
    {
        g_stream_enabled = (uint16_t)(strtoul(&command[7], &end, 10) != 0UL); // 将非零数值转换为使能状态
        if(*end != '\0')                                  // 检查 stream 参数末尾是否存在非法字符
        {
            PC_Serial_CPU2_SendError("bad_stream");      // 返回遥测开关参数格式错误
        }
        else
        {
            PC_Serial_CPU2_Sendf("ok:stream=%u\r\n", g_stream_enabled); // 返回已生效的遥测开关状态
        }
    }
    else if(strncmp(command, "period=", 7U) == 0)        // 处理遥测输出周期命令
    {
        value = strtof(&command[7], &end);                // 将周期文本转换为浮点数
        if((*end != '\0') || (value < 1.0f) || (value > 1000.0f)) // 限制周期在 1 至 1000 ms
        {
            PC_Serial_CPU2_SendError("bad_period");      // 返回周期参数格式或范围错误
        }
        else
        {
            g_stream_period_ms = (uint16_t)value;         // 保存新的遥测周期
            g_stream_elapsed_ms = 0U;                     // 从新的周期起点重新计时
            PC_Serial_CPU2_Sendf("ok:period_ms=%u\r\n", g_stream_period_ms); // 返回已生效的周期
        }
    }
    else
    {
        PC_Serial_CPU2_SendError("unknown_command");     // 返回未识别的命令错误
    }
}

/* 12. 初始化串口命令处理状态 */
void PC_Serial_CPU2_Init(void)
{
    g_rx_head = 0U;                                       // 清空接收 FIFO 写指针
    g_rx_tail = 0U;                                       // 清空接收 FIFO 读指针
    g_command_length = 0U;                                // 清空正在拼接的命令
    g_stream_elapsed_ms = 0U;                             // 从遥测周期起点开始计时
    PC_Serial_CPU2_SendText("boot:cpu2,serial:scia,ipc:command\r\n"); // 发送 CPU2 串口启动标识
    PC_Serial_CPU2_SendStatus();                          // 上电后立即发送一帧初始状态
}

/* 13. 从接收 FIFO 拼接并解析完整命令 */
void PC_Serial_CPU2_ProcessRx(void)
{
    char ch;                                              // 保存从接收 FIFO 取出的单个字符

    PC_Serial_CPU2_RXISR();                               // 轮询读取硬件 FIFO，兼容中断暂未触发的情况

    while(PC_Serial_CPU2_FifoPop(&ch))                    // 逐个处理已经接收的字符
    {
        if((ch == '\r') || (ch == '\n'))                 // 回车或换行表示一条命令结束
        {
            if(g_command_length != 0U)                    // 忽略空行
            {
                g_command[g_command_length] = '\0';       // 为命令字符串添加结束符
                PC_Serial_CPU2_HandleCommand(g_command);  // 解析并执行完整 ASCII 命令
                g_command_length = 0U;                    // 清空缓冲区准备接收下一条命令
            }
        }
        else if(g_command_length < (PC_COMMAND_SIZE - 1U)) // 缓冲区未满时继续保存字符
        {
            g_command[g_command_length++] = ch;           // 写入字符并增加命令长度
        }
        else
        {
            g_command_length = 0U;                        // 命令过长时丢弃当前命令
            g_rx_error_count++;                           // 记录命令长度错误
            PC_Serial_CPU2_SendError("command_too_long"); // 提示上位机缩短命令
        }
    }
}

/* 14. 将 CPU1 命令应答输出到串口 */
void PC_Serial_CPU2_ProcessAcks(void)
{
    uint32_t command;                                     // 保存应答对应的原始命令号
    uint32_t result;                                      // 保存 CPU1 返回的执行结果

    while(IPC_CPU2_TakeAck(&command, &result))            // 取出所有已缓存的 CPU1 应答
    {
        PC_Serial_CPU2_SendAck(command, result);          // 以 ASCII 文本发送应答给上位机
    }
}

/* 15. 判断是否达到下一次遥测发送周期 */
bool PC_Serial_CPU2_StreamDue(void)
{
    if(g_stream_enabled == 0U)                            // 串口遥测被关闭时不发送
    {
        return false;                                     // 直接返回未到发送时刻
    }

    if(g_stream_elapsed_ms < g_stream_period_ms)          // 本周期时间尚未累计到设定值
    {
        g_stream_elapsed_ms = (uint16_t)(g_stream_elapsed_ms + 10U); // 按 CPU1 遥测节拍累计时间
        return false;                                     // 继续等待下一帧 CPU1 遥测
    }

    g_stream_elapsed_ms = 0U;                             // 到达周期后从零重新计时
    return true;                                          // 通知调用者发送一帧遥测
}

/* 16. 输出编码器和速度遥测 */
void PC_Serial_CPU2_SendTelemetry(int32_t encoderCount, float currentSpeed)
{
    PC_Serial_CPU2_Sendf("enc_count:%ld,", (long)encoderCount); // 输出编码器计数
    PC_Serial_CPU2_SendScaled("current_speed:", currentSpeed);  // 输出当前机械转速
    PC_Serial_CPU2_SendText("\r\n");                    // 结束遥测文本行
}

/* 17. 输出当前串口和 IPC 诊断状态 */
void PC_Serial_CPU2_SendStatus(void)
{
    PC_Serial_CPU2_SendTelemetry(g_received_encoder_count, // 输出最近收到的 CPU1 遥测
                                 g_received_current_speed);
    PC_Serial_CPU2_Sendf(                                 // 输出接收、命令和遥测流状态
        "diag:1,rx_bytes:%lu,rx_cmds:%lu,rx_errs:%lu,last_rx:%u,"
        "ipc_rx:%lu,stream:%u,period_ms:%u\r\n",
        (unsigned long)g_rx_byte_count,
        (unsigned long)g_rx_cmd_count,
        (unsigned long)g_rx_error_count,
        (unsigned int)g_last_rx_char,
        (unsigned long)g_ipc_rx_count,
        (unsigned int)g_stream_enabled,
        (unsigned int)g_stream_period_ms);
}

/* 18. 读取 SCIA 硬件 FIFO 并写入软件 FIFO */
__interrupt void PC_Serial_CPU2_RXISR(void)
{
    uint16_t status = SCI_getRxStatus(mySCI1_BASE);       // 读取当前 SCIA 接收状态和错误位

    if((status & SCI_RXSTATUS_ERROR) != 0U)               // 检查帧、溢出或奇偶校验错误
    {
        g_rx_error_count++;                               // 记录一次硬件接收错误
        SCI_clearOverflowStatus(mySCI1_BASE);             // 清除 SCIA 接收 FIFO 溢出状态
        SCI_resetChannels(mySCI1_BASE);                   // 复位接收通道以恢复后续通信
    }

    while(SCI_getRxFIFOStatus(mySCI1_BASE) != SCI_FIFO_RX0) // 读取硬件 FIFO 中的全部字符
    {
        char ch = (char)SCI_readCharNonBlocking(mySCI1_BASE); // 非阻塞读取一个串口字符
        g_rx_byte_count++;                                // 累计收到的串口字节数
        g_last_rx_char = (uint16_t)(uint8_t)ch;           // 保存最近收到字符的 ASCII 值
        (void)PC_Serial_CPU2_FifoPush(ch);                // 将字符压入软件 FIFO，满时由函数记录错误
    }

    SCI_clearInterruptStatus(mySCI1_BASE,                 // 清除本次已处理的 SCIA 接收中断源
                             SCI_INT_RXFF | SCI_INT_RXERR |
                             SCI_INT_FE | SCI_INT_OE | SCI_INT_PE);
}

/* 19. 应答 SysConfig 注册的 SCIA 接收中断 */
__interrupt void INT_mySCI1_RX_ISR(void)
{
    PC_Serial_CPU2_RXISR();                               // 读取并缓存本次 SCIA 接收数据
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);        // 应答 SCIA 所在 PIE 第 9 组
}
