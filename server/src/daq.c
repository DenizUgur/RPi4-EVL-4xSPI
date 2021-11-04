/** @file       daq.h
 *  @brief      daq source file.
 *  @copyright  (c) 2021-denizugur - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of denizugur.
 *  @author     Deniz UGUR
 *  @contact	deniz.ugur@ozu.edu.tr
 *  @date       04.11.2021
 */
#define FILE_DAQ_C

/** INCLUDES ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <wchar.h>

#include "daq.h"

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/
ErrorCode retvalDO = Success;

/** LOCAL FUNCTION DECLARATIONS ***********************************************/

/** INTERFACE FUNCTION DEFINITIONS ********************************************/

/** LOCAL FUNCTION DEFINITIONS ************************************************/

/**
 * Initialize Advantech DAC Board - PCI1723
 * both Analog Output and Digital Output
 */


ErrorCode DAQInit(InstantAoCtrl ** pinstantAoCtrl, InstantAiCtrl ** pInstantAiCtrl)
{
    ErrorCode retval;
    const wchar_t* profilePath = L"/opt/advantech/examples/profile/DemoDevice.xml";

    /* DAC */
    *pinstantAoCtrl = InstantAoCtrl_Create();

    DeviceInformation devInfoAO;
    devInfoAO.DeviceNumber = -1;
    devInfoAO.DeviceMode = ModeWrite;
    devInfoAO.ModuleIndex = 0;
    wcscpy(devInfoAO.Description, DAC_Device);

    retval = InstantAoCtrl_setSelectedDevice(*pinstantAoCtrl, &devInfoAO);

    if(retval != Success)
    {
        perror("Error occurred during device selection!");
        printf("Error Code, Analog: 0x%X \n", retval);
        exit(-1);
    }

    InstantAoCtrl_LoadProfile(*pinstantAoCtrl, profilePath);

    /* ADC */
    *pInstantAiCtrl = InstantAiCtrl_Create();

    DeviceInformation devInfoAI;
    devInfoAI.DeviceNumber = -1;
    devInfoAI.DeviceMode = ModeWrite;
    devInfoAI.ModuleIndex = 0;
    wcscpy(devInfoAI.Description, ADC_Device);

    retval = InstantAiCtrl_setSelectedDevice(*pInstantAiCtrl, &devInfoAI);

    if(retval != Success)
    {
        perror("Error occurred during device selection!");
        printf("Error Code, Analog: 0x%X \n", retval);
        exit(-1);
    }

    InstantAiCtrl_LoadProfile(*pInstantAiCtrl, profilePath);

    return Success;
}

/**
 * This function writes the desired signal into the DAC device
 * returns 0 on success, error code on failure. Analog output.
 */
ErrorCode AnalogWrite(InstantAoCtrl ** obj, int chStart, int chCount, double *dataScaled)
{
    ErrorCode ret = Success;
    ret = InstantAoCtrl_WriteAny(*obj, chStart, chCount, NULL, dataScaled);
    return ret;
}

/**
 * This function writes the desired signal into the DAC device
 * returns 0 on success, error code on failure. Analog output.
 */
ErrorCode AnalogRead(InstantAiCtrl ** obj, int chStart, int chCount, double *dataScaled)
{
    ErrorCode ret = Success;
    ret = InstantAiCtrl_ReadAny(*obj, chStart, chCount, NULL, dataScaled);
    return ret;
}