/*****************************************************************************\
 *  $Id: cerebrod_event_server.h,v 1.1.2.2 2006-11-04 01:21:22 chu11 Exp $
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

#ifndef _CEREBROD_EVENT_SERVER_H
#define _CEREBROD_EVENT_SERVER_H

#include "cerebro/cerebro_event_protocol.h"

struct cerebrod_event_to_send
{
  char *event_name;
  int index;
  struct cerebro_event *event;
};

/*
 * cerebrod_queue_event
 *
 * Queue an event for sending
 */
void cerebrod_queue_event(struct cerebro_event *event, unsigned int index);

/* 
 * cerebrod_event_queue_monitor
 *
 * Runs the cerebrod event queue monitor which will send out events
 * when signaled.
 *
 * Passed no argument
 * 
 * Executed in detached state, no return value.
 */
void *cerebrod_event_queue_monitor(void *);

/* 
 * cerebrod_event_server
 *
 * Runs the cerebrod event server thread
 *
 * Passed no argument
 * 
 * Executed in detached state, no return value.
 */
void *cerebrod_event_server(void *);

#endif /* _CEREBROD_EVENT_SERVER_H */