#include "main.h"

int initialize_spi(int index, struct spi_int *device)
{
    // SPI configuration
    uint8_t spi_count = 4;
    static uint32_t mode = SPI_MODE_3;
    static uint8_t bits = 8;
    static uint32_t speed = 1250000;
    static int len = 8;

    int ret;
    char device_name[20];
    sprintf(device_name, "/dev/spidev%d.0", index);

    device->fd = open(device_name, O_RDWR);
    if (device->fd < 0)
        error(1, errno, "can't open device %s", device);

    ret = ioctl(device->fd, SPI_IOC_WR_MODE32, &mode);
    if (ret)
        error(1, errno, "ioctl(SPI_IOC_WR_MODE32)");

    ret = ioctl(device->fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret)
        error(1, errno, "ioctl(SPI_IOC_WR_BITS_PER_WORD)");

    ret = ioctl(device->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret)
        error(1, errno, "ioctl(SPI_IOC_WR_MAX_SPEED_HZ)");

    /* Switch to Out-of-Band operation */
    device->oob_setup.frame_len = len;
    device->oob_setup.speed_hz = speed;
    device->oob_setup.bits_per_word = bits;
    ret = ioctl(device->fd,
                SPI_IOC_ENABLE_OOB_MODE, &device->oob_setup);
    if (ret)
        error(1, errno, "ioctl(SPI_IOC_ENABLE_OOB_MODE)");

    /* Map the I/O area */
    device->iobuf = mmap(NULL,
                         device->oob_setup.iobuf_len,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED, device->fd, 0);
    if (device->iobuf == MAP_FAILED)
        error(1, errno, "mmap()");

    /* Define TX/RX from I/O Buffer */
    device->tx = device->iobuf + device->oob_setup.tx_offset;
    memset(device->tx, 0, len);

    device->rx = device->iobuf + device->oob_setup.rx_offset;
    memset(device->rx, 0, len);

    /* Extra information */
    device->len = len;

    return 0;
}

void encoder_read(struct spi_int *device)
{
    int ret;
    memset(device->rx, 0, device->len);

    ret = oob_ioctl(device->fd, SPI_IOC_RUN_OOB_XFER);
    if (ret)
        error(1, errno, "oob_ioctl(SPI_IOC_RUN_OOB_XFER)");

    /*Convert RX buffer to position data */
    uint8_t index;
    uint64_t mt = 0x00;
    uint64_t st = 0x00;

    for (index = 0; index < 2; index++)
    {
        mt <<= 8;
        mt |= (uint8_t)device->rx[index];
    }

    for (index = 2; index < 5; index++)
    {
        st <<= 8;
        st |= (uint8_t)device->rx[index];
    }
    st >>= 1;

    double read = (st * 0.000000749) + (mt * 2 * 3.141592654);

    device->diff = read - device->pos;
    device->pos = read;
}

