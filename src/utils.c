#include "utils.h"

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#else
#include <time.h>
#endif

/* OS X does not have clock_gettime, use clock_get_time*/
int posix_clock_gettime(clockid_t clk_id, struct timespec *ts)
{
#ifdef __MACH__
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	int ret = clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	ts->tv_sec = mts.tv_sec;
	ts->tv_nsec = mts.tv_nsec;
	return ret;
#else
	return clock_gettime(clk_id, ts);
#endif
}