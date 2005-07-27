/*****************************************************************************\
 *  $Id: cerebrod_listener_data.h,v 1.7 2005-07-27 00:29:22 achu Exp $
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

#ifndef _CEREBROD_LISTENER_DATA_H
#define _CEREBROD_LISTENER_DATA_H

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#if HAVE_PTHREAD_H
#include <pthread.h>
#endif /* HAVE_PTHREAD_H */

#include <sys/types.h>

#include "protocol/cerebrod_heartbeat_protocol.h"

#include "hash.h"

/* 
 * Flags to define if a metric is a default metric, or something
 * the listener received.
 */
#define CEREBROD_METRIC_LISTENER_ORIGIN_DEFAULT   0x00000001
#define CEREBROD_METRIC_LISTENER_ORIGIN_MONITORED 0x00000002

/* 
 * struct cerebrod_metric_data
 *
 * Contains metric data for a node
 */
struct cerebrod_metric_data
{
  char *metric_name;
  u_int32_t metric_value_received_time;
  u_int32_t metric_value_type;
  u_int32_t metric_value_len;
  void *metric_value;
};

/*
 * struct cerebrod_node_data
 *
 * contains cerebrod node data
 */
struct cerebrod_node_data
{
  char *nodename;
  int discovered;
  u_int32_t last_received_time;
  pthread_mutex_t node_data_lock;
  hash_t metric_data;
  int metric_data_count;
};

/*
 * struct cerebrod_monitor_module
 *
 * contains cerebrod monitor module metric information
 */
struct cerebrod_monitor_module
{
  char *metric_name;
  int index;
  pthread_mutex_t monitor_lock;
};

/* 
 * struct cerebrod_metric_name_data
 *
 * contains metric name and origin
 */
struct cerebrod_metric_name_data
{
  char *metric_name;
  u_int32_t metric_origin;
};

/* 
 * metric_data_create
 *
 * create metric name data
 *
 * Returns pointer on success, NULL on error
 */
struct cerebrod_metric_data *metric_data_create(const char *metric_name);

/* 
 * metric_data_destroy
 *
 * destroy metric name data
 */
void metric_data_destroy(void *data);

/* 
 * metric_name_data_create
 *
 * create metric name data
 *
 * Returns pointer on success, NULL on error
 */
struct cerebrod_metric_name_data *metric_name_data_create(const char *metric_name,
                                                          u_int32_t metric_origin);

/* 
 * metric_name_data_destroy
 *
 * destroy metric name data
 */
void metric_name_data_destroy(void *data);

/* 
 * cerebrod_listener_data_initialize
 *
 * Initialize listener_data structures
 */
void cerebrod_listener_data_initialize(void);

/* 
 * cerebrod_listener_data_update
 *
 * Update listener_data with information from a heartbeat
 */
void cerebrod_listener_data_update(char *nodename,
                                   struct cerebrod_heartbeat *hb,
                                   u_int32_t received_time);

#endif /* _CEREBROD_LISTENER_DATA_H */
