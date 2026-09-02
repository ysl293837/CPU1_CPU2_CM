#include "pc_serial.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "APP/key/key.h"
#include "speed_pi.h"

#ifndef PWM_FREQUENCY
#define PWM_FREQUENCY 10000.0f
#endif

static char g_pc_tx_buffer[PC_SERIAL_TX_BUF_SIZE];
static uint32_t g_serial_last_send_tick_ms = 0U;
static uint16_t g_serial_isr_div_count = 0U;

static volatile char g_rx_fifo[PC_SERIAL_RX_FIFO_SIZE];
static volatile uint16_t g_rx_head = 0U;
static volatile uint16_t g_rx_tail = 0U;
static volatile uint32_t g_rx_byte_count = 0U;
static volatile uint32_t g_rx_cmd_count = 0U;
static volatile uint32_t g_rx_error_count = 0U;
static volatile uint16_t g_last_rx_char = 0U;

static char g_cmd_buffer[PC_SERIAL_CMD_BUF_SIZE];
static uint16_t g_cmd_length = 0U;

uint8_t serial_stream_enabled = 1U;
uint32_t serial_stream_period_ms = PC_SERIAL_DEFAULT_PERIOD_MS;
uint32_t serial_stream_tick_ms = 0U;

extern volatile uint16_t g_motor_run_enabled;
extern volatile uint16_t g_motor_estop_active;
extern float uq;

static int32_t pc_float_to_milli(float value)
{
    if(value >= 0.0f)
    {
        return (int32_t)((value * 1000.0f) + 0.5f);
    }

    return (int32_t)((value * 1000.0f) - 0.5f);
}

static void pc_send_char(char ch)
{
    SCI_writeCharBlockingNonFIFO(mySCI1_BASE, (uint16_t)ch);
}

static void pc_send_scaled_field(char *buffer,
                                 size_t buffer_size,
                                 const char *prefix,
                                 float value)
{
    int32_t milli = pc_float_to_milli(value);
    uint32_t abs_value = (milli < 0) ? (uint32_t)(-milli) : (uint32_t)milli;

    snprintf(buffer,
             buffer_size,
             "%s%s%lu.%03lu",
             prefix,
             (milli < 0) ? "-" : "",
             (unsigned long)(abs_value / 1000U),
             (unsigned long)(abs_value % 1000U));
}

static void pc_fifo_push_char(char ch)
{
    uint16_t next = (uint16_t)((g_rx_head + 1U) % PC_SERIAL_RX_FIFO_SIZE);
    if(next == g_rx_tail)
    {
        return;
    }

    g_rx_fifo[g_rx_head] = ch;
    g_rx_head = next;
    g_rx_byte_count++;
    g_last_rx_char = (uint16_t)(uint8_t)ch;
}

static bool pc_fifo_pop_char(char *out_char)
{
    if(g_rx_tail == g_rx_head)
    {
        return false;
    }

    *out_char = g_rx_fifo[g_rx_tail];
    g_rx_tail = (uint16_t)((g_rx_tail + 1U) % PC_SERIAL_RX_FIFO_SIZE);
    return true;
}

static void pc_motor_force_stop(void)
{
    g_motor_run_enabled = 0U;
    Key_SetCommandSpeed(0.0f);
    PMSM_PID_Reset();
    uq = 0.0f;
}

static void pc_send_ok(const char *name)
{
    if(name == NULL)
    {
        PC_Serial_SendText("ack:1\r\n");
        return;
    }

    PC_Serial_Sendf("ack:1,cmd:%s\r\n", name);
}

static void pc_send_error(const char *message)
{
    if(message == NULL)
    {
        PC_Serial_SendText("err:1\r\n");
        return;
    }

    PC_Serial_Sendf("err:1,msg:%s\r\n", message);
}

