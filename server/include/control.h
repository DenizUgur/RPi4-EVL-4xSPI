/** @file       control.h
 *  @brief      control header file.
 *  @copyright  (c) 2021-denizugur - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of denizugur.
 *  @author     Deniz UGUR
 *  @contact	deniz.ugur@ozu.edu.tr
 *  @date       04.11.2021
 */
#ifndef FILE_CONTROL_H
#define FILE_CONTROL_H
#include "encoder.h"
#include "daq.h"

/** FUNCTIONS *****************************************************************/
int initializeControl();
int initialAction(double rt_time, encoder_data_t *encoder_data, InstantAoCtrl ** instantAoCtrl, InstantAiCtrl ** instantAiCtrl);
int mainAction(double rt_time, encoder_data_t *encoder_data, InstantAoCtrl ** instantAoCtrl, InstantAiCtrl ** instantAiCtrl);
int cleanUp(InstantAoCtrl ** instantAoCtrl);

#endif // FILE_CONTROL_H