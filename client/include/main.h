#ifndef HEADER_MAIN
#define HEADER_MAIN

/* Standard */
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <inttypes.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <linux/spi/spidev.h>
#include <evl/sem.h>

/* EVL */
#include <evl/thread.h>
#include <evl/clock.h>
#include <evl/timer.h>
#include <evl/proxy.h>
#include <uapi/evl/devices/spidev.h>

/* Include */
#include "color.h"
#include "helpers.h"

/* Porgram specific variables */
#define CLIENT_SPI_DEV_NUM  4
#define CLIENT_FREQUENCY    5.0
#define CLIENT_LOG_FILE     false
#define CLIENT_SEND_DIFF    true

struct spi_int
{
    /* SPI related */
    struct spi_ioc_oob_setup oob_setup;
    int fd;
    int len;
    char *tx;
    char *rx;
    void *iobuf;

    /* Encoder related */
    double pos;
    double diff;
    int mt_size;
    int st_size;
    double c_enc; // Encoder constant
};

int initialize_spi(int index, struct spi_int *device);

void encoder_read(struct spi_int *device);

#endif