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

#include "board.h"

//*****************************************************************************
//
// Board Configurations
// Initializes the rest of the modules.
// Call this function in your application if you wish to do all module
// initialization.
// If you wish to not use some of the initializations, instead of the
// Board_init use the individual Module_inits
//
//*****************************************************************************
void Board_init()
{
	EALLOW;

	IPC_SYSCFG_init();
	SCI_init();
	INTERRUPT_init();

	EDIS;
}


//*****************************************************************************
//
// INTERRUPT Configurations
//
//*****************************************************************************
void INTERRUPT_init(){
	
	// Interrupt Settings for IPC_0
	// ISR need to be defined for the registered interrupts
	Interrupt_register(IPC_0, &IPC_0_ISR);
	Interrupt_enable(IPC_0);
	
	// Interrupt Settings for INT_mySCI1_RX
	// ISR need to be defined for the registered interrupts
	Interrupt_register(INT_mySCI1_RX, &INT_mySCI1_RX_ISR);
	Interrupt_disable(INT_mySCI1_RX);
}
//*****************************************************************************
//
// IPC Configurations
//
//*****************************************************************************
void IPC_SYSCFG_init(){
}
//*****************************************************************************
//
// SCI Configurations
//
//*****************************************************************************
void SCI_init(){
	mySCI1_init();
}

void mySCI1_init(){
	SCI_clearInterruptStatus(mySCI1_BASE, SCI_INT_RXFF | SCI_INT_TXFF | SCI_INT_FE | SCI_INT_OE | SCI_INT_PE | SCI_INT_RXERR | SCI_INT_RXRDY_BRKDT | SCI_INT_TXRDY);
	SCI_clearOverflowStatus(mySCI1_BASE);
	SCI_resetTxFIFO(mySCI1_BASE);
	SCI_resetRxFIFO(mySCI1_BASE);
	SCI_resetChannels(mySCI1_BASE);
	SCI_setConfig(mySCI1_BASE, DEVICE_LSPCLK_FREQ, mySCI1_BAUDRATE, (SCI_CONFIG_WLEN_8|SCI_CONFIG_STOP_ONE|SCI_CONFIG_PAR_NONE));
	SCI_disableLoopback(mySCI1_BASE);
	SCI_performSoftwareReset(mySCI1_BASE);
	SCI_enableInterrupt(mySCI1_BASE, SCI_INT_RXFF);
	SCI_setFIFOInterruptLevel(mySCI1_BASE, SCI_FIFO_TX0, SCI_FIFO_RX1);
	SCI_enableInterrupt(mySCI1_BASE, SCI_INT_FE | SCI_INT_OE | SCI_INT_PE | SCI_INT_RXERR);
	SCI_enableFIFO(mySCI1_BASE);
	SCI_enableModule(mySCI1_BASE);
}

