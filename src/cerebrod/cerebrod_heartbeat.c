/*****************************************************************************\
 *  $Id: cerebrod_heartbeat.c,v 1.27 2005-06-06 20:39:55 achu Exp $
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "cerebro/cerebrod_heartbeat_protocol.h"
/* XXX */
#include "cerebro/cerebro_metric_protocol.h"

#include "cerebrod_heartbeat.h"
#include "cerebrod_config.h"
#include "wrappers.h"

extern struct cerebrod_config conf;
#if CEREBRO_DEBUG
extern pthread_mutex_t debug_output_mutex;
#endif /* CEREBRO_DEBUG */

void
cerebrod_heartbeat_dump(struct cerebrod_heartbeat *hb)
{
#if CEREBRO_DEBUG
  assert(hb);

  if (conf.debug)
    {
      int i, rv;

      rv = Pthread_mutex_trylock(&debug_output_mutex);
      if (rv != EBUSY)
	{
	  fprintf(stderr, "%s(%s:%d): mutex not locked: rv=%d",
                  __FILE__, __FUNCTION__, __LINE__, rv);
	  exit(1);
	}
      
      fprintf(stderr, "**************************************\n");
      fprintf(stderr, "* Cerebrod Heartbeat:\n");     
      fprintf(stderr, "* -------------------\n");
      fprintf(stderr, "* version: %d\n", hb->version);
      fprintf(stderr, "* nodename: \"%s\"\n", hb->nodename);
      fprintf(stderr, "* metrics_len: %d\n", hb->metrics_len);
      for (i = 0; i < hb->metrics_len; i++)
        {
          fprintf(stderr, "* %s: metric_type = %d, metric_len = %d ", 
                  hb->metrics[i]->metric_name, 
                  hb->metrics[i]->metric_type,
                  hb->metrics[i]->metric_len);

          switch(hb->metrics[i]->metric_type)
            {
            case CEREBRO_METRIC_TYPE_NONE:
              break;
            case CEREBRO_METRIC_TYPE_BOOL:
              fprintf(stderr, "metric_data = %d\n", 
                      *((char *)hb->metrics[i]->metric_data));
              break;
            case CEREBRO_METRIC_TYPE_INT32:
              fprintf(stderr, "metric_data = %d\n", 
                      *((int32_t *)hb->metrics[i]->metric_data));
              break;
            case CEREBRO_METRIC_TYPE_UNSIGNED_INT32:
              fprintf(stderr, "metric_data = %u\n", 
                      *((u_int32_t *)hb->metrics[i]->metric_data));
              break;
            case CEREBRO_METRIC_TYPE_FLOAT:
              fprintf(stderr, "metric_data = %f\n", 
                      *((float *)hb->metrics[i]->metric_data));
              break;
            case CEREBRO_METRIC_TYPE_DOUBLE:
              fprintf(stderr, "metric_data = %f\n", 
                      *((double *)hb->metrics[i]->metric_data));
              break;
            case CEREBRO_METRIC_TYPE_STRING:
              fprintf(stderr, "metric_data = %s\n", 
                      (char *)hb->metrics[i]->metric_data);
              break;
            default:
              fprintf(stderr, "\n");
              break;
            }
        }
      fprintf(stderr, "**************************************\n");
    }
#endif /* CEREBRO_DEBUG */
}
