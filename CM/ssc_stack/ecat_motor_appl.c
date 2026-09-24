//#############################################################################
//
// FILE:   f2838x_cm_echoback.c
//
// TITLE:  EtherCAT Echoback Reference Solution for F2838x CM
//
// This reference solution demonstrates usage of the EtherCAT stack using
// CAN-over-EtherCAT (CoE) mailbox protocol to perform a basic loopback of
// data received from the EtherCAT master and then transmitted back to the
// master.
//
// This file initializes the device and EtherCAT hardware before starting
// the EtherCAT state machine handled by the slave stack code files. The
// slave stack code will call these "APPL" functions during state transitions
// as well as while running its main loop to copy data from the EtherCAT
// RAM to device RAM and from device RAM to EtherCAT RAM.
//
// Important: Before running this example, refer to the EtherCAT Slave
//            Controller User Guide in C2000Ware for the proper
//            setup/execution procedure of this example
//
// Note
//  Slave Stack Code (SSC) tool must be used to generate the stack files
//  required by this solution.
//
// External Connections
//  - The controlCARD RJ45 port 0 is connected to PC running TwinCAT master
//  - If distributed clocks enabled, connect and observe SYNC0/1 signals on
//    GPIO127 and GPIO128
//
// Watch Variables
//  - Switches0x6000 - 8-Bit switches data to send from slave to master
//  - LEDS0x7000 - 8-Bit LEDs data received from master
//  - DataToMaster0x6010 - 32-Bit general data to send from slave to master
//  - DatafromMaster0x7010 - 32-Bit general data received from master
//  - TargetModeResponse0x6012 - 16-Bit mode data to send from slave to master
//  - TargetMode0x7012 - 16-Bit mode data received from master
//  - TargetSpeedPosFeedback0x6014 - 32-Bit speed/position data to send from
//                                   slave to master
//  - TargetSpeedPosReq0x7014 - 32-Bit speed/position data received from master
//
//#############################################################################
// $TI Release: F2838x EtherCAT Software v2.01.00.00 $
// $Release Date: August 31 2020 $
// $Copyright: Copyright (C) 2020 Texas Instruments Incorporated -
//             http://www.ti.com/ ALL RIGHTS RESERVED $
//#############################################################################

//
// Included Files
//
#include <stdint.h>
#include <string.h>
#include "ecat_def.h"
#include "applInterface.h"
#include "ethercat_slave_cm_hal.h"
#include "../ecat_motor_app.h"
#include "../../ipc_protocol.h"

#define _F2838X_ECHOBACK_ 1
#include "f2838x_cm_echoback.h"
#undef _F2838X_ECHOBACK_

/* 1. 保存最近一次 RxPDO，只有数据变化时才产生新的 IPC 命令序号 */
static UINT32 g_ssc_last_command = 0U;                    // 最近一次命令号
static UINT32 g_ssc_last_dataw1 = 0U;                     // 最近一次参数1
static UINT32 g_ssc_last_dataw2 = 0U;                     // 最近一次参数2
static UINT32 g_ssc_rx_sequence = 0U;                     // CM 本地 RxPDO 序号

//
// PDO_ResetOutputs - Resets the Output data to zero
//
void PDO_ResetOutputs(void)
{
    LEDS0x7000.LED1 = 0x0U;
    LEDS0x7000.LED2 = 0x0U;
    LEDS0x7000.LED3 = 0x0U;
    LEDS0x7000.LED4 = 0x0U;
    LEDS0x7000.LED5 = 0x0U;
    LEDS0x7000.LED6 = 0x0U;
    LEDS0x7000.LED7 = 0x0U;
    LEDS0x7000.LED8 = 0x0U;

    DatafromMaster0x7010.DatafromMaster = 0x0UL;
    TargetMode0x7012.Mode = 0x0U;
    TargetSpeedPosReq0x7014.SpeedPosReq = 0x0UL;
    g_ssc_last_command = 0U;                               // 允许相同命令在重新进入 OP 后再次触发
    g_ssc_last_dataw1 = 0U;                                // 清除上次参数1缓存
    g_ssc_last_dataw2 = 0U;                                // 清除上次参数2缓存

    g_ecat_motor_command_rxpdo.command = 0U;               // 离开 OP 后清除待执行命令
    g_ecat_motor_command_rxpdo.dataw1 = 0U;                // 清除参数1
    g_ecat_motor_command_rxpdo.dataw2 = 0U;                // 清除参数2
    g_ecat_motor_command_rxpdo.sequence = ++g_ssc_rx_sequence; // 防止旧命令再次执行
}

//
// APPL_AckErrorInd - Called when error state was acknowledged by the master
//
void APPL_AckErrorInd(UINT16 stateTrans)
{
    //
    // No implementation for this application
    //
}