static bool pc_parse_three_floats(const char *text, float *a, float *b, float *c)
{
    char local[PC_SERIAL_CMD_BUF_SIZE];
    char *token;

    if(text == NULL || a == NULL || b == NULL || c == NULL)
    {
        return false;
    }

    strncpy(local, text, sizeof(local) - 1U);
    local[sizeof(local) - 1U] = '\0';

    token = strtok(local, ",");
    if(token == NULL)
    {
        return false;
    }
    *a = strtof(token, NULL);

    token = strtok(NULL, ",");
    if(token == NULL)
    {
        return false;
    }
    *b = strtof(token, NULL);

    token = strtok(NULL, ",");
    if(token == NULL)
    {
        return false;
    }
    *c = strtof(token, NULL);

    return true;
}

static void pc_handle_command(const char *line)
{
    float kp;
    float ki;
    float kd;
    float speed;
    uint32_t period;

    if(line == NULL || line[0] == '\0')
    {
        return;
    }

    g_rx_cmd_count++;
    PC_Serial_Sendf("rxcmd:%s\r\n", line);

    if(strcmp(line, "status") == 0)
    {
        PC_Serial_SendStatus();
        return;
    }

    if(strcmp(line, "help") == 0)
    {
        PC_Serial_SendText(
            "help:start stop estop clear_estop status stream=0|1 period=ms target=x pid=kp,ki,kd kp=x ki=x kd=x reset_pid\r\n");
        return;
    }

    if(strcmp(line, "start") == 0 || strcmp(line, "run=1") == 0)
    {
        if(g_motor_estop_active != 0U)
        {
            pc_send_error("estop_active");
            return;
        }

        g_motor_run_enabled = 1U;
        PMSM_PID_Enable(true);
        pc_send_ok("start");
        PC_Serial_SendStatus();
        return;
    }

    if(strcmp(line, "stop") == 0 || strcmp(line, "run=0") == 0)
    {
        pc_motor_force_stop();
        pc_send_ok("stop");
        PC_Serial_SendStatus();
        return;
    }

    if(strcmp(line, "estop") == 0 || strcmp(line, "estop=1") == 0)
    {
        g_motor_estop_active = 1U;
        pc_motor_force_stop();
        pc_send_ok("estop");
        PC_Serial_SendStatus();
        return;
    }

    if(strcmp(line, "clear_estop") == 0 || strcmp(line, "estop=0") == 0)
    {
        g_motor_estop_active = 0U;
        PMSM_PID_Reset();
        uq = 0.0f;
        pc_send_ok("clear_estop");
        PC_Serial_SendStatus();
        return;
    }

    if(strcmp(line, "reset_pid") == 0)
    {
        PMSM_PID_Reset();
        pc_send_ok("reset_pid");
        PC_Serial_SendStatus();
        return;
    }

    if(strncmp(line, "stream=", 7U) == 0)
    {
        serial_stream_enabled = (uint8_t)(strtoul(line + 7U, NULL, 10) ? 1U : 0U);
        pc_send_ok("stream");
        PC_Serial_SendStatus();
        return;
    }

    if(strncmp(line, "period=", 7U) == 0)
    {
        period = strtoul(line + 7U, NULL, 10);
        if(period == 0U)
        {
            period = 1U;
        }
        if(period > 1000U)
        {
            period = 1000U;
        }

        serial_stream_period_ms = period;
        pc_send_ok("period");
        PC_Serial_SendStatus();
        return;
    }

    if(strncmp(line, "target=", 7U) == 0)
    {
        if(g_motor_estop_active != 0U)
        {
            pc_send_error("estop_active");
            return;
        }

        speed = strtof(line + 7U, NULL);
        Key_SetCommandSpeed(speed);
        pc_send_ok("target");
        PC_Serial_SendStatus();
        return;
    }

    if(strncmp(line, "pid=", 4U) == 0)
    {
        if(!pc_parse_three_floats(line + 4U, &kp, &ki, &kd))
        {
            pc_send_error("bad_pid");
            return;
        }

        PMSM_SetPIDParams(kp, ki, kd);
        pc_send_ok("pid");
        PC_Serial_SendStatus();
        return;
    }

    if(strncmp(line, "kp=", 3U) == 0)
    {
        kp = strtof(line + 3U, NULL);
        PMSM_SetPIDParams(kp, speedPID.Integral, speedPID.Derivative);
        pc_send_ok("kp");
        PC_Serial_SendStatus();
        return;
    }

    if(strncmp(line, "ki=", 3U) == 0)
    {
        ki = strtof(line + 3U, NULL);
        PMSM_SetPIDParams(speedPID.Proportion, ki, speedPID.Derivative);
        pc_send_ok("ki");
        PC_Serial_SendStatus();
        return;
    }

    if(strncmp(line, "kd=", 3U) == 0)
    {
        kd = strtof(line + 3U, NULL);
        PMSM_SetPIDParams(speedPID.Proportion, speedPID.Integral, kd);
        pc_send_ok("kd");
        PC_Serial_SendStatus();
        return;
    }

    pc_send_error("unknown_cmd");
}

