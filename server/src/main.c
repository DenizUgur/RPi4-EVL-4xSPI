/** @file       main.h
 *  @brief      main source file.
 *  @copyright  (c) 2021-denizugur - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of denizugur.
 *  @author     Deniz UGUR
 *  @contact	deniz.ugur@ozu.edu.tr
 *  @date       04.11.2021
 */
#define FILE_MAIN_C

/** INCLUDES ******************************************************************/
#include "main.h"
#include "server.h"
#include "encoder.h"
#include "control.h"
#include "daq.h"

/** CONSTANTS *****************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/
RT_TASK loop_task;
uint64_t RT_PERIOD;

/** LOCAL FUNCTION DECLARATIONS ***********************************************/
static uint64_t calculateRTSamplingTime(void);
static void stack_prefault(void);

/** INTERFACE FUNCTION DEFINITIONS ********************************************/
encoder_data_t Encoder;

/** Advantech Related Variables ***********************************************/
InstantAoCtrl * instantAoCtrl;
InstantAiCtrl * instantAiCtrl;
ErrorCode ret = Success;

/** LOCAL FUNCTION DEFINITIONS ************************************************/
void loop_task_proc(void *arg)
{
    RT_TASK *curtask;
    RT_TASK_INFO curtaskinfo;
    int iret = 0;

    curtask = rt_task_self();
    rt_task_inquire(curtask, &curtaskinfo);

    // Print the info
    printf("Starting task %s with period of %f ms ....\n", curtaskinfo.name, RTOS_PER_MILLI);

    // Make the task periodic with a specified loop period
    rt_task_set_periodic(NULL, TM_NOW, RT_PERIOD);

    FILE *fp;
    fp = fopen("Data_time.dat", "w");

    // Initialize Control Algorithm
    assert(0 == initializeControl());
    DAQInit(&instantAoCtrl, &instantAiCtrl);

    // Skip first frame
    rt_task_wait_period(NULL);

    RTIME tstart, now;
    tstart = rt_timer_read();
    RTIME prev_time = rt_timer_read();
    double task_time = 0;
    long ctr = 0;

    // Start the task loop
    while (task_time / 1000 <= TEST_TIME) {
        // Wait until next frame
        rt_task_wait_period(NULL);

        // Update time vector
        now = rt_timer_read();
        task_time = rt_timer_ticks2ns(now - tstart) / NSEC_PER_mSEC;
        double delta = rt_timer_ticks2ns(now - prev_time) / NSEC_PER_mSEC;
        prev_time = rt_timer_read();
        rt_fprintf(fp, "%.5f %.5f\n", task_time / 1000, delta);
        ctr++;

        // Read Encoders
        for (int cliID = 0; cliID < NUM_OF_CLIENTS; cliID++) {
            receiveEncoderData(connfd[cliID], cliID);
        }

        if (task_time / 1000 < START_TIME)
        {
            assert(0 == initialAction(task_time, &Encoder, instantAoCtrl, instantAiCtrl));
        }
        else {
            assert(0 == mainAction(task_time, &Encoder, instantAoCtrl, instantAiCtrl));
        }
    }

    for (int cliID = 0; cliID < NUM_OF_CLIENTS; cliID++) {
        rt_printf("closing");
        close(connfd[cliID]);
    }

    // Clean Up action for Control Algorithm
    assert(0 == cleanUp(instantAoCtrl));

    rt_printf("Done");
    rt_print_flush_buffers();
    fclose(fp);
}


typedef enum tagWaveStyle{ Sine, Sawtooth, Square }WaveStyle;

int main(int argc, char **argv)
{
    RT_PERIOD = calculateRTSamplingTime();

    // Initialize server
    ServerInit();

    // Prepare Real-Time Environment
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
    {
        perror("mlockall failed");
        exit(-2);
    }
    stack_prefault();

    // Prepare the Xenomai Task
    char str[20];
    sprintf(str, "cyclic_task");

    rt_task_create(&loop_task, str, 0, 50, 0);

    // Since task starts in suspended mode, start task
    printf("Starting cyclic task...\n");
    rt_task_start(&loop_task, &loop_task_proc, 0);
    rt_task_join(&loop_task);

    pause();

    return 0;
}

/**
 * This function calculates the sampling time of the RealTime system
 * takes no input but the Global define RTOS_FREQUENCY, available at
 * the top of main.c
 */
static uint64_t calculateRTSamplingTime(void)
{
    double period;
    period = round(RTOS_PER_MILLI * NSEC_PER_mSEC);
    return round(period);
}

/**
 * To prevent page faults in the memory
 */
static void stack_prefault(void)
{
    unsigned char dummy[MAX_SAFE_STACK];
    memset(dummy, 0, MAX_SAFE_STACK);
}