//
// APPL_StartMailboxHandler - Informs application of state transition from INIT
//                            to PREOP
//
UINT16 APPL_StartMailboxHandler(void)
{
    return(ALSTATUSCODE_NOERROR);
}

//
// APPL_StopMailboxHandler - Informs application of state transition from PREOP
//                           to INIT
//
UINT16 APPL_StopMailboxHandler(void)
{
    return(ALSTATUSCODE_NOERROR);
}

//
// APPL_StartInputHandler - Informs application of state transition from PREOP
//                          to SAFEOP
//
UINT16 APPL_StartInputHandler(UINT16 *pIntMask)
{
    return(ALSTATUSCODE_NOERROR);
}

//
// APPL_StopInputHandler - Informs application of state transition from SAFEOP
//                         PREOP
//
UINT16 APPL_StopInputHandler(void)
{
    return(ALSTATUSCODE_NOERROR);
}

//
// APPL_StartOutputHandler - Informs application of state transition from SAFEOP
//                           to OP
//
UINT16 APPL_StartOutputHandler(void)
{
    return(ALSTATUSCODE_NOERROR);
}

//
// APPL_StopOutputHandler - Informs application of state transition from OP to
//                          SAFEOP
//
UINT16 APPL_StopOutputHandler(void)
{
    //
    // Reset output data to zero
    //
    PDO_ResetOutputs();

    return(ALSTATUSCODE_NOERROR);
}

//
// APPL_GenerateMapping - Calculates the input and output process data sizes
//
UINT16 APPL_GenerateMapping(UINT16 *pInputSize, UINT16 *pOutputSize)
{
    UINT16 result = ALSTATUSCODE_NOERROR;
    UINT16 inputSize = 0U;
    UINT16 outputSize = 0U;
    UINT16 PDOAssignEntryCnt = 0U;
    OBJCONST TOBJECT OBJMEM * pPDO = NULL;
    UINT8 PDOSubindex0 = 0U;
    UINT8 *pPDOEntry = NULL;
    UINT8 PDOEntryCnt = 0U;

    //
    // Scan Object 0x1C12 (SyncManager 2)
    // (Sums up sizes of all SyncManager assigned RxPDOs subindexes)
    //
    for(PDOAssignEntryCnt = 0U; PDOAssignEntryCnt < sRxPDOassign.u16SubIndex0;
        PDOAssignEntryCnt++)
    {
        //
        // Get object handle for specified RxPDO assigned to SyncManager
        //
        pPDO = OBJ_GetObjectHandle(sRxPDOassign.aEntries[PDOAssignEntryCnt]);

        //
        // Confirm that Object isn't NULL before continuing
        //
        if(pPDO != NULL)
        {
            //
            // Get number of Object Entry subindexes
            //
            PDOSubindex0 = *((UINT8 *)pPDO->pVarPtr);

            //
            // Add up sizes of all the Object Entry subindexes
            //
            for(PDOEntryCnt = 0U; PDOEntryCnt < PDOSubindex0; PDOEntryCnt++)
            {
                pPDOEntry = ((UINT8 *)pPDO->pVarPtr +
                             (OBJ_GetEntryOffset((PDOEntryCnt + 1U),
                                                 pPDO) >> 3U));
                //
                // Increment the expected output size
                // depending on the mapped entry
                //
                outputSize += (UINT16)(*pPDOEntry);
            }
        }
        else
        {
            //
            // Assigned PDO was not found in object dictionary.
            // Return invalid mapping status.
            //
            outputSize = 0U;
            result = ALSTATUSCODE_INVALIDOUTPUTMAPPING;
            break;
        }
    }

    outputSize = (outputSize + 7U) >> 3U;

    //
    // Continue scanning of TXPDO if no error during RXPDO scanning
    //
    if(result == ALSTATUSCODE_NOERROR)
    {
        //
        // Scan Object 0x1C13 (SyncManager 3)
        // (Sums up sizes of all SyncManager assigned TXPDO subindexes)
        //
        for(PDOAssignEntryCnt = 0U;
            PDOAssignEntryCnt < sTxPDOassign.u16SubIndex0;
            PDOAssignEntryCnt++)
        {
            //
            // Get object handle for specified TxPDO assigned to SyncManager
            //
            pPDO =
             OBJ_GetObjectHandle(sTxPDOassign.aEntries[PDOAssignEntryCnt]);

            //
            // Confirm that Object isn't NULL before continuing
            //
            if(pPDO != NULL)
            {
                //
                // Get number of Object Entry subindexes
                //
                PDOSubindex0 = *((UINT8 *)pPDO->pVarPtr);

                //
                // Add up sizes of all the Object Entry subindexes
                //
                for(PDOEntryCnt = 0U; PDOEntryCnt < PDOSubindex0; PDOEntryCnt++)
                {
                    pPDOEntry = ((UINT8 *)pPDO->pVarPtr +
                                 (OBJ_GetEntryOffset((PDOEntryCnt + 1U),
                                                     pPDO) >> 3U));
                    //
                    // Increment the expected output size
                    // depending on the mapped entry
                    //
                    inputSize += (UINT16)(*pPDOEntry);
                }
            }
            else
            {
                //
                // Assigned PDO was not found in object dictionary.
                // Return invalid mapping status.
                //
                inputSize = 0U;
                result = ALSTATUSCODE_INVALIDINPUTMAPPING;
                break;
            }
        }
    }

    inputSize = (inputSize + 7U) >> 3U;

    //
    // Assign calculated sizes
    //
    *pInputSize = inputSize;
    *pOutputSize = outputSize;

    return(result);
}

