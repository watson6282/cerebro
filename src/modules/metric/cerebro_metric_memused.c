/*****************************************************************************\
 *  $Id: cerebro_metric_memused.c,v 1.2 2006-08-27 05:23:53 chu11 Exp $
 *****************************************************************************
 *  Copyright (C) 2005 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Albert Chu <chu11@llnl.gov>.
 *  UCRL-CODE-155989 All rights reserved.
 *
 *  This file is part of Cerebro, a collection of cluster monitoring
 *  tools and libraries.  For details, see
 *  <http://www.llnl.gov/linux/cerebro/>.
 *
 *  Cerebro is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  Cerebro is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Genders; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#include <errno.h>

#include "cerebro.h"
#include "cerebro/cerebro_metric_module.h"

#include "cerebro_metric_memory.h"
#include "debug.h"

#define MEMUSED_METRIC_MODULE_NAME  "memused"
#define MEMUSED_METRIC_NAME         "memused"

/*
 * memused_metric_setup
 *
 * memused metric module setup function.  Read and store the memused
 * out of /proc.
 */
static int
memused_metric_setup(void)
{
  /* nothing to do */
  return 0;
}

/*
 * memused_metric_cleanup
 *
 * memused metric module cleanup function
 */
static int
memused_metric_cleanup(void)
{
  /* nothing to do */
  return 0;
}

/*
 * memused_metric_get_metric_name
 *
 * memused metric module get_metric_name function
 */
static char *
memused_metric_get_metric_name(void)
{
  return MEMUSED_METRIC_NAME;
}

/*
 * memused_metric_get_metric_period
 *
 * memused metric module get_metric_period function
 */
static int
memused_metric_get_metric_period(int *period)
{
  if (!period)
    {
      CEREBRO_DBG(("invalid parameters"));
      return -1;
    }
  
  *period = 60;
  return 0;
}

/*
 * memused_metric_get_metric_value
 *
 * memused metric module get_metric_value function
 */
static int
memused_metric_get_metric_value(unsigned int *metric_value_type,
                                 unsigned int *metric_value_len,
                                 void **metric_value)
{
  u_int32_t memtotalval, memfreeval;
  u_int32_t *memusedptr = NULL;
  int rv = -1;

  if (!metric_value_type || !metric_value_len || !metric_value)
    {
      CEREBRO_DBG(("invalid parameters"));
      return -1;
    }
  
  if (cerebro_metric_get_memory(&memtotalval,
				&memfreeval,
				NULL,
				NULL) < 0)
    goto cleanup;
  
  if (!(memusedptr = (u_int32_t *)malloc(sizeof(u_int32_t))))
    {
      CEREBRO_DBG(("malloc: %s", strerror(errno)));
      goto cleanup;
    }
  
  *memusedptr = memtotalval - memfreeval;

  *metric_value_type = CEREBRO_METRIC_VALUE_TYPE_U_INT32;
  *metric_value_len = sizeof(u_int32_t);
  *metric_value = (void *)memusedptr;

  rv = 0;
 cleanup:
  if (rv < 0 && memusedptr)
    free(memusedptr);
  return rv;
}

/*
 * memused_metric_destroy_metric_value
 *
 * memused metric module destroy_metric_value function
 */
static int
memused_metric_destroy_metric_value(void *metric_value)
{
  if (!metric_value)
    {
      CEREBRO_DBG(("invalid parameters"));
      return -1;
    }

  free(metric_value);
  return 0;
}

/*
 * memused_metric_get_metric_thread
 *
 * memused metric module get_metric_thread function
 */
static Cerebro_metric_thread_pointer
memused_metric_get_metric_thread(void)
{
  return NULL;
}

/*
 * memused_metric_send_heartbeat_function_pointer
 *
 * memused metric module send_heartbeat_function_pointer function
 */
static int
memused_metric_send_heartbeat_function_pointer(Cerebro_metric_send_heartbeat function_pointer)
{
  return 0;
}

#if WITH_STATIC_MODULES
struct cerebro_metric_module_info memused_metric_module_info =
#else  /* !WITH_STATIC_MODULES */
struct cerebro_metric_module_info metric_module_info =
#endif /* !WITH_STATIC_MODULES */
  {
    MEMUSED_METRIC_MODULE_NAME,
    &memused_metric_setup,
    &memused_metric_cleanup,
    &memused_metric_get_metric_name,
    &memused_metric_get_metric_period,
    &memused_metric_get_metric_value,
    &memused_metric_destroy_metric_value,
    &memused_metric_get_metric_thread,
    &memused_metric_send_heartbeat_function_pointer,
  };