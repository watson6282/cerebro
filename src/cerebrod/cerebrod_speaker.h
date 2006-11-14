/*****************************************************************************\
 *  $Id: cerebrod_speaker.h,v 1.8.4.2 2006-11-14 18:09:35 chu11 Exp $
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

#ifndef _CEREBROD_SPEAKER_H
#define _CEREBROD_SPEAKER_H

#include "cerebro/cerebrod_heartbeat_protocol.h"

/* 
 * Flags to define if a metric should be sent with cerebrod heartbeats
 * or by themselves.
 */
#define CEREBROD_SPEAKER_NEXT_SEND_TYPE_HEARTBEAT 0x1
#define CEREBROD_SPEAKER_NEXT_SEND_TYPE_MODULE    0x2

/* 
 * struct cerebrod_next_send_time
 *
 * Store information on when to next send information
 */
struct cerebrod_next_send_time
{
  u_int32_t next_send_type;
  u_int32_t next_send_time;

  /* For Metric Modules */
  int metric_period;
  int index;
};

/* 
 * cerebrod_speaker
 *
 * Runs the cerebrod speaker thread
 *
 * Passed no argument
 * 
 * Executed in detached state, no return value.
 */
void *cerebrod_speaker(void *);

/* 
 * cerebrod_send_heartbeat
 *
 * Sends a heartbeat.  Called by metric modules when necessary.
 *
 * Returns 0 on success, -1 on error.
 */
int cerebrod_send_heartbeat(struct cerebrod_heartbeat *hb);

#endif /* _CEREBROD_SPEAKER_H */
