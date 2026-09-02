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

	PinMux_init();
	SYSCTL_init();
	SYNC_init();
	ADC_init();
	EPWM_init();
	EQEP_init();
	GPIO_init();
	IPC_SYSCFG_init();
	INTERRUPT_init();

	EDIS;
}

//*****************************************************************************
//
// PINMUX Configurations
//
//*****************************************************************************
void PinMux_init()
{
	//
	// PinMux for modules assigned to CPU1
	//

	//
	// EPWM1 -> myEPWM1 Pinmux
	//
	GPIO_setPinConfig(myEPWM1_EPWMA_PIN_CONFIG);
	GPIO_setPadConfig(myEPWM1_EPWMA_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myEPWM1_EPWMA_GPIO, GPIO_QUAL_SYNC);

	GPIO_setPinConfig(myEPWM1_EPWMB_PIN_CONFIG);
	GPIO_setPadConfig(myEPWM1_EPWMB_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myEPWM1_EPWMB_GPIO, GPIO_QUAL_SYNC);

	//
	// EPWM2 -> myEPWM2 Pinmux
	//
	GPIO_setPinConfig(myEPWM2_EPWMA_PIN_CONFIG);
	GPIO_setPadConfig(myEPWM2_EPWMA_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myEPWM2_EPWMA_GPIO, GPIO_QUAL_SYNC);

	GPIO_setPinConfig(myEPWM2_EPWMB_PIN_CONFIG);
	GPIO_setPadConfig(myEPWM2_EPWMB_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myEPWM2_EPWMB_GPIO, GPIO_QUAL_SYNC);

	//
	// EPWM3 -> myEPWM3 Pinmux
	//
	GPIO_setPinConfig(myEPWM3_EPWMA_PIN_CONFIG);
	GPIO_setPadConfig(myEPWM3_EPWMA_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myEPWM3_EPWMA_GPIO, GPIO_QUAL_SYNC);

	GPIO_setPinConfig(myEPWM3_EPWMB_PIN_CONFIG);
	GPIO_setPadConfig(myEPWM3_EPWMB_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myEPWM3_EPWMB_GPIO, GPIO_QUAL_SYNC);

	//
	// EQEP1 -> myEQEP0 Pinmux
	//
	GPIO_setPinConfig(myEQEP0_EQEPA_PIN_CONFIG);
	GPIO_setPadConfig(myEQEP0_EQEPA_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myEQEP0_EQEPA_GPIO, GPIO_QUAL_SYNC);

	GPIO_setPinConfig(myEQEP0_EQEPB_PIN_CONFIG);
	GPIO_setPadConfig(myEQEP0_EQEPB_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myEQEP0_EQEPB_GPIO, GPIO_QUAL_SYNC);

	GPIO_setPinConfig(myEQEP0_EQEPSTROBE_PIN_CONFIG);
	GPIO_setPadConfig(myEQEP0_EQEPSTROBE_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myEQEP0_EQEPSTROBE_GPIO, GPIO_QUAL_SYNC);

	GPIO_setPinConfig(myEQEP0_EQEPINDEX_PIN_CONFIG);
	GPIO_setPadConfig(myEQEP0_EQEPINDEX_GPIO, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(myEQEP0_EQEPINDEX_GPIO, GPIO_QUAL_SYNC);

	// GPIO16 -> XINT1 Pinmux
	GPIO_setPinConfig(GPIO_16_GPIO16);
	// GPIO17 -> XINT2 Pinmux
	GPIO_setPinConfig(GPIO_17_GPIO17);
	// GPIO18 -> XINT3 Pinmux
	GPIO_setPinConfig(GPIO_18_GPIO18);
	// GPIO19 -> XINT4 Pinmux
	GPIO_setPinConfig(GPIO_19_GPIO19);

}

//*****************************************************************************
//
// ADC Configurations
//
//*****************************************************************************
void ADC_init(){
	myADC1_init();
}

void myADC1_init(){
	//
	// Configures the analog-to-digital converter module prescaler.
	//
	ADC_setPrescaler(myADC1_BASE, ADC_CLK_DIV_4_0);
	//
	// Configures the analog-to-digital converter resolution and signal mode.
	//
	ADC_setMode(myADC1_BASE, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);
	//
	// Sets the timing of the end-of-conversion pulse
	//
	ADC_setInterruptPulseMode(myADC1_BASE, ADC_PULSE_END_OF_ACQ_WIN);
	//
	// Sets the timing of early interrupt generation.
	//
	ADC_setInterruptCycleOffset(myADC1_BASE, 0U);
	//
	// Powers up the analog-to-digital converter core.
	//
	ADC_enableConverter(myADC1_BASE);
	//
	// Delay for 1ms to allow ADC time to power up
	//
	DEVICE_DELAY_US(500);
	//
	// SOC Configuration: Setup ADC EPWM channel and trigger settings
	//
	// Disables SOC burst mode.
	//
	ADC_disableBurstMode(myADC1_BASE);
	//
	// Sets the priority mode of the SOCs.
	//
	ADC_setSOCPriority(myADC1_BASE, ADC_PRI_ALL_ROUND_ROBIN);
}


//*****************************************************************************
//
// EPWM Configurations
//
//*****************************************************************************
void EPWM_init(){
    EPWM_setClockPrescaler(myEPWM1_BASE, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);	
    EPWM_setTimeBasePeriod(myEPWM1_BASE, 10000);	
    EPWM_setupEPWMLinks(myEPWM1_BASE, EPWM_LINK_WITH_EPWM_1, EPWM_LINK_TBPRD);	
    EPWM_setTimeBaseCounter(myEPWM1_BASE, 0);	
    EPWM_setTimeBaseCounterMode(myEPWM1_BASE, EPWM_COUNTER_MODE_UP_DOWN);	
    EPWM_disablePhaseShiftLoad(myEPWM1_BASE);	
    EPWM_setPhaseShift(myEPWM1_BASE, 0);	
    EPWM_setSyncInPulseSource(myEPWM1_BASE, EPWM_SYNC_IN_PULSE_SRC_DISABLE);	
    EPWM_enableSyncOutPulseSource(myEPWM1_BASE, EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);	
    EPWM_setCounterCompareValue(myEPWM1_BASE, EPWM_COUNTER_COMPARE_A, 5000);	
    EPWM_setCounterCompareShadowLoadMode(myEPWM1_BASE, EPWM_COUNTER_COMPARE_A, EPWM_COMP_LOAD_ON_CNTR_ZERO);	
    EPWM_setCounterCompareValue(myEPWM1_BASE, EPWM_COUNTER_COMPARE_B, 1);	
    EPWM_disableCounterCompareShadowLoadMode(myEPWM1_BASE, EPWM_COUNTER_COMPARE_B);	
    EPWM_setCounterCompareShadowLoadMode(myEPWM1_BASE, EPWM_COUNTER_COMPARE_B, EPWM_COMP_LOAD_ON_CNTR_ZERO);	
    EPWM_setActionQualifierShadowLoadMode(myEPWM1_BASE, EPWM_ACTION_QUALIFIER_A, EPWM_AQ_LOAD_ON_CNTR_ZERO);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);	
    EPWM_disableActionQualifierShadowLoadMode(myEPWM1_BASE, EPWM_ACTION_QUALIFIER_B);	
    EPWM_setActionQualifierShadowLoadMode(myEPWM1_BASE, EPWM_ACTION_QUALIFIER_B, EPWM_AQ_LOAD_ON_CNTR_ZERO);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);	
    EPWM_setActionQualifierAction(myEPWM1_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);	
    EPWM_setDeadBandDelayPolarity(myEPWM1_BASE, EPWM_DB_FED, EPWM_DB_POLARITY_ACTIVE_LOW);	
    EPWM_setDeadBandDelayMode(myEPWM1_BASE, EPWM_DB_RED, true);	
    EPWM_setRisingEdgeDelayCountShadowLoadMode(myEPWM1_BASE, EPWM_RED_LOAD_ON_CNTR_ZERO);	
    EPWM_disableRisingEdgeDelayCountShadowLoadMode(myEPWM1_BASE);	
    EPWM_setRisingEdgeDelayCount(myEPWM1_BASE, 50);	
    EPWM_setDeadBandDelayMode(myEPWM1_BASE, EPWM_DB_FED, true);	
    EPWM_setFallingEdgeDelayCountShadowLoadMode(myEPWM1_BASE, EPWM_FED_LOAD_ON_CNTR_ZERO);	
    EPWM_disableFallingEdgeDelayCountShadowLoadMode(myEPWM1_BASE);	
    EPWM_setFallingEdgeDelayCount(myEPWM1_BASE, 50);	
    EPWM_setDeadBandControlShadowLoadMode(myEPWM1_BASE, EPWM_DB_LOAD_ON_CNTR_ZERO);	
    EPWM_disableDeadBandControlShadowLoadMode(myEPWM1_BASE);	
    EPWM_enableInterrupt(myEPWM1_BASE);	
    EPWM_setInterruptSource(myEPWM1_BASE, EPWM_INT_TBCTR_ZERO);	
    EPWM_setInterruptEventCount(myEPWM1_BASE, 1);	
    EPWM_enableADCTrigger(myEPWM1_BASE, EPWM_SOC_A);	
    EPWM_setADCTriggerSource(myEPWM1_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_ZERO);	
    EPWM_setADCTriggerEventPrescale(myEPWM1_BASE, EPWM_SOC_A, 1);	
    EPWM_setClockPrescaler(myEPWM2_BASE, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);	
    EPWM_setTimeBasePeriod(myEPWM2_BASE, 10000);	
    EPWM_setupEPWMLinks(myEPWM2_BASE, EPWM_LINK_WITH_EPWM_1, EPWM_LINK_TBPRD);	
    EPWM_setTimeBaseCounter(myEPWM2_BASE, 0);	
    EPWM_setTimeBaseCounterMode(myEPWM2_BASE, EPWM_COUNTER_MODE_UP_DOWN);	
    EPWM_setCountModeAfterSync(myEPWM2_BASE, EPWM_COUNT_MODE_UP_AFTER_SYNC);	
    EPWM_enablePhaseShiftLoad(myEPWM2_BASE);	
    EPWM_setPhaseShift(myEPWM2_BASE, 0);	
    EPWM_enableSyncOutPulseSource(myEPWM2_BASE, EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);	
    EPWM_setCounterCompareValue(myEPWM2_BASE, EPWM_COUNTER_COMPARE_A, 5000);	
    EPWM_setCounterCompareShadowLoadMode(myEPWM2_BASE, EPWM_COUNTER_COMPARE_A, EPWM_COMP_LOAD_ON_CNTR_ZERO);	
    EPWM_setCounterCompareValue(myEPWM2_BASE, EPWM_COUNTER_COMPARE_B, 1);	
    EPWM_disableCounterCompareShadowLoadMode(myEPWM2_BASE, EPWM_COUNTER_COMPARE_B);	
    EPWM_setCounterCompareShadowLoadMode(myEPWM2_BASE, EPWM_COUNTER_COMPARE_B, EPWM_COMP_LOAD_ON_CNTR_ZERO);	
    EPWM_setActionQualifierShadowLoadMode(myEPWM2_BASE, EPWM_ACTION_QUALIFIER_A, EPWM_AQ_LOAD_ON_CNTR_ZERO);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);	
    EPWM_disableActionQualifierShadowLoadMode(myEPWM2_BASE, EPWM_ACTION_QUALIFIER_B);	
    EPWM_setActionQualifierShadowLoadMode(myEPWM2_BASE, EPWM_ACTION_QUALIFIER_B, EPWM_AQ_LOAD_ON_CNTR_ZERO);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);	
    EPWM_setActionQualifierAction(myEPWM2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);	
    EPWM_setDeadBandDelayPolarity(myEPWM2_BASE, EPWM_DB_FED, EPWM_DB_POLARITY_ACTIVE_LOW);	
    EPWM_setDeadBandDelayMode(myEPWM2_BASE, EPWM_DB_RED, true);	
    EPWM_setRisingEdgeDelayCountShadowLoadMode(myEPWM2_BASE, EPWM_RED_LOAD_ON_CNTR_ZERO);	
    EPWM_disableRisingEdgeDelayCountShadowLoadMode(myEPWM2_BASE);	
    EPWM_setRisingEdgeDelayCount(myEPWM2_BASE, 50);	
    EPWM_setDeadBandDelayMode(myEPWM2_BASE, EPWM_DB_FED, true);	
    EPWM_setFallingEdgeDelayCountShadowLoadMode(myEPWM2_BASE, EPWM_FED_LOAD_ON_CNTR_ZERO);	
    EPWM_disableFallingEdgeDelayCountShadowLoadMode(myEPWM2_BASE);	
    EPWM_setFallingEdgeDelayCount(myEPWM2_BASE, 50);	
    EPWM_setDeadBandControlShadowLoadMode(myEPWM2_BASE, EPWM_DB_LOAD_ON_CNTR_ZERO);	
    EPWM_disableDeadBandControlShadowLoadMode(myEPWM2_BASE);	
    EPWM_setClockPrescaler(myEPWM3_BASE, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);	
    EPWM_setTimeBasePeriod(myEPWM3_BASE, 10000);	
    EPWM_setupEPWMLinks(myEPWM3_BASE, EPWM_LINK_WITH_EPWM_1, EPWM_LINK_TBPRD);	
    EPWM_setTimeBaseCounter(myEPWM3_BASE, 0);	
    EPWM_setTimeBaseCounterMode(myEPWM3_BASE, EPWM_COUNTER_MODE_UP_DOWN);	
    EPWM_disablePhaseShiftLoad(myEPWM3_BASE);	
    EPWM_setPhaseShift(myEPWM3_BASE, 0);	
    EPWM_enableSyncOutPulseSource(myEPWM3_BASE, EPWM_SYNC_OUT_PULSE_ON_CNTR_ZERO);	
    EPWM_setCounterCompareValue(myEPWM3_BASE, EPWM_COUNTER_COMPARE_A, 5000);	
    EPWM_setCounterCompareShadowLoadMode(myEPWM3_BASE, EPWM_COUNTER_COMPARE_A, EPWM_COMP_LOAD_ON_CNTR_ZERO);	
    EPWM_setCounterCompareValue(myEPWM3_BASE, EPWM_COUNTER_COMPARE_B, 1);	
    EPWM_disableCounterCompareShadowLoadMode(myEPWM3_BASE, EPWM_COUNTER_COMPARE_B);	
    EPWM_setCounterCompareShadowLoadMode(myEPWM3_BASE, EPWM_COUNTER_COMPARE_B, EPWM_COMP_LOAD_ON_CNTR_ZERO);	
    EPWM_setActionQualifierShadowLoadMode(myEPWM3_BASE, EPWM_ACTION_QUALIFIER_A, EPWM_AQ_LOAD_ON_CNTR_ZERO);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);	
    EPWM_disableActionQualifierShadowLoadMode(myEPWM3_BASE, EPWM_ACTION_QUALIFIER_B);	
    EPWM_setActionQualifierShadowLoadMode(myEPWM3_BASE, EPWM_ACTION_QUALIFIER_B, EPWM_AQ_LOAD_ON_CNTR_ZERO);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);	
    EPWM_setActionQualifierAction(myEPWM3_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);	
    EPWM_setDeadBandDelayPolarity(myEPWM3_BASE, EPWM_DB_FED, EPWM_DB_POLARITY_ACTIVE_LOW);	
    EPWM_setDeadBandDelayMode(myEPWM3_BASE, EPWM_DB_RED, true);	
    EPWM_setRisingEdgeDelayCountShadowLoadMode(myEPWM3_BASE, EPWM_RED_LOAD_ON_CNTR_ZERO);	
    EPWM_disableRisingEdgeDelayCountShadowLoadMode(myEPWM3_BASE);	
    EPWM_setRisingEdgeDelayCount(myEPWM3_BASE, 50);	
    EPWM_setDeadBandDelayMode(myEPWM3_BASE, EPWM_DB_FED, true);	
    EPWM_setFallingEdgeDelayCountShadowLoadMode(myEPWM3_BASE, EPWM_FED_LOAD_ON_CNTR_ZERO);	
    EPWM_disableFallingEdgeDelayCountShadowLoadMode(myEPWM3_BASE);	
    EPWM_setFallingEdgeDelayCount(myEPWM3_BASE, 50);	
    EPWM_setDeadBandControlShadowLoadMode(myEPWM3_BASE, EPWM_DB_LOAD_ON_CNTR_ZERO);	
    EPWM_disableDeadBandControlShadowLoadMode(myEPWM3_BASE);	
}

