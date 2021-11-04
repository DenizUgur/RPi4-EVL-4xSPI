#include "main.h"

// static double ts2ms(struct timespec *__restrict ts)
// {
//     double ret = 0;

//     ret += ts->tv_nsec / 1.0e6;
//     ret += ts->tv_sec * 1.0e3;

//     return ret;
// }

static void timespec_add_ns(struct timespec *__restrict r, const struct timespec *__restrict t,
                            long ns)
{
    long s, rem;

    s = ns / 1000000000;
    rem = ns - s * 1000000000;
    r->tv_sec = t->tv_sec + s;
    r->tv_nsec = t->tv_nsec + rem;
    if (r->tv_nsec >= 1000000000)
    {
        r->tv_sec++;
        r->tv_nsec -= 1000000000;
    }
}

static int initialize_spi(int index, struct spi_int *device)
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

static void encoder_read(struct spi_int *device)
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

    uint8_t mt_size_byte = ceil(device->mt_size / 8);
    uint8_t st_size_byte = ceil(device->st_size / 8);

    for (index = 0; index < mt_size_byte; index++)
    {
        mt <<= 8;
        mt |= (uint8_t)device->rx[index];
    }
    mt >>= (device->mt_size % 8) == 0 ? 0 : 8 - (device->mt_size % 8);

    for (index = mt_size_byte; index < mt_size_byte + st_size_byte; index++)
    {
        st <<= 8;
        st |= (uint8_t)device->rx[index];
    }
    st >>= (device->st_size % 8) == 0 ? 0 : 8 - (device->st_size % 8);

    device->pos = (st * device->c_enc) + (mt * 2 * 3.141592654);
}

int main(int argc, char *argv[])
{
    struct spi_int *spi_devices;
    pid_t childs[CLIENT_SPI_DEV_NUM];

    { /* SPI devices memory initialization  */
        printf(ANSI_COLOR_YELLOW "Creating shared memory for SPI devices... " ANSI_COLOR_RESET);

        int memfd, proxyfd, ret;
        int size = sizeof(struct spi_int) * CLIENT_SPI_DEV_NUM;

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

            efd = evl_attach_self("oob-spi:%d", getpid());
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
            int size = sizeof(struct spi_int) * CLIENT_SPI_DEV_NUM;

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

            /* Set encoder specific attributes */
            device->mt_size = 16;
            device->st_size = 23;
            device->c_enc = 0.000000749;

            if (argc > 3 && sid == atoi(argv[3]))
            {
                device->mt_size = 0;
                device->st_size = 21;
            }

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
    efd = evl_attach_self("oob-spi:%d", getpid());
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

    // int proxyfd, debugfd;
    // debugfd = open("time.log", O_WRONLY | O_CREAT | O_TRUNC, 0600);
    // proxyfd = evl_new_proxy(debugfd, 1024 * 1024, "log:%d", getpid());

    // double start_time = ts2ms(&now);
    // double prev_time = ts2ms(&now);

    for (;;)
    {
        int waiters = 0;
        for (int sid = 0; sid < CLIENT_SPI_DEV_NUM; sid++)
        {
            int r_val;
            evl_peek_sem(&sems[sid], &r_val);
            waiters += r_val;
        }

        if (waiters != -CLIENT_SPI_DEV_NUM)
        {
            nanosleep((const struct timespec[]){{0, 10000L}}, NULL);
            continue;
        }

        char packet[100];
        sprintf(packet, "<%.8f %.8f %.8f %.8f>",
                spi_devices[0].pos,
                spi_devices[1].pos,
                spi_devices[2].pos,
                spi_devices[3].pos);

        for (int sid = 0; sid < CLIENT_SPI_DEV_NUM; sid++)
            evl_put_sem(&sems[sid]);

        ret = send(sockfd, &packet, sizeof(packet), 0);
        if (ret != sizeof(packet))
            error(1, errno, "send() start");

        /* Wait for the next tick to be notified. */
        ret = oob_read(tmfd, &ticks, sizeof(ticks));
        if (ret < 0)
            error(1, errno, "oob_read()");

        ret = evl_read_clock(EVL_CLOCK_MONOTONIC, &now);
        if (ret < 0)
            error(1, errno, "evl_read_clock()");

        // double now_ms = ts2ms(&now);
        // double diff = now_ms - prev_time;
        // prev_time = now_ms;

        // if (now_ms >= (start_time + (10.0 * 1000)))
        //     break;

        // evl_print_proxy(proxyfd, "%.8f %d\n", diff, ticks - 1);
    }

    for (int sid = 0; sid < CLIENT_SPI_DEV_NUM; sid++)
        if (kill(childs[sid], SIGTERM) == -1 && errno != ESRCH)
            exit(EXIT_FAILURE);

    while (wait(NULL) != -1 || errno == EINTR)
        ;

    // for (int spi_i = 0; spi_i < spi_count; spi_i++)
    // {
    //     struct spi_int *current_device = &spi_devices[spi_i];
    //     munmap(current_device->iobuf, current_device->oob_setup.iobuf_len);

    //     ret = ioctl(current_device->fd, SPI_IOC_DISABLE_OOB_MODE);
    //     if (ret)
    //         error(1, errno, "ioctl(SPI_IOC_DISABLE_OOB_MODE)");
    //     close(current_device->fd);
    // }

    return 0;
}
