/** @file       control.h
 *  @brief      control source file.
 *  @copyright  (c) 2021-denizugur - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of denizugur.
 *  @author     Deniz UGUR
 *  @contact	deniz.ugur@ozu.edu.tr
 *  @date       04.11.2021
 */
#define FILE_CONTROL_C
#include "control.h"

// All of the functions should return "0" if successful.
// If anything else returns (e.g. -1), system will halt.
// This is an optional feature but might be useful.
// So if you want to take advantage of this, try to catch errors and return -1.

/** VARIABLES *****************************************************************/
FILE * fp_enc[NUM_OF_ENCODERS];

int initializeControl()
{
    // Initialize all vectors, matrices, etc.
    // You can also open file pointers here
    char file_name[20];
    for (int i = 0; i < NUM_OF_ENCODERS; i++) {
        sprintf(file_name, "data/enc_{%d}", i);
        fp_enc[i] = fopen(file_name, "w");
    }

    return 0;
}

int initialAction(double rt_time, encoder_data_t *encoder_data, InstantAoCtrl ** instantAoCtrl, InstantAiCtrl ** instantAiCtrl)
{
    // Real-Time Time will be provided along with encoder data.
    // Do all of the INITIAL calculations here
    // Use rt_printf() and rt_fprintf() instead of printf() and fprintf()
    // DO NOT log time vector as it will be recorded on main.c

    return 0;
}

int mainAction(double rt_time, encoder_data_t *encoder_data, InstantAoCtrl ** instantAoCtrl, InstantAiCtrl ** instantAiCtrl)
{
    // Real-Time Time will be provided along with encoder data.
    // Do all of the calculations here
    // Use rt_printf() and rt_fprintf() instead of printf() and fprintf()
    // DO NOT log time vector as it will be recorded on main.c

    double out = 5 * sin((double) rt_time * 2.0 * (3.14159) / 1024);
    AnalogWrite(&instantAoCtrl, 0, 1, &out);

    double in;
    AnalogRead(&instantAiCtrl, 0, 1, &in);

    int fp_i = 0;
    rt_printf("SET_1: ");
    for (int i = 0; i < 4; i++) {
        fprintf(fp_enc[fp_i++],"%.8f\n", encoder_data->encoderAngleSet0[i]);
        rt_printf("%.8f ", encoder_data->encoderAngleSet0[i]);
    }
    rt_printf("\n");

    rt_printf("SET_2: ");
    for (int i = 0; i < 4; i++) {
        fprintf(fp_enc[fp_i++],"%.8f\n", encoder_data->encoderAngleSet1[i]);
        rt_printf("%.8f ", encoder_data->encoderAngleSet1[i]);
    }
    rt_printf("\n");

    rt_printf("SET_3: ");
    for (int i = 0; i < 4; i++) {
        fprintf(fp_enc[fp_i++],"%.8f\n", encoder_data->encoderAngleSet2[i]);
        rt_printf("%.8f ", encoder_data->encoderAngleSet2[i]);
    }
    rt_printf("\n");

    rt_printf("SET_4: ");
    for (int i = 0; i < 4; i++) {
        fprintf(fp_enc[fp_i++],"%.8f\n", encoder_data->encoderAngleSet3[i]);
        rt_printf("%.8f ", encoder_data->encoderAngleSet3[i]);
    }

    rt_printf("\n\n");
    return 0;
}

int cleanUp(InstantAoCtrl ** instantAoCtrl)
{
    // Dispose DaQ Cards, close file pointers, etc.
    // Will be executed upon exit of main loop.
    for (int i = 0; i < NUM_OF_ENCODERS; i++) {
        fclose(fp_enc[i]);
    }

    return 0;
}