//*****************************************************************************
//
// EQEP Configurations
//
//*****************************************************************************
void EQEP_init(){
	myEQEP0_init();
}

void myEQEP0_init(){
	EQEP_SourceSelect source_myEQEP0 =
	{
		EQEP_SOURCE_DEVICE_PIN, 		// eQEPA source
		EQEP_SOURCE_DEVICE_PIN,		// eQEPB source
		EQEP_SOURCE_DEVICE_PIN,  	// eQEP Index source
	};
	//
	// Selects the source for eQEPA/B/I signals
	//
	EQEP_selectSource(myEQEP0_BASE, source_myEQEP0);
	//
	// Set the strobe input source of the eQEP module.
	//
	EQEP_setStrobeSource(myEQEP0_BASE,EQEP_STROBE_FROM_GPIO);
	//
	// Sets the polarity of the eQEP module's input signals.
	//
	EQEP_setInputPolarity(myEQEP0_BASE,false,false,false,false);
	//
	// Configures eQEP module's quadrature decoder unit.
	//
	EQEP_setDecoderConfig(myEQEP0_BASE, (EQEP_CONFIG_QUADRATURE | EQEP_CONFIG_2X_RESOLUTION | EQEP_CONFIG_NO_SWAP | EQEP_CONFIG_IGATE_DISABLE));
	//
	// Set the emulation mode of the eQEP module.
	//
	EQEP_setEmulationMode(myEQEP0_BASE,EQEP_EMULATIONMODE_RUNFREE);
	//
	// Configures eQEP module position counter unit.
	//
	EQEP_setPositionCounterConfig(myEQEP0_BASE,EQEP_POSITION_RESET_IDX,9999U);
	//
	// Sets the current encoder position.
	//
	EQEP_setPosition(myEQEP0_BASE,0U);
	//
	// Disables the eQEP module unit timer.
	//
	EQEP_disableUnitTimer(myEQEP0_BASE);
	//
	// Disables the eQEP module watchdog timer.
	//
	EQEP_disableWatchdog(myEQEP0_BASE);
	//
	// Configures the quadrature modes in which the position count can be latched.
	//
	EQEP_setLatchMode(myEQEP0_BASE,(EQEP_LATCH_CNT_READ_BY_CPU|EQEP_LATCH_RISING_STROBE|EQEP_LATCH_RISING_INDEX));
	//
	// Set the quadrature mode adapter (QMA) module mode.
	//
	EQEP_setQMAModuleMode(myEQEP0_BASE,EQEP_QMA_MODE_BYPASS);
	//
	// Disable Direction Change During Index
	//
	EQEP_disableDirectionChangeDuringIndex(myEQEP0_BASE);
	//
	// Configures the mode in which the position counter is initialized.
	//
	EQEP_setPositionInitMode(myEQEP0_BASE,(EQEP_INIT_DO_NOTHING));
	//
	// Sets the software initialization of the encoder position counter.
	//
	EQEP_setSWPositionInit(myEQEP0_BASE,false);
	//
	// Sets the init value for the encoder position counter.
	//
	EQEP_setInitialPosition(myEQEP0_BASE,0U);
	//
	// Enables the eQEP module.
	//
	EQEP_enableModule(myEQEP0_BASE);
	//
	// Configures eQEP module edge-capture unit.
	//
	EQEP_setCaptureConfig(myEQEP0_BASE,EQEP_CAPTURE_CLK_DIV_1,EQEP_UNIT_POS_EVNT_DIV_4);
	//
	// Enables the eQEP module edge-capture unit.
	//
	EQEP_enableCapture(myEQEP0_BASE);
}