void PC_Serial_Init(void)
{
    serial_stream_enabled = 1U;
    serial_stream_period_ms = PC_SERIAL_DEFAULT_PERIOD_MS;
    serial_stream_tick_ms = 0U;
    g_serial_last_send_tick_ms = 0U;
    g_serial_isr_div_count = 0U;
    g_rx_head = 0U;
    g_rx_tail = 0U;
    g_rx_byte_count = 0U;
    g_rx_cmd_count = 0U;
    g_rx_error_count = 0U;
    g_last_rx_char = 0U;
    g_cmd_length = 0U;

    PC_Serial_SendText("boot:1,proto:1,stream:1,period_ms:10\r\n");
    PC_Serial_SendStatus();
}

uint8_t PC_Serial_SendText(const char *text)
{
    if(text == NULL)
    {
        return 0U;
    }

    while(*text != '\0')
    {
        pc_send_char(*text);
        text++;
    }

    return 1U;
}

uint8_t PC_Serial_Sendf(const char *fmt, ...)
{
    int len;
    va_list args;

    if(fmt == NULL)
    {
        return 0U;
    }

    va_start(args, fmt);
    len = vsnprintf(g_pc_tx_buffer, sizeof(g_pc_tx_buffer), fmt, args);
    va_end(args);

    if(len <= 0)
    {
        return 0U;
    }

    if((uint16_t)len >= PC_SERIAL_TX_BUF_SIZE)
    {
        g_pc_tx_buffer[PC_SERIAL_TX_BUF_SIZE - 1U] = '\0';
    }

    return PC_Serial_SendText(g_pc_tx_buffer);
}

void PC_Serial_TickFromISR(void)
{
    g_serial_isr_div_count++;

    if(g_serial_isr_div_count >= (uint16_t)(PWM_FREQUENCY / 1000.0f))
    {
        g_serial_isr_div_count = 0U;
        serial_stream_tick_ms++;
    }
}

