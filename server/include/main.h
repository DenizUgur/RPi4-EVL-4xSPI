/** @file       main.h
 *  @brief      main header file.
 *  @copyright  (c) 2021-denizugur - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of denizugur.
 *  @author     Deniz UGUR
 *  @contact	deniz.ugur@ozu.edu.tr
 *  @date       04.11.2021
 */
#ifndef FILE_MAIN_H
#define FILE_MAIN_H

/** INCLUDES ******************************************************************/
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <math.h>
#include <sched.h>
#include <time.h>
#include <sys/mman.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

/** CONSTANTS *****************************************************************/

/* Uncomment below line if you need to print debug information to terminal */
//#define DEBUG

/* Uncomment below line if you need to save experiment results into a file */
#define LOG_EXPERIMENT

/* Uncomment below line if you want the experiment to continue forever
 * otherwise the test will continue until the specified time is passed
 * In that case adjust the TEST_TIME parameter.
 */
//#define CONTINUOUS

/*
 * If you want to unwrap encoder data in Linux, uncomment below line
 * beware that you have to implement
 */
//#define UNWRAP_IN_LINUX

// ---------------------------------------------------------------------------
// RTOS RELATED CONSTANTS
// ---------------------------------------------------------------------------
#define NSEC_PER_SEC        (1000000000.0)  ///< The number of nsecs per sec
#define NSEC_PER_mSEC       (1000000.0)     ///< The number of usecs per sec
#define mSEC_PER_SEC        (1000)        ///< The number of msecs per sec

#define MAX_SAFE_STACK      (10*1024)     ///< The maximum stack size which is guaranteed safe to access without faulting

#define RTOS_PER_MILLI      (0.5)        ///< Set period, enter period in milliseconds, f = 1/T
#define CLOCK_RES           (1e9)

#ifndef CONTINUOUS
#define TEST_TIME           (10)          ///< experiment duration in seconds
#define START_TIME          (1)          ///< Initial phase Duration
#endif

// ----------------------------------------------------------------------------
// PCI1723 DAC RELATED CONSTANTS
// ----------------------------------------------------------------------------
#define AMPLITUDE           (5.0)         ///< Set amplitude of the signal
#define V_OFFSET            (0)         ///< Voltage to give 0A output to motor drives
#define V_MAX               (10.0)        ///< Voltage max, i.e. +10A current reference
#define V_MIN               (0.0)         ///< Voltage min, i.e. -10A current reference

// ----------------------------------------------------------------------------
// TCP SERVER/CLIENT RELATED CONSTANTS
// ----------------------------------------------------------------------------
#define NUM_OF_CLIENTS      (4)           ///< Number of clients to be connected, MAX 4 clients are supported

// ----------------------------------------------------------------------------
// ENCODER RELATED CONSTANTS
// ----------------------------------------------------------------------------
#define GEAR_RATIO          (100)         ///< Motor gear ratio 1:100
#define NUM_OF_ENCODERS     (16)           ///< Number of Encoders in total

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/
// Returns the absolute value of a given number
#define ABS(x)	            (x < 0 ? -x : x)


#ifndef FILE_MAIN_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#undef INTERFACE // Should not let this roam free

#endif // FILE_MAIN_H