int main(int argc, char *argv[])
{
    struct spi_int *spi_devices;
    pid_t childs[CLIENT_SPI_DEV_NUM];
    int size = sizeof(struct spi_int) * CLIENT_SPI_DEV_NUM;

    { /* SPI devices memory initialization  */
        printf(ANSI_COLOR_YELLOW "Creating shared memory for SPI devices... " ANSI_COLOR_RESET);

        int memfd, proxyfd, ret;
        memfd = memfd_create("spi_devices", 0);

        ret = ftruncate(memfd, size);
        spi_devices = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
        if (spi_devices == MAP_FAILED)
            error(1, ret, "mmap()");

        proxyfd = evl_new_proxy(memfd, 0, "/spi-devices");
        if (!proxyfd)
            error(1, ret, "evl_new_proxy()");

        printf(ANSI_COLOR_GREEN "SUCCESS\n" ANSI_COLOR_RESET);
    }

    { /* Setup processes */
        int ret, efd;
        pid_t pid;

        for (int sid = 0; sid < CLIENT_SPI_DEV_NUM; sid++)
        {
            if ((pid = fork()) != 0)
            {
                childs[sid] = pid;
                continue;
            }

            printf(ANSI_COLOR_YELLOW "Configuring process #%d\n" ANSI_COLOR_RESET, sid);

            /* Attach to EVL core */
            struct sched_param param;

            param.sched_priority = 1;
            ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
            if (ret)
                error(1, ret, "pthread_setschedparam()");

            efd = evl_attach_self("rt-encoder-task:%d", getpid());
            if (efd < 0)
                error(1, -efd, "cannot attach to the EVL core");

            /* Move to a free CPU */
            cpu_set_t mask;
            CPU_ZERO(&mask);
            CPU_SET(getpid(), &mask);
            CPU_SET(sid % get_nprocs(), &mask);

            ret = sched_setaffinity(0, sizeof(mask), &mask);
            if (ret)
                error(1, ret, "sched_setaffinity()");

            /* Setup SPI */
            struct spi_int *ptr;
            int fd;

            fd = open("/dev/evl/proxy/spi-devices", O_RDWR);
            ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

            struct spi_int *device = &ptr[sid];
            ret = initialize_spi(sid, device);
            if (ret)
                error(1, ret, "initialize_spi()");

            /* Initialize semaphore */
            int semfd;
            static struct evl_sem sem;
            semfd = evl_create_sem(&sem, EVL_CLOCK_MONOTONIC, 0, EVL_CLONE_PUBLIC, "/sem-%d", sid);
            if (!semfd)
                error(1, ret, "evl_create_sem()");

            printf(ANSI_COLOR_GREEN "Configured process #%d\n" ANSI_COLOR_RESET, sid);

            for (;;)
            {
                evl_get_sem(&sem);
                encoder_read(&spi_devices[sid]);
            }

            exit(0);
        }
    }

    /* Setup parent EVL process */
    int ret, efd;
    struct sched_param param;

    param.sched_priority = 1;
    ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (ret)
        error(1, ret, "pthread_setschedparam()");

    /* Let's attach to the EVL core. */
    efd = evl_attach_self("rt-encoder-task:%d", getpid());
    if (efd < 0)
        error(1, -efd, "cannot attach to the EVL core");

    /* Setup network */
    printf(ANSI_COLOR_YELLOW "Setting up network... " ANSI_COLOR_RESET);

    if (argc < 3)
    {
        printf(ANSI_COLOR_RED "FAILED: Insufficient arguments\n" ANSI_COLOR_RESET);
        abort();
    }

    int sockfd;
    struct sockaddr_in in_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error(1, errno, "socket()");

    int sock_flag = 1;
    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&sock_flag, sizeof(int));
    if (ret < 0)
        error(1, errno, "setsockopt()");

    memset(&in_addr, 0, sizeof(in_addr));
    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = inet_addr(argv[1]);
    in_addr.sin_port = htons(atoi(argv[2]));

    printf("Waiting for connection on %s:%s\n", argv[1], argv[2]);
    while (0 > (ret = connect(sockfd, (struct sockaddr *)&in_addr, sizeof(in_addr))))
        ;
    printf("Got connection on %s:%s\n", argv[1], argv[2]);

    /* Set up timer */
    int tmfd;
    __u64 ticks;
    struct timespec now;
    struct itimerspec value, ovalue;

    /*Create a timer on the built-in monotonic clock. */
    tmfd = evl_new_timer(EVL_CLOCK_MONOTONIC);
    if (!tmfd)
        error(1, ret, "evl_new_timer()");

    /*Set up periodic timer. */
    ret = evl_read_clock(EVL_CLOCK_MONOTONIC, &now);
    if (ret)
        error(1, ret, "evl_read_clock()");

    /*EVL always uses absolute timeouts, add 1s to the current date */
    timespec_add_ns(&value.it_value, &now, 1000000000ULL);
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_nsec = lround((1 / CLIENT_FREQUENCY) * 1.0e6);
    ret = evl_set_timer(tmfd, &value, &ovalue);
    if (ret)
        error(1, ret, "evl_set_timer()");

    /* Get the semaphores */
    struct evl_sem sems[CLIENT_SPI_DEV_NUM];
    for (int sid = 0; sid < CLIENT_SPI_DEV_NUM; sid++)
    {
        char sem_name[22];
        struct stat buffer;

        sprintf(sem_name, "/dev/evl/monitor/sem-%d", sid);
        while (stat(sem_name, &buffer))
            sleep(1);

        ret = evl_open_sem(&sems[sid], "/sem-%d", sid);
        if (ret < 0)
            error(1, ret, "evl_open_sem()");
    }