//
// APPL_InputMapping - Copies the input data from local device memory to ESC
//                     memory
//
void APPL_InputMapping(UINT16 *pData)
{
    UINT16 j = 0U;
    UINT8 *pTmpData = (UINT8 *)pData;
    UINT8 data;

    //
    // Loop through all TxPDO entries and write out data
    //
    for(j = 0U; j < sTxPDOassign.u16SubIndex0; j++)
    {
        switch(sTxPDOassign.aEntries[j])
        {
            //
            // TxPDO 0 (Data from ESC to master)
            //
            case 0x1A00U:
                //
                // Switches (8-bit (Byte) Data)
                //
                data = ((Switches0x6000.Switch8 << 7U) |
                        (Switches0x6000.Switch7 << 6U) |
                        (Switches0x6000.Switch6 << 5U) |
                        (Switches0x6000.Switch5 << 4U) |
                        (Switches0x6000.Switch4 << 3U) |
                        (Switches0x6000.Switch3 << 2U) |
                        (Switches0x6000.Switch2 << 1U) |
                        Switches0x6000.Switch1);

                *(volatile UINT8 *)pTmpData = data;
                pTmpData++;
                break;
            //
            // TxPDO 1 (Data from ESC to master)
            //
            case 0x1A01U:
                //
                // DataToMaster (32 bits Data)
                //
                *(volatile UINT32 *)pTmpData =
                 DataToMaster0x6010.DataToMaster;
                pTmpData += 4U;

                //
                // ModeResponse (16 bits Data)
                //
                *(volatile UINT16 *)pTmpData =
                 TargetModeResponse0x6012.ModeResponse;
                pTmpData += 2U;

                //
                // SpeedPosFbk (32 bits Data)
                //
                *(volatile UINT32 *)pTmpData =
                 TargetSpeedPosFeedback0x6014.SpeedPosFbk;
                break;
        }
    }
}

//
// APPL_OutputMapping - Copies the output data from ESC memory to local device
//                      memory
//
void APPL_OutputMapping(UINT16 *pData)
{
    UINT16 j = 0U;
    UINT8 *pTmpData = (UINT8 *)pData;
    UINT8 data = 0U;

    //
    // Loop through all RxPDO entries and read in data
    //
    for(j = 0U; j < sRxPDOassign.u16SubIndex0; j++)
    {
        switch(sRxPDOassign.aEntries[j])
        {
            //
            // RxPDO 0 (Data from Master to ESC)
            //
            case 0x1600U:
                //
                // LEDS (8-bit (Byte) Data)
                //
                data = (*(volatile UINT8 *)pTmpData);

                (LEDS0x7000.LED1) = data & BIT_MASK;
                data = data >> 1U;
                (LEDS0x7000.LED2) = data & BIT_MASK;
                data = data >> 1U;
                (LEDS0x7000.LED3) = data & BIT_MASK;
                data = data >> 1U;
                (LEDS0x7000.LED4) = data & BIT_MASK;
                data = data >> 1U;
                (LEDS0x7000.LED5) = data & BIT_MASK;
                data = data >> 1U;
                (LEDS0x7000.LED6) = data & BIT_MASK;
                data = data >> 1U;
                (LEDS0x7000.LED7) = data & BIT_MASK;
                data = data >> 1U;
                (LEDS0x7000.LED8) = data & BIT_MASK;

                pTmpData++;
                break;
            //
            // RxPDO 1 (Data from Master to ESC)
            //
            case 0x1601U:
                //
                // DatafromMaster (32 bits Data)
                //
                DatafromMaster0x7010.DatafromMaster =
                 *(volatile UINT32 *)pTmpData;
                pTmpData += 4U;

                //
                // Mode (16 bits Data)
                //
                TargetMode0x7012.Mode = *(volatile UINT16 *)pTmpData;
                pTmpData += 2U;

                //
                // SpeedPosReq (32 bits Data)
                //
                TargetSpeedPosReq0x7014.SpeedPosReq =
                 *(volatile UINT32 *)pTmpData;
                break;
        }
    }

    /* 2. 将当前 EchoBack RxPDO 临时字段转换为电机 IPC 命令影子区 */
    if((g_ssc_last_command != DatafromMaster0x7010.DatafromMaster) ||
       (g_ssc_last_dataw1 != TargetSpeedPosReq0x7014.SpeedPosReq) ||
       (g_ssc_last_dataw2 != (UINT32)TargetMode0x7012.Mode))
    {
        g_ssc_last_command = DatafromMaster0x7010.DatafromMaster; // 0x7010 作为 IPC 命令号
        g_ssc_last_dataw1 = TargetSpeedPosReq0x7014.SpeedPosReq;  // 0x7014 作为参数1
        g_ssc_last_dataw2 = (UINT32)TargetMode0x7012.Mode;        // 0x7012 作为参数2

        g_ecat_motor_command_rxpdo.command = g_ssc_last_command;  // 写入命令影子区
        g_ecat_motor_command_rxpdo.dataw1 = g_ssc_last_dataw1;    // 写入参数1
        g_ecat_motor_command_rxpdo.dataw2 = g_ssc_last_dataw2;    // 写入参数2
        g_ecat_motor_command_rxpdo.sequence = ++g_ssc_rx_sequence; // 通知应用层有新命令
    }
}

