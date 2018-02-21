#include <time.h>

double wallclock_millis() {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME,&t);
  return 1e3*t.tv_sec+1e-6*t.tv_nsec;
}
