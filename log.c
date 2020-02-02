#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "loglib_api.h"
#include "log.h"

//#define MAX_LOGFILE_SIZE 40*1024*1024
static struct {
  void *udata;
  log_LockFn lock;
  int pzlog;
  int level;
  int quiet;
} L;

static const char *level_names[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif

static void lock(void)   {
  if (L.lock) {
    L.lock(L.udata, 1);
  }
}

static void unlock(void) {
  if (L.lock) {
    L.lock(L.udata, 0);
  }
}

void log_set_udata(void *udata) {
  L.udata = udata;
}

void log_set_lock(log_LockFn fn) {
  L.lock = fn;
}

void log_set_level(int level) {
  L.level = level;
}

void log_set_quiet(int enable) {
  L.quiet = enable ? 1 : 0;
}

void log_init(void)
{
  printf("Log init function\n");
  int ret = init_log_msg_queue(2000);
  if (ret != 0)
  {
    printf("Message queue cannot be created\n");
    L.pzlog = 0;
  }
  else
  {
    log_set_level(1);
    L.pzlog = 1;
    printf("Message queue has been created\n");
  }
}

void log_deinit(void)
{
  deinit_msg_queue();
}

void log_log(int level, const char *file, int line, const char *fmt, ...) {
  if (level < L.level) {
    return;
  }

  /* Acquire lock */
  lock();

  /* Get current time */
  time_t t = time(NULL);
  struct tm *lt = localtime(&t);
//  int ret = -1;
  
  /* Log to stderr */
  if (!L.quiet) {
    va_list args;
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';
#ifdef LOG_USE_COLOR
    fprintf(
      stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
      buf, level_colors[level], level_names[level], file, line);
#else
    fprintf(stderr, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
#endif
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
  }

  /* Log to file */
  if (L.pzlog) {
    va_list args;
    char buf[150] = {'\0'};
    char prereq[40] = {'\0'};
    char format[60] = {'\0'};
    prereq[strftime(prereq, sizeof(prereq), "%Y-%m-%d %H:%M:%S", lt)] = '\0';

    va_start(args, fmt);
    vsprintf(format, fmt, args);
    va_end(args);
    snprintf(buf, 150,"%s %-5s %s:%d: %s ", prereq, level_names[level], file, line, format);
    log_msg(buf);
  }



  /* Release lock */
  unlock();
}