#if CLIENT_LOG_FILE
    char log_name[100];
    sprintf(log_name, "/var/log/rt-encoder-task-%u.log", (unsigned)time(NULL));

    int proxyfd, debugfd;
    debugfd = open(log_name, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    proxyfd = evl_new_proxy(debugfd, 1024 * 1024, "log:%d", getpid());
#endif

#if CLIENT_SEND_DIFF
    double prev_time = ts2ms(&now);
#endif

    for (;;)
    {
        if (poll_readers(sems) != -CLIENT_SPI_DEV_NUM)
        {
            nanosleep((const struct timespec[]){{0, 10000L}}, NULL);
            continue;
        }

#if CLIENT_SEND_DIFF || CLIENT_LOG_FILE
        double now_ms = ts2ms(&now);
        double rate = (now_ms - prev_time) / 1e4;
        prev_time = now_ms;
#endif

#if CLIENT_SEND_DIFF
        char packet[150];
        sprintf(packet, "<%.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f>",
                spi_devices[0].pos,
                spi_devices[0].diff / rate,
                spi_devices[1].pos,
                spi_devices[1].diff / rate,
                spi_devices[2].pos,
                spi_devices[2].diff / rate,
                spi_devices[3].pos,
                spi_devices[3].diff / rate);

#if CLIENT_LOG_FILE
        evl_print_proxy(proxyfd, "%.8f %d %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f\n",
                        rate,
                        ticks - 1,
                        spi_devices[0].pos,
                        spi_devices[0].diff / rate,
                        spi_devices[1].pos,
                        spi_devices[1].diff / rate,
                        spi_devices[2].pos,
                        spi_devices[2].diff / rate,
                        spi_devices[3].pos,
                        spi_devices[3].diff / rate);
#endif
#else
        char packet[100];
        sprintf(packet, "<%.8f %.8f %.8f %.8f>",
                spi_devices[0].pos,
                spi_devices[1].pos,
                spi_devices[2].pos,
                spi_devices[3].pos);

#if CLIENT_LOG_FILE
        evl_print_proxy(proxyfd, "%.8f %d %.8f %.8f %.8f %.8f\n",
                        rate,
                        ticks - 1,
                        spi_devices[0].pos,
                        spi_devices[1].pos,
                        spi_devices[2].pos,
                        spi_devices[3].pos);
#endif
#endif

        for (int sid = 0; sid < CLIENT_SPI_DEV_NUM; sid++)
            evl_put_sem(&sems[sid]);

        ret = send(sockfd, &packet, sizeof(packet), 0);
        if (ret != sizeof(packet))
            break;

        /* Wait for the next tick to be notified. */
        ret = oob_read(tmfd, &ticks, sizeof(ticks));
        if (ret < 0)
            error(1, errno, "oob_read()");

        ret = evl_read_clock(EVL_CLOCK_MONOTONIC, &now);
        if (ret < 0)
            error(1, errno, "evl_read_clock()");
    }

    // Wait children to stop using SPI
    for (;;)
    {
        if (poll_readers(sems) != -CLIENT_SPI_DEV_NUM)
            nanosleep((const struct timespec[]){{0, 10000L}}, NULL);
        else
            break;
    }

    // Kill child processes
    for (int sid = 0; sid < CLIENT_SPI_DEV_NUM; sid++)
    {
        struct spi_int *current_device = &spi_devices[sid];
        munmap(current_device->iobuf, current_device->oob_setup.iobuf_len);
        close(current_device->fd);

        ret = evl_close_sem(&sems[sid]);
        if (ret < 0)
            error(1, ret, "evl_close_sem()");

        if (kill(childs[sid], SIGTERM) == -1 && errno != ESRCH)
            exit(EXIT_FAILURE);
    }

    // Release memory
    munmap(spi_devices, size);

#if CLIENT_LOG_FILE
    close(debugfd);
#endif

    while (wait(NULL) != -1 || errno == EINTR)
        ;

    return 0;
}
