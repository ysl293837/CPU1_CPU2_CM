#ifndef APP_PC_SERIAL_PC_SERIAL_H
#define APP_PC_SERIAL_PC_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "device.h"
#include "driverlib.h"

#ifndef PC_SERIAL_TX_BUF_SIZE
#define PC_SERIAL_TX_BUF_SIZE 256U
#endif

#ifndef PC_SERIAL_RX_FIFO_SIZE
#define PC_SERIAL_RX_FIFO_SIZE 256U
#endif

#ifndef PC_SERIAL_CMD_BUF_SIZE
#define PC_SERIAL_CMD_BUF_SIZE 96U
#endif

#ifndef PC_SERIAL_DEFAULT_PERIOD_MS
#define PC_SERIAL_DEFAULT_PERIOD_MS 10U
#endif

extern int32_t current_count;
extern float current_speed;
extern float target_speed;

extern uint8_t serial_stream_enabled;
extern uint32_t serial_stream_period_ms;
extern uint32_t serial_stream_tick_ms;

void PC_Serial_Init(void);
uint8_t PC_Serial_SendText(const char *text);
uint8_t PC_Serial_Sendf(const char *fmt, ...);
void PC_Serial_TickFromISR(void);
void PC_Serial_PollSend(void);
void PC_Serial_ProcessRx(void);
void PC_Serial_RXISR(void);
void PC_Serial_SendStatus(void);

#ifdef __cplusplus
}
#endif

#endif
