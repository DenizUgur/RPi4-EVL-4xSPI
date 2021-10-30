#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>

#include <evl/thread.h>
#include <evl/clock.h>
#include <evl/timer.h>
#include <evl/proxy.h>
#include <linux/spi/spidev.h>
#include <uapi/evl/devices/spidev.h>

#include <inttypes.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

struct spi_int
{
	/*SPI related */
	struct spi_ioc_oob_setup oob_setup;
	int fd;
	int len;
	char *tx;
	char *rx;
	void *iobuf;

	/*Encoder related */
	double pos;
	int mt_size;
	int st_size;
	double c_enc; // Encoder constant
};

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

static int check_this_fd(int fd)
{
	if (!fd)
	{
		exit(19);
	}
	return fd;
}

static int check_this_status(int status)
{
	if (0 != status)
	{
		error(1, status, "() failed");
		exit(status);
	}
	return status;
}

static long _lround(float num)
{
	return num < 0 ? num - 0.5 : num + 0.5;
}

static uint8_t _dceil(int a, int b)
{
	return (a / b) + ((a % b) != 0);
}

int main(int argc, char *argv[])
{
	int ret, tfd;
	struct sched_param param;

	uint8_t spi_count = 4;
	static uint32_t mode = SPI_MODE_3;
	static uint8_t bits = 8;
	static uint32_t speed = 1250000;
	static int len = 8;

	double frequency = 3.0;
	long period = _lround((1 / frequency) * 1.0e6);

	/*Initialize SPI devices */
	struct spi_int spi_devices[spi_count];

	for (int spi_i = 0; spi_i < spi_count; spi_i++)
	{
		/*Perform initialization operations on current SPI device */
		struct spi_int *current_device = &spi_devices[spi_i];

		char device[20];
		sprintf(device, "/dev/spidev%d.0", spi_i);

		/*
		 *This is usual SPI configuration stuff using the spidev
		 *interface.
		 */
		current_device->fd = open(device, O_RDWR);
		if (current_device->fd < 0)
			error(1, errno, "can't open device %s", device);

		ret = ioctl(current_device->fd, SPI_IOC_WR_MODE32, &mode);
		if (ret)
			error(1, errno, "ioctl(SPI_IOC_WR_MODE32)");

		ret = ioctl(current_device->fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
		if (ret)
			error(1, errno, "ioctl(SPI_IOC_WR_BITS_PER_WORD)");

		ret = ioctl(current_device->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
		if (ret)
			error(1, errno, "ioctl(SPI_IOC_WR_MAX_SPEED_HZ)");

		/*
		 *This part switches the device to out-of-band operation
		 *mode. In this case, I/O is performed via the DMA engine
		 *exclusively, directly from/to buffers (m)mapped into the
		 *address space of the client.
		 */
		current_device->oob_setup.frame_len = len;
		current_device->oob_setup.speed_hz = speed;
		current_device->oob_setup.bits_per_word = bits;
		ret = ioctl(current_device->fd,
					SPI_IOC_ENABLE_OOB_MODE, &current_device->oob_setup);
		if (ret)
			error(1, errno, "ioctl(SPI_IOC_ENABLE_OOB_MODE)");

		printf("mapping %d bytes, tx@%d, rx@%d\n",
			   current_device->oob_setup.iobuf_len,
			   current_device->oob_setup.tx_offset,
			   current_device->oob_setup.rx_offset);

		/*
		 *We may map the I/O area now, it is composed of two adjacent
		 *buffers of @len bytes (plus alignment). CAUTION: the
		 *mapping is always on coherent DMA memory (i.e. non-cached).
		 */
		current_device->iobuf = mmap(NULL,
									 current_device->oob_setup.iobuf_len,
									 PROT_READ | PROT_WRITE,
									 MAP_SHARED, current_device->fd, 0);
		if (current_device->iobuf == MAP_FAILED)
			error(1, errno, "mmap()");

		/*
		 *The core told us where to read and write data on return to
		 *ioctl(SPI_IOC_ENABLE_OOB_MODE), which is at some offset
		 *from the I/O buffer we received from mmap().
		 */
		current_device->tx = current_device->iobuf + current_device->oob_setup.tx_offset;
		memset(current_device->tx, 0, len);
		current_device->rx = current_device->iobuf + current_device->oob_setup.rx_offset;
		memset(current_device->rx, 0, len);
		current_device->len = len;

		/*Decide whether an encoder has a different data format */
		current_device->mt_size = 16;
		current_device->st_size = 23;
		current_device->c_enc = 0.000000749;

		if (argc > 3 && spi_i == atoi(argv[3]))
		{
			current_device->mt_size = 0;
			current_device->st_size = 21;
		}
	}

	/*Setup OOB process */
	param.sched_priority = 1;
	ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
	if (ret)
		error(1, ret, "pthread_setschedparam()");

	/*Let's attach to the EVL core. */
	tfd = evl_attach_self("oob-spi:%d", getpid());
	if (tfd < 0)
		error(1, -tfd, "cannot attach to the EVL core");

	/*Set up TCP */
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

	evl_printf("Waiting for connection on %s:%s\n", argv[1], argv[2]);
	while (0 > (ret = connect(sockfd, (struct sockaddr *)&in_addr, sizeof(in_addr))));
	evl_printf("Got connection on %s:%s\n", argv[1], argv[2]);

	/*Set up timer */
	int tmfd;
	__u64 ticks;
	struct timespec now;
	struct itimerspec value, ovalue;

	/*Create a timer on the built-in monotonic clock. */
	tmfd = evl_new_timer(EVL_CLOCK_MONOTONIC);
	check_this_fd(tmfd);

	/*Set up periodic timer. */
	ret = evl_read_clock(EVL_CLOCK_MONOTONIC, &now);
	check_this_status(ret);

	/*EVL always uses absolute timeouts, add 1s to the current date */
	timespec_add_ns(&value.it_value, &now, 1000000000 ULL);
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_nsec = period;
	ret = evl_set_timer(tmfd, &value, &ovalue);
	check_this_status(ret);

	for (;;)
	{
		/*Wait for the next tick to be notified. */
		ret = oob_read(tmfd, &ticks, sizeof(ticks));

		if (ret < 0)
			exit(2);

		if (ticks > 1)
		{
			fprintf(stderr, "timer overrun! %lld ticks late\n",
					ticks - 1);
		}

		/*Read current time */
		ret = evl_read_clock(EVL_CLOCK_MONOTONIC, &now);
		check_this_status(ret);

		/*Do the transfer for all SPI devices */
		for (int spi_i = 0; spi_i < spi_count; spi_i++)
		{ /*Perform read operation on current SPI device */
			struct spi_int *current_device = &spi_devices[spi_i];
			memset(current_device->rx, 0, current_device->len);

			ret = oob_ioctl(current_device->fd, SPI_IOC_RUN_OOB_XFER);
			if (ret)
				error(1, errno, "oob_ioctl(SPI_IOC_RUN_OOB_XFER)");

			/*Convert RX buffer to position data */
			uint8_t index;
			uint64_t mt = 0x00;
			uint64_t st = 0x00;

			uint8_t mt_size_byte = _dceil(current_device->mt_size, 8);
			uint8_t st_size_byte = _dceil(current_device->st_size, 8);

			for (index = 0; index < mt_size_byte; index++)
			{
				mt <<= 8;
				mt |= (uint8_t)current_device->rx[index];
			}
			mt >>= (current_device->mt_size % 8) == 0 ? 0 : 8 - (current_device->mt_size % 8);

			for (index = mt_size_byte; index < mt_size_byte + st_size_byte; index++)
			{
				st <<= 8;
				st |= (uint8_t)current_device->rx[index];
			}
			st >>= (current_device->st_size % 8) == 0 ? 0 : 8 - (current_device->st_size % 8);

			current_device->pos = (st * current_device->c_enc) + (mt * 2 * 3.141592654);
		}

		/*Do the TCP transfer */
		char sample[100];
		sprintf(sample, "<%.8f %.8f %.8f %.8f>",
				spi_devices[0].pos,
				spi_devices[1].pos,
				spi_devices[2].pos,
				spi_devices[3].pos);

		ret = send(sockfd, &sample, sizeof(sample), 0);
		if (ret != sizeof(sample))
			error(1, errno, "send() start");
	}

	/*Disable the timer (not required if closing). */
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_nsec = 0;
	ret = evl_set_timer(tmfd, &value, NULL);
	check_this_status(ret);

	evl_printf("\nDone!\n");

	/*All done, wrap it up. */
	for (int spi_i = 0; spi_i < spi_count; spi_i++)
	{
		/*Perform close operations on current SPI device */
		struct spi_int *current_device = &spi_devices[spi_i];
		munmap(current_device->iobuf, current_device->oob_setup.iobuf_len);

		ret = ioctl(current_device->fd, SPI_IOC_DISABLE_OOB_MODE);
		if (ret)
			error(1, errno, "ioctl(SPI_IOC_DISABLE_OOB_MODE)");
		close(current_device->fd);
	}

	return 0;
}