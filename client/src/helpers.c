#include "helpers.h"

double ts2ms(struct timespec *__restrict ts)
{
    double ret = 0;

    ret += ts->tv_nsec / 1.0e6;
    ret += ts->tv_sec * 1.0e3;

    return ret;
}

void timespec_add_ns(struct timespec *__restrict r, const struct timespec *__restrict t,
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

int poll_readers(struct evl_sem * sems)
{
    int ret = 0;
    for (int sid = 0; sid < CLIENT_SPI_DEV_NUM; sid++)
    {
        int r_val;
        evl_peek_sem(&sems[sid], &r_val);
        ret += r_val;
    }

    return ret;
}