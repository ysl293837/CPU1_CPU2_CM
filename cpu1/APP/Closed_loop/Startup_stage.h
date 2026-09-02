#ifndef APP_CLOSED_LOOP_STARTUP_STAGE_H_
#define APP_CLOSED_LOOP_STARTUP_STAGE_H_

#include <stdint.h>

void Rotor_Positioning2(float UD, float UQ, uint32_t positioning_time_us); // 用固定 dq 电压完成转子预定位

#endif /* APP_CLOSED_LOOP_STARTUP_STAGE_H_ */
