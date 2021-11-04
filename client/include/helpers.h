#ifndef HEADER_HELPERS
#define HEADER_HELPERS

#include <time.h>
#include "main.h"

double ts2ms(struct timespec *__restrict ts);

void timespec_add_ns(struct timespec *__restrict r, const struct timespec *__restrict t,
                     long ns);

int poll_readers(struct evl_sem * sem);

#endif