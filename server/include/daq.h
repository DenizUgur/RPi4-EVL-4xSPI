/** @file       daq.h
 *  @brief      daq header file.
 *  @copyright  (c) 2021-denizugur - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of denizugur.
 *  @author     Deniz UGUR
 *  @contact	deniz.ugur@ozu.edu.tr
 *  @date       04.11.2021
 */
#ifndef FILE_DAQ_H
#define FILE_DAQ_H

/** INCLUDES ******************************************************************/
#include <stdint.h>
#include "/opt/advantech/inc/bdaqctrl.h"
#include "/opt/advantech/examples/ANSI_C_Example/inc/compatibility.h"


/** CONSTANTS *****************************************************************/
#define DAC_Device L"PCI-1723,BID#15"
#define ADC_Device L"PCI-1716L,BID#0"

/** MACROS ********************************************************************/

#ifndef FILE_DAQ_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/
// Initialize DAC board
ErrorCode DAQInit(InstantAoCtrl ** pinstantAoCtrl, InstantAiCtrl ** pinstantAiCtrl);

// To control analog outputs of the DAC - for ex: use this to generate a sinusoidal
ErrorCode AnalogWrite(InstantAoCtrl ** obj, int32 chStart, int32 chCount, double *dataScaled);

// To control analog outputs of the DAC - for ex: use this to generate a sinusoidal
ErrorCode AnalogRead(InstantAiCtrl ** obj, int32 chStart, int32 chCount, double *dataScaled);

#undef INTERFACE // Should not let this roam free

#endif // FILE_DAQ_H