void PC_Serial_SendStatus(void)
{
    char current_speed_text[24];
    char target_speed_text[24];
    char pid_output_text[24];
    char i_term_text[24];
    char kp_text[24];
    char ki_text[24];
    char kd_text[24];

    pc_send_scaled_field(current_speed_text, sizeof(current_speed_text), "", current_speed);
    pc_send_scaled_field(target_speed_text, sizeof(target_speed_text), "", target_speed);
    pc_send_scaled_field(pid_output_text, sizeof(pid_output_text), "", speedPID.Output);
    pc_send_scaled_field(i_term_text, sizeof(i_term_text), "", PMSM_PID_GetIntegralTerm());
    pc_send_scaled_field(kp_text, sizeof(kp_text), "", speedPID.Proportion);
    pc_send_scaled_field(ki_text, sizeof(ki_text), "", speedPID.Integral);
    pc_send_scaled_field(kd_text, sizeof(kd_text), "", speedPID.Derivative);

    snprintf(g_pc_tx_buffer,
         sizeof(g_pc_tx_buffer),
         "enc_count:%ld,current_speed:%s\r\n",
         (long)current_count,
         current_speed_text);

    // snprintf(g_pc_tx_buffer,
    //          sizeof(g_pc_tx_buffer),
    //          "status:1,run:%u,estop:%u,stream:%u,period_ms:%lu,enc_count:%ld,current_speed:%s,target_speed:%s,pid_output:%s,i_term:%s,kp:%s,ki:%s,kd:%s\r\n",
    //          (unsigned int)g_motor_run_enabled,
    //          (unsigned int)g_motor_estop_active,
    //          (unsigned int)serial_stream_enabled,
    //          (unsigned long)serial_stream_period_ms,
    //          (long)current_count,
    //          current_speed_text,
    //          target_speed_text,
    //          pid_output_text,
    //          i_term_text,
    //          kp_text,
    //          ki_text,
    //          kd_text);

    PC_Serial_SendText(g_pc_tx_buffer);

    PC_Serial_Sendf("diag:1,rx_bytes:%lu,rx_cmds:%lu,rx_errs:%lu,last_rx:%u\r\n",
                    (unsigned long)g_rx_byte_count,
                    (unsigned long)g_rx_cmd_count,
                    (unsigned long)g_rx_error_count,
                    (unsigned int)g_last_rx_char);
}

void PC_Serial_PollSend(void)
{
    uint32_t now;

    if(serial_stream_enabled == 0U)
    {
        return;
    }

    now = serial_stream_tick_ms;
    if((now - g_serial_last_send_tick_ms) < serial_stream_period_ms)
    {
        return;
    }

    g_serial_last_send_tick_ms = now;
    PC_Serial_SendStatus();
}

void PC_Serial_ProcessRx(void)
{
    char ch;

    /*
     * 兼容两种接收路径：
     * 1. 正常情况下由SCIA RX中断把字节推入软件FIFO；
     * 2. 如果中断配置异常或被意外关闭，主循环这里再主动轮询一次SCI，
     *    避免出现“上位机能连上、下位机会发状态、但任何命令都收不到”的情况。
     */
    PC_Serial_RXISR();

    while(pc_fifo_pop_char(&ch))
    {
        if(ch == '\r' || ch == '\n')
        {
            if(g_cmd_length > 0U)
            {
                g_cmd_buffer[g_cmd_length] = '\0';
                pc_handle_command(g_cmd_buffer);
                g_cmd_length = 0U;
            }
            continue;
        }

        if(g_cmd_length < (PC_SERIAL_CMD_BUF_SIZE - 1U))
        {
            g_cmd_buffer[g_cmd_length++] = ch;
        }
        else
        {
            g_cmd_length = 0U;
            pc_send_error("cmd_too_long");
        }
    }
}

void PC_Serial_RXISR(void)
{
    uint16_t rx_status = SCI_getRxStatus(mySCI1_BASE);

    if((rx_status & SCI_RXSTATUS_ERROR) != 0U)
    {
        g_rx_error_count++;
        SCI_clearOverflowStatus(mySCI1_BASE);
        SCI_clearInterruptStatus(mySCI1_BASE,
                                 SCI_INT_RXFF |
                                 SCI_INT_RXERR |
                                 SCI_INT_FE |
                                 SCI_INT_OE |
                                 SCI_INT_PE |
                                 SCI_INT_RXRDY_BRKDT);
        SCI_performSoftwareReset(mySCI1_BASE);
    }

    while(SCI_isDataAvailableNonFIFO(mySCI1_BASE))
    {
        char ch = (char)SCI_readCharNonBlocking(mySCI1_BASE);
        pc_fifo_push_char(ch);
    }
}