//
// APPL_Application - Called as part of main stack loop. Assigns the output
//                    values to the input values for EtherCAT master
//
void APPL_Application(void)
{
    UINT32 currentSpeedBits = 0U;                          // 保存 current_speed 的 IEEE754 原始位

    /* 3. 先处理 CPU1 IPC，再刷新 SSC TxPDO 输出数据 */
    ECAT_MotorApplication_Process();                       // EtherCAT 命令通过 CM→CPU1 IPC 执行
    memcpy(&currentSpeedBits,                             // 将浮点转速按原始位发送给主站
           (const void *)&g_ecat_motor_telemetry_txpdo.currentSpeed,
           sizeof(currentSpeedBits));

    /* 4. 使用当前 EchoBack 对象承载 CPU1 的电机遥测和状态 */
    Switches0x6000.Switch1 =
        (g_ecat_motor_status_txpdo.motorStatus & IPC_MOTOR_STATUS_RUN_ENABLED) != 0U; // 运行状态
    Switches0x6000.Switch2 =
        (g_ecat_motor_status_txpdo.motorStatus & IPC_MOTOR_STATUS_ESTOP_ACTIVE) != 0U; // 急停状态
    Switches0x6000.Switch3 = (g_ecat_hw_init_status == ESC_HW_INIT_SUCCESS); // ESC 初始化状态
    Switches0x6000.Switch4 = (g_ecat_al_state == 0x0008U);   // OP 状态
    Switches0x6000.Switch5 = (g_cm_ipc_cmd_drop_count != 0U); // IPC 丢弃诊断
    Switches0x6000.Switch6 = (g_cm_ipc_error_count != 0U);    // IPC 错误诊断
    Switches0x6000.Switch7 = 0U;                              // 预留
    Switches0x6000.Switch8 = 0U;                              // 预留

    DataToMaster0x6010.DataToMaster =
        (UINT32)g_ecat_motor_telemetry_txpdo.encoderCount;    // 0x6010 返回 current_count
    TargetModeResponse0x6012.ModeResponse =
        (UINT16)(g_ecat_motor_status_txpdo.motorStatus & 0xFFFFU); // 0x6012 返回状态低16位
    TargetSpeedPosFeedback0x6014.SpeedPosFbk =
        currentSpeedBits;                                    // 0x6014 返回 current_speed 的 IEEE754 位
}

#if EXPLICIT_DEVICE_ID
//
// APPL_GetDeviceID - Return explicit device ID for F2838x.CM
//
UINT16 APPL_GetDeviceID(void)
{
    return(0x5U);
}
#endif // EXPLICIT_DEVICE_ID

#if USE_DEFAULT_MAIN
//
// Main Echoback Function
//
int main(void)
{
    //
    // Initialize device hardware and EtherCAT slave controller
    //
    HW_Init();

    //
    // Initialize Slave Stack
    //
    MainInit();

    bRunApplication = TRUE;
    do
    {
        //
        // Run Slave Stack
        //
        MainLoop();
    } while(bRunApplication == TRUE);

    //
    // De-Initialize device and resources
    //
    HW_Release();

    return(0U);
}
#endif // USE_DEFAULT_MAIN

//
// End of File
//
