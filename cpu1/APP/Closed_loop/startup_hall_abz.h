#ifndef __STARTUP_Hall_ABZ_H
#define __STARTUP_Hall_ABZ_H

#include <stdint.h>

void Startup_HallABZ_Init(void);
void Startup_HallABZ_Update(void);
float Startup_HallABZ_GetElectricalAngle(void);
float Startup_HallABZ_LimitUq(float uq_in);
void Startup_HallABZ_OnZPulse(void);

float Startup_GetUqCmd(float uq_pid);

#endif






