/** @file       encoder.h
 *  @brief      encoder header file.
 *  @copyright  (c) 2021-denizugur - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of denizugur.
 *  @author     Deniz UGUR
 *  @contact	deniz.ugur@ozu.edu.tr
 *  @date       04.11.2021
 */
#ifndef FILE_ENCODER_H
#define FILE_ENCODER_H

/** INCLUDES ******************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "main.h"

/** CONSTANTS *****************************************************************/
#define BUFFER_LENGTH (256) ///< Buffer size for incoming data

#define PI (3.141592654)           ///< pi
#define PI2 (6.283185307)          ///< 2 * pi
#define ONE_OVER_2PI (0.159154943) ///< 1 / 2pi
#define ENCODER_CONSTANT (PI2 / (pow(2, 23) - 1))
#define SPEED_FROM_CLIENT (true)

#define UNWRAP_IN_LINUX

/** TYPEDEFS ******************************************************************/
#ifdef UNWRAP_IN_LINUX
/*!
 * @brief Encoder data structure
 *
 * This structure stores angles of each independent STM32DAQ board
 * modify it according to how many STM32DAQ boards are used. Below
 * struct stores incoming data from up to 4 clients (STM32DAQ boards)
 * each having NUM_OF_ENCODERS amount of encoders.
 */
typedef struct
{
    double encoderAngleSet0[4]; ///< buffer to store angle information in set 1
    double encoderAngleSet1[4]; ///< buffer to store angle information in set 2
    double encoderAngleSet2[4]; ///< buffer to store angle information in set 3
    double encoderAngleSet3[4]; ///< buffer to store angle information in set 3

#if SPEED_FROM_CLIENT
    double encoderSpeedSet0[4]; ///< buffer to store speed information in set 1
    double encoderSpeedSet1[4]; ///< buffer to store speed information in set 2
    double encoderSpeedSet2[4]; ///< buffer to store speed information in set 3
    double encoderSpeedSet3[4]; ///< buffer to store speed information in set 3
#endif
} encoder_data_t;
#endif

/** MACROS ********************************************************************/

#ifndef FILE_ENCODER_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

/** VARIABLES *****************************************************************/
INTERFACE uint8_t receiveBuffer[BUFFER_LENGTH]; ///< buffer to store incoming data from STM32DAQ ///< singleTurn * (2*pi / (2^23-1))
INTERFACE encoder_data_t Encoder;               ///< Structure to hold encoder data from clients

/** FUNCTIONS *****************************************************************/
void receiveEncoderData(int socketfd, uint8_t clientID);
bool parseEncoderData(uint8_t const *buffer, size_t len);
double parseData(char *data, uint8_t clientID);

double roundValues(double val);
double unwrap(uint64_t raw);
int fix(double num);

#undef INTERFACE // Should not let this roam free

#endif // FILE_ENCODER_H
