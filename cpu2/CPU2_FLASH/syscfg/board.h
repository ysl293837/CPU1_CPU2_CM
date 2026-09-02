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
// SCIA -> mySCI1 Pinmux
//
//
// SCIA_RX - GPIO Settings
//
#define GPIO_PIN_SCIA_RX 28
#define mySCI1_SCIRX_GPIO 28
#define mySCI1_SCIRX_PIN_CONFIG GPIO_28_SCIA_RX
//
// SCIA_TX - GPIO Settings
//
#define GPIO_PIN_SCIA_TX 29
#define mySCI1_SCITX_GPIO 29
#define mySCI1_SCITX_PIN_CONFIG GPIO_29_SCIA_TX

//*****************************************************************************
//
// INTERRUPT Configurations
//
//*****************************************************************************

// Interrupt Settings for IPC_0
// ISR need to be defined for the registered interrupts
#define IPC_0 INT_CIPC0
#define IPC_0_INTERRUPT_ACK_GROUP INTERRUPT_ACK_GROUP1
extern __interrupt void IPC_0_ISR(void);

// Interrupt Settings for INT_mySCI1_RX
// ISR need to be defined for the registered interrupts
#define INT_mySCI1_RX INT_SCIA_RX
#define INT_mySCI1_RX_INTERRUPT_ACK_GROUP INTERRUPT_ACK_GROUP9
extern __interrupt void INT_mySCI1_RX_ISR(void);

//*****************************************************************************
//
// IPC Configurations
//
//*****************************************************************************
#define CPU2_to_CPU1_IPC_FLAG1 IPC_FLAG1
#define CPU2_to_CPU1_IPC_FLAG31 IPC_FLAG31

//*****************************************************************************
//
// SCI Configurations
//
//*****************************************************************************
#define mySCI1_BASE SCIA_BASE
#define mySCI1_BAUDRATE 115200
#define mySCI1_CONFIG_WLEN SCI_CONFIG_WLEN_8
#define mySCI1_CONFIG_STOP SCI_CONFIG_STOP_ONE
#define mySCI1_CONFIG_PAR SCI_CONFIG_PAR_NONE
#define mySCI1_FIFO_TX_LVL SCI_FIFO_TX0
#define mySCI1_FIFO_RX_LVL SCI_FIFO_RX1
void mySCI1_init();

//*****************************************************************************
//
// Board Configurations
//
//*****************************************************************************
void	Board_init();
void	INTERRUPT_init();
void	IPC_SYSCFG_init();
void	SCI_init();

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif  // end of BOARD_H definition
