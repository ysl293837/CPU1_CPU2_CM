/*
 * Copyright (c) 2020 Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef BOARD_H
#define BOARD_H

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//
// Included Files
//

#include "driverlib.h"
#include "device.h"

//*****************************************************************************
//
// PinMux Configurations
//
//*****************************************************************************

//
// EPWM1 -> myEPWM1 Pinmux
//
//
// EPWM1A - GPIO Settings
//
#define GPIO_PIN_EPWM1A 0
#define myEPWM1_EPWMA_GPIO 0
#define myEPWM1_EPWMA_PIN_CONFIG GPIO_0_EPWM1A
//
// EPWM1B - GPIO Settings
//
#define GPIO_PIN_EPWM1B 1
#define myEPWM1_EPWMB_GPIO 1
#define myEPWM1_EPWMB_PIN_CONFIG GPIO_1_EPWM1B

//
// EPWM2 -> myEPWM2 Pinmux
//
//
// EPWM2A - GPIO Settings
//
#define GPIO_PIN_EPWM2A 2
#define myEPWM2_EPWMA_GPIO 2
#define myEPWM2_EPWMA_PIN_CONFIG GPIO_2_EPWM2A
//
// EPWM2B - GPIO Settings
//
#define GPIO_PIN_EPWM2B 3
#define myEPWM2_EPWMB_GPIO 3
#define myEPWM2_EPWMB_PIN_CONFIG GPIO_3_EPWM2B

//
// EPWM3 -> myEPWM3 Pinmux
//
//
// EPWM3A - GPIO Settings
//
#define GPIO_PIN_EPWM3A 4
#define myEPWM3_EPWMA_GPIO 4
#define myEPWM3_EPWMA_PIN_CONFIG GPIO_4_EPWM3A
//
// EPWM3B - GPIO Settings
//
#define GPIO_PIN_EPWM3B 5
#define myEPWM3_EPWMB_GPIO 5
#define myEPWM3_EPWMB_PIN_CONFIG GPIO_5_EPWM3B

//
// EQEP1 -> myEQEP0 Pinmux
//
//
// EQEP1_A - GPIO Settings
//
#define GPIO_PIN_EQEP1_A 20
#define myEQEP0_EQEPA_GPIO 20
#define myEQEP0_EQEPA_PIN_CONFIG GPIO_20_EQEP1_A
//
// EQEP1_B - GPIO Settings
//
#define GPIO_PIN_EQEP1_B 21
#define myEQEP0_EQEPB_GPIO 21
#define myEQEP0_EQEPB_PIN_CONFIG GPIO_21_EQEP1_B
//
// EQEP1_STROBE - GPIO Settings
//
#define GPIO_PIN_EQEP1_STROBE 22
#define myEQEP0_EQEPSTROBE_GPIO 22
#define myEQEP0_EQEPSTROBE_PIN_CONFIG GPIO_22_EQEP1_STROBE
//
// EQEP1_INDEX - GPIO Settings
//
#define GPIO_PIN_EQEP1_INDEX 23
#define myEQEP0_EQEPINDEX_GPIO 23
#define myEQEP0_EQEPINDEX_PIN_CONFIG GPIO_23_EQEP1_INDEX
//
// GPIO16 - GPIO Settings
//
#define XINT1_GPIO_PIN_CONFIG GPIO_16_GPIO16
//
// GPIO17 - GPIO Settings
//
#define XINT2_GPIO_PIN_CONFIG GPIO_17_GPIO17
//
// GPIO18 - GPIO Settings
//
#define XINT3_GPIO_PIN_CONFIG GPIO_18_GPIO18
//
// GPIO19 - GPIO Settings
//
#define XINT4_GPIO_PIN_CONFIG GPIO_19_GPIO19

//*****************************************************************************
//
// ADC Configurations
//
//*****************************************************************************
#define myADC1_BASE ADCA_BASE
#define myADC1_RESULT_BASE ADCARESULT_BASE
void myADC1_init();


//*****************************************************************************
//
// EPWM Configurations
//
//*****************************************************************************
#define myEPWM1_BASE EPWM1_BASE
#define myEPWM1_TBPRD 10000
#define myEPWM1_COUNTER_MODE EPWM_COUNTER_MODE_UP_DOWN
#define myEPWM1_TBPHS 0
#define myEPWM1_CMPA 5000
#define myEPWM1_CMPB 1
#define myEPWM1_CMPC 0
#define myEPWM1_CMPD 0
#define myEPWM1_DBRED 50
#define myEPWM1_DBFED 50
#define myEPWM1_TZA_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM1_TZB_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM1_INTERRUPT_SOURCE EPWM_INT_TBCTR_ZERO
#define myEPWM2_BASE EPWM2_BASE
#define myEPWM2_TBPRD 10000
#define myEPWM2_COUNTER_MODE EPWM_COUNTER_MODE_UP_DOWN
#define myEPWM2_TBPHS 0
#define myEPWM2_CMPA 5000
#define myEPWM2_CMPB 1
#define myEPWM2_CMPC 0
#define myEPWM2_CMPD 0
#define myEPWM2_DBRED 50
#define myEPWM2_DBFED 50
#define myEPWM2_TZA_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM2_TZB_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM2_INTERRUPT_SOURCE EPWM_INT_TBCTR_DISABLED
#define myEPWM3_BASE EPWM3_BASE
#define myEPWM3_TBPRD 10000
#define myEPWM3_COUNTER_MODE EPWM_COUNTER_MODE_UP_DOWN
#define myEPWM3_TBPHS 0
#define myEPWM3_CMPA 5000
#define myEPWM3_CMPB 1
#define myEPWM3_CMPC 0
#define myEPWM3_CMPD 0
#define myEPWM3_DBRED 50
#define myEPWM3_DBFED 50
#define myEPWM3_TZA_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM3_TZB_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM3_INTERRUPT_SOURCE EPWM_INT_TBCTR_DISABLED

//*****************************************************************************
//
// EQEP Configurations
//
//*****************************************************************************
#define myEQEP0_BASE EQEP1_BASE
void myEQEP0_init();

//*****************************************************************************
//
// GPIO Configurations
//
//*****************************************************************************
#define XINT1 16
void XINT1_init();
#define XINT2 17
void XINT2_init();
#define XINT3 18
void XINT3_init();
#define XINT4 19
void XINT4_init();

//*****************************************************************************
//
// INTERRUPT Configurations
//
//*****************************************************************************

// Interrupt Settings for IPC_1
// ISR need to be defined for the registered interrupts
#define IPC_1 INT_CIPC1
#define IPC_1_INTERRUPT_ACK_GROUP INTERRUPT_ACK_GROUP1
extern __interrupt void IPC_1_ISR(void);

//*****************************************************************************
//
// IPC Configurations
//
//*****************************************************************************
#define CPU1_to_CPU2_IPC_FLAG0 IPC_FLAG0
#define CPU1_to_CPU2_IPC_FLAG31 IPC_FLAG31
#define BOOT_MODE_CPU2 BOOTMODE_BOOT_TO_FLASH_SECTOR0

//*****************************************************************************
//
// SYNC Scheme Configurations
//
//*****************************************************************************

//*****************************************************************************
//
// SYSCTL Configurations
//
//*****************************************************************************

//*****************************************************************************
//
// Board Configurations
//
//*****************************************************************************
void	Board_init();
void	ADC_init();
void	EPWM_init();
void	EQEP_init();
void	GPIO_init();
void	INTERRUPT_init();
void	IPC_SYSCFG_init();
void	SYNC_init();
void	SYSCTL_init();
void	PinMux_init();

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif  // end of BOARD_H definition