//*****************************************************************************
//
// GPIO Configurations
//
//*****************************************************************************
void GPIO_init(){
	XINT1_init();
	XINT2_init();
	XINT3_init();
	XINT4_init();
}

void XINT1_init(){
	GPIO_setPadConfig(XINT1, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
	GPIO_setQualificationMode(XINT1, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(XINT1, GPIO_DIR_MODE_IN);
	GPIO_setControllerCore(XINT1, GPIO_CORE_CPU1);
}
void XINT2_init(){
	GPIO_setPadConfig(XINT2, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
	GPIO_setQualificationMode(XINT2, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(XINT2, GPIO_DIR_MODE_IN);
	GPIO_setControllerCore(XINT2, GPIO_CORE_CPU1);
}
void XINT3_init(){
	GPIO_setPadConfig(XINT3, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
	GPIO_setQualificationMode(XINT3, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(XINT3, GPIO_DIR_MODE_IN);
	GPIO_setControllerCore(XINT3, GPIO_CORE_CPU1);
}
void XINT4_init(){
	GPIO_setPadConfig(XINT4, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
	GPIO_setQualificationMode(XINT4, GPIO_QUAL_SYNC);
	GPIO_setDirectionMode(XINT4, GPIO_DIR_MODE_IN);
	GPIO_setControllerCore(XINT4, GPIO_CORE_CPU1);
}

//*****************************************************************************
//
// INTERRUPT Configurations
//
//*****************************************************************************
void INTERRUPT_init(){
	
	// Interrupt Settings for IPC_1
	// ISR need to be defined for the registered interrupts
	Interrupt_register(IPC_1, &IPC_1_ISR);
	Interrupt_enable(IPC_1);
}
//*****************************************************************************
//
// IPC Configurations
//
//*****************************************************************************
void IPC_SYSCFG_init(){
    //
    // Paste the following line in your main() function after device_init, if you would like CPU2 to boot
    // Device_bootCPU2(BOOT_MODE_CPU2);           
    //
}
//*****************************************************************************
//
// SYNC Scheme Configurations
//
//*****************************************************************************
void SYNC_init(){
	SysCtl_setSyncOutputConfig(SYSCTL_SYNC_OUT_SRC_EPWM1SYNCOUT);
	//
	// SOCA
	//
	SysCtl_enableExtADCSOCSource(0);
	//
	// SOCB
	//
	SysCtl_enableExtADCSOCSource(0);
}
//*****************************************************************************
//
// SYSCTL Configurations
//
//*****************************************************************************
void SYSCTL_init(){
	//
    // sysctl initialization
	//
    SysCtl_setStandbyQualificationPeriod(2);
    SysCtl_configureType(SYSCTL_USBTYPE, 0, 0);
    SysCtl_configureType(SYSCTL_ECAPTYPE, 0, 0);
    SysCtl_configureType(SYSCTL_SDFMTYPE, 0, 0);
    SysCtl_configureType(SYSCTL_MEMMAPTYPE, 0, 0);
    SysCtl_setSemOwner(SYSCTL_CPUSEL_CPU1);
    SysCtl_selectErrPinPolarity(0);

    SysCtl_disableMCD();

    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 3, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 4, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 5, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 6, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 7, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 8, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 9, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 10, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 12, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 13, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 14, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 15, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 16, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL1_ECAP, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL1_ECAP, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL1_ECAP, 3, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL1_ECAP, 4, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL1_ECAP, 5, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL1_ECAP, 6, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL1_ECAP, 7, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL2_EQEP, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL2_EQEP, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL2_EQEP, 3, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL4_SD, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL4_SD, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL5_SCI, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL5_SCI, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL5_SCI, 3, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL5_SCI, 4, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL6_SPI, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL6_SPI, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL6_SPI, 3, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL6_SPI, 4, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL7_I2C, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL7_I2C, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL8_CAN, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL8_CAN, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL9_MCBSP, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL9_MCBSP, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL11_ADC, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL11_ADC, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL11_ADC, 3, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL11_ADC, 4, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL12_CMPSS, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL12_CMPSS, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL12_CMPSS, 3, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL12_CMPSS, 4, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL12_CMPSS, 5, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL12_CMPSS, 6, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL12_CMPSS, 7, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL12_CMPSS, 8, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL14_DAC, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL14_DAC, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL14_DAC, 3, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL15_CLB, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL15_CLB, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL15_CLB, 3, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL15_CLB, 4, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL15_CLB, 5, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL15_CLB, 6, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL15_CLB, 7, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL15_CLB, 8, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL16_FSI, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL16_FSI, 2, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL16_FSI, 3, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL16_FSI, 4, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL16_FSI, 5, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL16_FSI, 6, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL16_FSI, 7, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL16_FSI, 8, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL16_FSI, 9, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL16_FSI, 10, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL18_PMBUS, 1, SYSCTL_CPUSEL_CPU1);
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL25_HRCAL, 1, SYSCTL_CPUSEL_CPU1);

    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCA, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCA, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCA, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCB, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCB, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCB, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCC, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCC, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCC, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCD, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCD, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ADCD, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS1, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS1, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS1, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS2, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS2, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS2, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS3, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS3, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS3, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS4, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS4, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS4, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS5, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS5, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS5, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS6, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS6, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS6, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS7, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS7, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS7, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS8, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS8, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CMPSS8, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_DACA, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_DACA, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_DACA, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_DACB, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_DACB, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_DACB, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_DACC, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_DACC, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_DACC, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM1, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM1, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM1, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM2, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM2, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM2, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM3, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM3, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM3, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM4, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM4, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM4, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM5, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM5, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM5, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM6, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM6, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM6, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM7, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM7, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM7, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM8, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM8, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM8, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM9, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM9, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM9, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM10, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM10, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM10, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM11, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM11, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM11, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM12, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM12, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM12, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM13, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM13, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM13, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM14, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM14, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM14, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM15, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM15, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM15, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM16, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM16, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EPWM16, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EQEP1, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EQEP1, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EQEP1, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EQEP2, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EQEP2, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EQEP2, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EQEP3, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EQEP3, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_EQEP3, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP1, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP1, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP1, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP2, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP2, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP2, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP3, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP3, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP3, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP4, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP4, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP4, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP5, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP5, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP5, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP6, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP6, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP6, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP7, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP7, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAP7, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SDFM1, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SDFM1, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SDFM1, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SDFM2, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SDFM2, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SDFM2, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB1, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB1, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB1, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB2, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB2, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB2, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB3, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB3, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB3, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB4, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB4, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB4, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB5, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB5, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB5, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB6, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB6, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB6, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB7, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB7, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB7, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB8, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB8, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CLB8, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPIA, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPIA, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPIA, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPIB, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPIB, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPIB, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPIC, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPIC, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPIC, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPID, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPID, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_SPID, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_PMBUSA, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_PMBUSA, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_PMBUSA, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CANA, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CANA, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CANA, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CANB, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CANB, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_CANB, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_MCBSPA, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_MCBSPA, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_MCBSPA, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_MCBSPB, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_MCBSPB, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_MCBSPB, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_USBA, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_USBA, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_USBA, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_HRPWM, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_HRPWM, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_HRPWM, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAT, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAT, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_ECAT, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIATX, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIATX, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIATX, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIARX, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIARX, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIARX, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIBTX, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIBTX, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIBTX, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIBRX, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIBRX, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIBRX, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSICRX, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSICRX, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSICRX, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIDRX, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIDRX, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIDRX, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIERX, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIERX, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIERX, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIFRX, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIFRX, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIFRX, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIGRX, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIGRX, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIGRX, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIHRX, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIHRX, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_FSIHRX, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_MCANA, 
        SYSCTL_ACCESS_CPUX, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_MCANA, 
        SYSCTL_ACCESS_CLA1, SYSCTL_ACCESS_FULL);
    SysCtl_setPeripheralAccessControl(SYSCTL_ACCESS_MCANA, 
        SYSCTL_ACCESS_DMA1, SYSCTL_ACCESS_FULL);
    SysCtl_allocateSharedPeripheral(SYSCTL_PALLOCATE_USBA, 0);
    SysCtl_allocateSharedPeripheral(SYSCTL_PALLOCATE_ETHERCAT, 0);
    SysCtl_allocateSharedPeripheral(SYSCTL_PALLOCATE_CAN_A, 0);
    SysCtl_allocateSharedPeripheral(SYSCTL_PALLOCATE_CAN_B, 0);
    SysCtl_allocateSharedPeripheral(SYSCTL_PALLOCATE_MCAN_A, 0);

    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CLA1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_DMA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TIMER0);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TIMER1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TIMER2);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CPUBGCRC);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CLA1BGCRC);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_HRCAL);
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_GTBCLKSYNC);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ERAD);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EMIF1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EMIF2);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM2);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM3);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM4);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM5);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM6);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM7);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM8);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM9);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM10);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM11);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM12);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM13);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM14);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM15);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM16);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ECAP1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ECAP2);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ECAP3);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ECAP4);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ECAP5);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ECAP6);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ECAP7);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EQEP1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EQEP2);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EQEP3);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SD1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SD2);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SCIA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SCIB);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SCIC);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SCID);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SPIA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SPIB);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SPIC);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SPID);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_I2CA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_I2CB);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CANA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CANB);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_MCANA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_MCBSPA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_MCBSPB);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_USBA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ADCA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ADCB);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ADCC);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ADCD);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CMPSS1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CMPSS2);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CMPSS3);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CMPSS4);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CMPSS5);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CMPSS6);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CMPSS7);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CMPSS8);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_DACA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_DACB);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_DACC);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CLB1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CLB2);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CLB3);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CLB4);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CLB5);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CLB6);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CLB7);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_CLB8);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_FSITXA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_FSITXB);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_FSIRXA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_FSIRXB);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_FSIRXC);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_FSIRXD);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_FSIRXE);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_FSIRXF);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_FSIRXG);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_FSIRXH);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_PMBUSA);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_DCC0);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_DCC1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_DCC2);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_MPOSTCLK);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ECAT);



}

