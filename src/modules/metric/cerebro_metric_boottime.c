/*****************************************************************************\
 *  $Id: cerebro_metric_boottime.c,v 1.3 2005-06-15 22:46:08 achu Exp $
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#include <ctype.h>
#endif /* STDC_HEADERS */
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include "cerebro.h"
#include "cerebro/cerebro_constants.h"
#include "cerebro/cerebro_error.h"
#include "cerebro/cerebro_metric_module.h"

#define BOOTTIME_FILE                "/proc/stat"
#define BOOTTIME_BUFLEN              4096
#define BOOTTIME_KEYWORD             "btime"
#define BOOTTIME_METRIC_MODULE_NAME  "boottime"
#define BOOTTIME_METRIC_NAME         "boottime"

/*
 * metric_boottime
 *
 * cached system boottime
 *
 * On some systems, due to kernel bugs, the boottime value may change
 * as the system executes.  We will read the boottime value from
 * /proc, cache it, and assume that it is always correct.
 */
static u_int32_t metric_boottime = 0;

/*
 * boottime_metric_setup
 *
 * boottime metric module setup function
 */
static int
boottime_metric_setup(void)
{
  int fd, len;
  char *bootvalptr, *endptr, *tempptr;
  char buf[BOOTTIME_BUFLEN];
  long int bootval;
 
  if ((fd = open(BOOTTIME_FILE, O_RDONLY, 0)) < 0)
    {
      cerebro_err_debug_module("%s(%s:%d): open: %s",
                               __FILE__, __FUNCTION__, __LINE__,
                               strerror(errno));
      goto cleanup;
    }

  if ((len = read(fd, buf, BOOTTIME_BUFLEN)) < 0)
    {
      cerebro_err_debug_module("%s(%s:%d): read: %s",
                               __FILE__, __FUNCTION__, __LINE__,
                               strerror(errno));
      goto cleanup;
    }
                                                                                      
  bootvalptr = strstr(buf, BOOTTIME_KEYWORD);
  bootvalptr += strlen(BOOTTIME_KEYWORD);
  if (bootvalptr < (buf + BOOTTIME_BUFLEN))
    {
      while(isspace(*bootvalptr))
        bootvalptr++;
                                                                                      
      tempptr = bootvalptr;
                                                                                      
      while(!isspace(*tempptr) && *tempptr != '\0')
        tempptr++;
      *tempptr = '\0';
    }
  else
    {
      cerebro_err_debug_module("%s(%s:%d): boottime file parse error",
                               __FILE__, __FUNCTION__, __LINE__);
      goto cleanup;
    }
                                                                                      
  errno = 0;
  bootval = (u_int32_t)strtol(bootvalptr, &endptr, 10);
  if ((bootval == LONG_MIN || bootval == LONG_MAX) && errno == ERANGE)
    {
      cerebro_err_debug_module("%s(%s:%d): boottime out of range",
                               __FILE__, __FUNCTION__, __LINE__);
      goto cleanup;
    }
  if ((bootvalptr + strlen(bootvalptr)) != endptr)
    {
      cerebro_err_debug_module("%s(%s:%d): boottime value parse error",
                               __FILE__, __FUNCTION__, __LINE__);
      goto cleanup;
    }
                                                                                      
  metric_boottime = (u_int32_t)bootval;
  return 0;

 cleanup:
  close(fd);
  return -1;
}

/*
 * boottime_metric_cleanup
 *
 * boottime metric module cleanup function
 */
static int
boottime_metric_cleanup(void)
{
  /* nothing to do */
  return 0;
}

/* 
 * boottime_metric_get_metric_name
 *
 * boottime metric module get_metric_name function
 *
 * Returns string on success, -1 on error
 */
static char *
boottime_metric_get_metric_name(void)
{
  return BOOTTIME_METRIC_NAME;
}

/* 
 * boottime_metric_get_metric_value_type
 *
 * boottime metric module get_metric_value_type function
 *
 * Returns value_type on success, -1 on error
 */
static int
boottime_metric_get_metric_value_type(void)
{
  return CEREBRO_METRIC_VALUE_TYPE_U_INT32;
}

/* 
 * boottime_metric_get_metric_value_len
 *
 * boottime metric module get_metric_value_len function
 *
 * Returns value_len on success, -1 on error
 */
static int
boottime_metric_get_metric_value_len(void)
{
  return sizeof(u_int32_t);
}

/* 
 * boottime_metric_get_metric_value
 *
 * boottime metric module get_metric_value function
 *
 * Returns length of data copied on success, -1 on error
 */
static int
boottime_metric_get_metric_value(void *metric_value_buf,
                                  unsigned int metric_value_buflen)
{
  if (!metric_value_buf)
    {
      cerebro_err_debug_module("%s(%s:%d): metric_value_buf null",
                               __FILE__, __FUNCTION__, __LINE__);
      return -1;
    }

  if (metric_value_buflen < sizeof(u_int32_t))
    {
      cerebro_err_debug_module("%s(%s:%d): metric_value_buflen invalid",
                               __FILE__, __FUNCTION__, __LINE__);
      return -1;
    }

  memcpy(metric_value_buf, &metric_boottime, sizeof(u_int32_t));
  return sizeof(u_int32_t);
}

#if WITH_STATIC_MODULES
struct cerebro_metric_module_info boottime_metric_module_info =
#else /* !WITH_STATIC_MODULES */
struct cerebro_metric_module_info metric_module_info =
#endif /* !WITH_STATIC_MODULES */
  {
    BOOTTIME_METRIC_MODULE_NAME,
    &boottime_metric_setup,
    &boottime_metric_cleanup,
    &boottime_metric_get_metric_name,
    &boottime_metric_get_metric_value_type,
    &boottime_metric_get_metric_value_len,
    &boottime_metric_get_metric_value,
  };
