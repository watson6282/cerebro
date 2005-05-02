/*****************************************************************************\
 *  $Id: cerebro_updown.c,v 1.19 2005-05-02 20:42:25 achu Exp $
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#include <sys/select.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else  /* !TIME_WITH_SYS_TIME */
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else /* !HAVE_SYS_TIME_H */
#  include <time.h>
# endif /* !HAVE_SYS_TIME_H */
#endif /* !TIME_WITH_SYS_TIME */
#include <sys/types.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "cerebro.h"
#include "cerebro_api.h"
#include "cerebro_module.h"
#include "cerebro_util.h"
#include "cerebro_updown.h"
#include "cerebro_updown_protocol.h"
#include "cerebro_marshalling.h"
#include "hostlist.h"
#include "fd.h"

#define CEREBRO_UPDOWN_MAGIC_NUMBER             0xF00F5678

/* 
 * cerebro_updown_data
 *
 * cerebro update state date stored in cerebro handle's updown_data
 * pointer.
 */
struct cerebro_updown_data {
  int32_t magic;
  hostlist_t up_nodes;
  hostlist_t down_nodes;
};

/* 
 * _cerebro_updown_data_check
 *
 * Check for a proper updown_data handle, setting the errnum
 * appropriately if an error is found.
 *
 * Returns 0 on success, -1 on error
 */
static int
_cerebro_updown_data_check(cerebro_t handle, 
                           struct cerebro_updown_data *updown_data)
{
#ifndef NDEBUG
  if (!updown_data)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }
#endif /* NDEBUG */

  if (updown_data->magic != CEREBRO_UPDOWN_MAGIC_NUMBER)
    {
      handle->errnum = CEREBRO_ERR_MAGIC_NUMBER;
      return -1;
    }

  return 0;
}

/* 
 * _cerebro_updown_protocol_err_conversion
 *
 * Convert updown protocol err codes to API err codes
 *
 * Returns proper err code
 */
static int
_cerebro_updown_protocol_err_conversion(u_int32_t protocol_error)
{
  switch(protocol_error)
    {
    case CEREBRO_UPDOWN_PROTOCOL_ERR_SUCCESS:
      return CEREBRO_ERR_SUCCESS;
    case CEREBRO_UPDOWN_PROTOCOL_ERR_VERSION_INVALID:
      return CEREBRO_ERR_VERSION_INCOMPATIBLE;
    case CEREBRO_UPDOWN_PROTOCOL_ERR_PACKET_INVALID:
      return CEREBRO_ERR_PROTOCOL;
    case CEREBRO_UPDOWN_PROTOCOL_ERR_UPDOWN_REQUEST_INVALID:
    case CEREBRO_UPDOWN_PROTOCOL_ERR_TIMEOUT_INVALID:
    case CEREBRO_UPDOWN_PROTOCOL_ERR_INTERNAL_SYSTEM_ERROR:
    default:
      return CEREBRO_ERR_INTERNAL;
    }
}

/* 
 * _cerebro_updown_data_loaded_check
 *
 * Checks if the handle contains properly loaded updown_data data.
 * 
 * Returns 0 on success, -1 on error
 */
static int
_cerebro_updown_data_loaded_check(cerebro_t handle)
{
  struct cerebro_updown_data *updown_data;

  if (cerebro_handle_check(handle) < 0)
    return -1;

  if (!(handle->loaded_state & CEREBRO_UPDOWN_DATA_LOADED))
    {
      handle->errnum = CEREBRO_ERR_NOT_LOADED;
      return -1;
    }

  updown_data = (struct cerebro_updown_data *)handle->updown_data;
  if (_cerebro_updown_data_check(handle, updown_data) < 0)
    return -1;

  return 0;
}

/* 
 * _cerebro_updown_init_request
 *
 * Initialize an updown request
 *
 * Returns 0 on success, -1 on error
 */
static int
_cerebro_updown_init_request(cerebro_t handle,
                             struct cerebro_updown_request *req,
                             unsigned int timeout_len,
                             int flags)
{
  u_int32_t updown_request;

  if (flags == CEREBRO_UPDOWN_UP_NODES)
    updown_request = CEREBRO_UPDOWN_PROTOCOL_REQUEST_UP_NODES;
  else if (flags == CEREBRO_UPDOWN_DOWN_NODES)
    updown_request = CEREBRO_UPDOWN_PROTOCOL_REQUEST_DOWN_NODES;
  else 
    updown_request = CEREBRO_UPDOWN_PROTOCOL_REQUEST_UP_AND_DOWN_NODES;
    
  req->version = CEREBRO_UPDOWN_PROTOCOL_VERSION;
  req->updown_request = updown_request;
  req->timeout_len = timeout_len;

  return 0;
}

/* 
 * _cerebro_updown_request_marshall
 *
 * Marshall contents of a updown server request
 *
 * Returns length written to buffer on success, -1 on error
 */
static int
_cerebro_updown_request_marshall(cerebro_t handle,
                                 struct cerebro_updown_request *req,
                                 char *buffer,
                                 int bufferlen)
{
  int ret, c = 0;

#ifndef NDEBUG
  if (!buffer || bufferlen < CEREBRO_UPDOWN_REQUEST_LEN)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }
#endif /* NDEBUG */

  memset(buffer, '\0', bufferlen);

  if ((ret = cerebro_marshall_int32(req->version,
                                    buffer + c, 
                                    bufferlen - c)) < 0)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }
  c += ret;

  if ((ret = cerebro_marshall_uint32(req->updown_request,
                                     buffer + c, 
                                     bufferlen - c)) < 0)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }
  c += ret;

  if ((ret = cerebro_marshall_uint32(req->timeout_len,
                                     buffer + c, 
                                     bufferlen - c)) < 0)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }
  c += ret;

  return c;
}

/* 
 * _cerebro_updown_response_unmarshall
 *
 * Marshall contents of a updown server request
 *
 * Returns 0 on success, -1 on error
 */
static int
_cerebro_updown_response_unmarshall(cerebro_t handle,
                                    struct cerebro_updown_response *res,
                                    char *buffer,
                                    int bufferlen)
{
  int ret, c = 0;

#ifndef NDEBUG
  if (!buffer)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }
#endif /* NDEBUG */

  if ((ret = cerebro_unmarshall_int32(&(res->version),
                                      buffer + c,
                                      bufferlen - c)) < 0)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }
  if (!ret)
    return c;

  c += ret;

  if ((ret = cerebro_unmarshall_uint32(&(res->updown_err_code),
                                       buffer + c,
                                       bufferlen - c)) < 0)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }
  if (!ret)
    return c;

  c += ret;

  if ((ret = cerebro_unmarshall_uint8(&(res->end_of_responses),
                                      buffer + c,
                                      bufferlen - c)) < 0)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }
  if (!ret)
    return c;

  c += ret;

  if ((ret = cerebro_unmarshall_buffer(res->nodename,
                                       sizeof(res->nodename),
                                       buffer + c,
                                       bufferlen - c)) < 0)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }
  if (!ret)
    return c;

  c += ret;

  if ((ret = cerebro_unmarshall_uint8(&(res->updown_state),
                                      buffer + c,
                                      bufferlen - c)) < 0)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }
  if (!ret)
    return c;

  c += ret;

  return c;
}

/* 
 * _cerebro_updown_send_request
 *
 * Send the updown request
 *
 * Returns 0 on success, -1 on error
 */
static int
_cerebro_updown_send_request(cerebro_t handle,
                             int fd,
                             unsigned int timeout_len,
                             int flags)
{
  struct cerebro_updown_request req;
  char buffer[CEREBRO_PACKET_BUFLEN];
  int req_len;

  if (_cerebro_updown_init_request(handle, &req, timeout_len, flags) < 0)
    return -1;
  
  if ((req_len = _cerebro_updown_request_marshall(handle,
                                                  &req,
                                                  buffer,
                                                  CEREBRO_PACKET_BUFLEN)) < 0)
    return -1;

  if (fd_write_n(fd, buffer, req_len) < 0)
    {
      handle->errnum = CEREBRO_ERR_INTERNAL;
      return -1;
    }

  return 0;
}

/* 
 * _cerebro_updown_receive_a_response
 *
 * Receive a single response
 *
 * Returns response packet and length of packet unmarshalled on
 * success, -1 on error
 */
static int
_cerebro_updown_receive_a_response(cerebro_t handle,
                                   int fd,
                                   struct cerebro_updown_response *res)
{
  int rv, bytes_read = 0;
  char buffer[CEREBRO_PACKET_BUFLEN];

  memset(buffer, '\0', CEREBRO_PACKET_BUFLEN);
  while (bytes_read < CEREBRO_UPDOWN_RESPONSE_LEN)
    {
      fd_set rfds;
      struct timeval tv;
      tv.tv_sec = CEREBRO_UPDOWN_PROTOCOL_TIMEOUT_LEN;
      tv.tv_usec = 0;

      FD_ZERO(&rfds);
      FD_SET(fd, &rfds);

      if ((rv = select(fd + 1, &rfds, NULL, NULL, &tv)) < 0)
        {
          handle->errnum = CEREBRO_ERR_INTERNAL;
          goto cleanup;
        }
      
      if (!rv)
        {
          /* Timed out.  If atleast some bytes were read, unmarshall
           * the received bytes.  Its possible we are expecting more
           * bytes than the client is sending, perhaps because we are
           * using a different protocol version.  This will allow the
           * server to return a invalid version number back to the
           * user.
           */
          if (!bytes_read) 
            {
              handle->errnum = CEREBRO_ERR_PROTOCOL_TIMEOUT;
              goto cleanup;
            }
          else
            goto unmarshall_received;
        }

      if (FD_ISSET(fd, &rfds))
        {
          int n;

          /* Don't use fd_read_n b/c it loops until exactly
           * CEREBRO_UPDOWN_RESPONSE_LEN is read.  Due to version
           * incompatability or error packets, we may want to read a
           * smaller packet.
           */
          if ((n = read(fd,
                        buffer + bytes_read,
                        CEREBRO_UPDOWN_RESPONSE_LEN - bytes_read)) < 0)
            {
              handle->errnum = CEREBRO_ERR_INTERNAL;
              goto cleanup;
            }

          if (!n)
            {
              /* Pipe closed */
              handle->errnum = CEREBRO_ERR_PROTOCOL;
              goto cleanup;
            }

          bytes_read += n;
        }
      else
        {
          handle->errnum = CEREBRO_ERR_INTERNAL;
          goto cleanup;
        }
    }

 unmarshall_received:
  if ((rv = _cerebro_updown_response_unmarshall(handle, 
                                                res, 
                                                buffer, 
                                                bytes_read)) < 0)
    goto cleanup;

  return rv;

 cleanup:
  return -1;
}

/* 
 * _cerebro_updown_receive_responses
 *
 * Receive all of the updown server responses.
 * 
 * Returns 0 on success, -1 on error
 */
static int
_cerebro_updown_receive_responses(cerebro_t handle,
                                  int fd,
                                  struct cerebro_updown_data *updown_data,
                                  int flags)
{
  struct cerebro_updown_response res;
  int res_len;

  while (1)
    {
      if ((res_len = _cerebro_updown_receive_a_response(handle,
                                                        fd, 
                                                        &res)) < 0)
        goto cleanup;

      if (res_len != CEREBRO_UPDOWN_RESPONSE_LEN)
        {
          if (res_len == CEREBRO_UPDOWN_RESPONSE_LEN)
            {
              if (res.version != CEREBRO_UPDOWN_PROTOCOL_VERSION)
                {
                  handle->errnum = CEREBRO_ERR_PROTOCOL;
                  goto cleanup;
                }

              if (res.updown_err_code != CEREBRO_UPDOWN_PROTOCOL_ERR_SUCCESS)
                {
                  handle->errnum = _cerebro_updown_protocol_err_conversion(res.updown_err_code);;
                  goto cleanup;
                }
            }

          handle->errnum = CEREBRO_ERR_PROTOCOL;
          goto cleanup;
        }

      if (res.version != CEREBRO_UPDOWN_PROTOCOL_VERSION)
        {
          handle->errnum = CEREBRO_ERR_PROTOCOL;
          goto cleanup;
        }
      
      if (res.updown_err_code != CEREBRO_UPDOWN_PROTOCOL_ERR_SUCCESS)
        {
          handle->errnum = _cerebro_updown_protocol_err_conversion(res.updown_err_code);
          goto cleanup;
        }

      if (res.end_of_responses == CEREBRO_UPDOWN_PROTOCOL_IS_LAST_RESPONSE)
        break;

      if (flags == CEREBRO_UPDOWN_UP_NODES
          && res.updown_state != CEREBRO_UPDOWN_PROTOCOL_STATE_NODE_UP)
        {
          handle->errnum = CEREBRO_ERR_PROTOCOL;
          goto cleanup;
        }
      
      if (flags == CEREBRO_UPDOWN_DOWN_NODES
          && res.updown_state != CEREBRO_UPDOWN_PROTOCOL_STATE_NODE_DOWN)
        {
          handle->errnum = CEREBRO_ERR_PROTOCOL;
          goto cleanup;
        }

      if (res.updown_state == CEREBRO_UPDOWN_PROTOCOL_STATE_NODE_UP)
        {
          if (!hostlist_push(updown_data->up_nodes, res.nodename))
            {
              handle->errnum = CEREBRO_ERR_INTERNAL;
              goto cleanup;
            }
        }
      else if (res.updown_state == CEREBRO_UPDOWN_PROTOCOL_STATE_NODE_DOWN)
        {
          if (!hostlist_push(updown_data->down_nodes, res.nodename))
            {
              handle->errnum = CEREBRO_ERR_INTERNAL;
              goto cleanup;
            }
        }
      else
        {
          handle->errnum = CEREBRO_ERR_PROTOCOL;
          goto cleanup;
        }
    }

  return 0;
 cleanup:
  return -1;
}

/*  
 * _cerebro_updown_get_updown_data
 *
 * Get updown data and store it appropriately into the updown_data
 * structure.
 *
 * Returns 0 on success, -1 on error
 */
static int
_cerebro_updown_get_updown_data(cerebro_t handle,
                                struct cerebro_updown_data *updown_data,
                                const char *hostname,
                                unsigned int port,
                                unsigned int timeout_len,
                                int flags)
{
  int fd = -1, ret = -1;

  if ((fd = cerebro_low_timeout_connect(handle, 
                                        hostname, 
                                        port, 
                                        CEREBRO_UPDOWN_PROTOCOL_CONNECT_TIMEOUT_LEN)) < 0)
    goto cleanup;

  if (_cerebro_updown_send_request(handle, fd, timeout_len, flags) < 0)
    goto cleanup;

  if (_cerebro_updown_receive_responses(handle, fd, updown_data, flags) < 0)
    goto cleanup;
  
  ret = 0;

 cleanup:
  close(fd);
  return ret;
}

int 
cerebro_updown_load_data(cerebro_t handle, 
                         const char *hostname, 
                         unsigned int port, 
                         unsigned int timeout_len,
                         int flags)
{
  struct cerebro_updown_data *updown_data = NULL;

  if (cerebro_handle_check(handle) < 0)
    goto cleanup;

  if (flags 
      && flags != CEREBRO_UPDOWN_UP_NODES
      && flags != CEREBRO_UPDOWN_DOWN_NODES
      && flags != CEREBRO_UPDOWN_UP_AND_DOWN_NODES)
    {
      handle->errnum = CEREBRO_ERR_PARAMETERS;
      goto cleanup;
    }

  if (!(updown_data = (struct cerebro_updown_data *)malloc(sizeof(struct cerebro_updown_data))))
    {
      handle->errnum = CEREBRO_ERR_OUTMEM;
      goto cleanup;
    }
  memset(updown_data, '\0', sizeof(struct cerebro_updown_data));
  updown_data->magic = CEREBRO_UPDOWN_MAGIC_NUMBER;
  updown_data->up_nodes = NULL;
  updown_data->down_nodes = NULL;

  if (flags & CEREBRO_UPDOWN_UP_NODES)
    {
      if (!(updown_data->up_nodes = hostlist_create(NULL)))
        {
          handle->errnum = CEREBRO_ERR_OUTMEM;
          goto cleanup;
        }
    }

  if (flags & CEREBRO_UPDOWN_DOWN_NODES)
    {
      if (!(updown_data->down_nodes = hostlist_create(NULL)))
        {
          handle->errnum = CEREBRO_ERR_OUTMEM;
          goto cleanup;
        }
    }

  if (!(handle->loaded_state & CEREBRO_CONFIG_FILE_LOADED))
    {
      if (cerebro_api_load_config_file(handle) < 0)
	goto cleanup;
    }

  if (!(handle->loaded_state & CEREBRO_CLUSTERLIST_MODULE_FOUND))
    {
      if (cerebro_api_load_clusterlist_module(handle) < 0)
	goto cleanup;
    }

  if (!port)
    {
      if (handle->config_file_data.cerebro_updown_port_flag)
	{
	  if (!handle->config_file_data.cerebro_updown_port)
	    {
	      handle->errnum = CEREBRO_ERR_CONFIG_FILE;
	      goto cleanup;
	    }
	  port = handle->config_file_data.cerebro_updown_port;
	}
      else
	port = CEREBRO_UPDOWN_SERVER_PORT;
    }

  if (!timeout_len)
    {
      if (handle->config_file_data.cerebro_updown_timeout_len_flag)
	{
	  if (!handle->config_file_data.cerebro_updown_timeout_len)
	    {
	      handle->errnum = CEREBRO_ERR_CONFIG_FILE;
	      goto cleanup;
	    }
	  timeout_len = handle->config_file_data.cerebro_updown_timeout_len;
	}
      else
	timeout_len = CEREBRO_UPDOWN_TIMEOUT_LEN_DEFAULT;
    }

  if (!flags)
    {
      if (handle->config_file_data.cerebro_updown_flags_flag)
	{
	  if (handle->config_file_data.cerebro_updown_flags != CEREBRO_UPDOWN_UP_NODES
	      && handle->config_file_data.cerebro_updown_flags != CEREBRO_UPDOWN_DOWN_NODES
	      && handle->config_file_data.cerebro_updown_flags != CEREBRO_UPDOWN_UP_AND_DOWN_NODES)

	    {
	      handle->errnum = CEREBRO_ERR_CONFIG_FILE;
	      goto cleanup;
	    }
	  flags = handle->config_file_data.cerebro_updown_flags;
	}
      else
	flags = CEREBRO_UPDOWN_UP_AND_DOWN_NODES;
    }
  
  if (!hostname)
    {
      if (handle->config_file_data.cerebro_updown_hostnames_flag)
	{
	  int rv, i;

	  for (i = 0; i < handle->config_file_data.cerebro_updown_hostnames_len; i++)
	    {
	      if ((rv = _cerebro_updown_get_updown_data(handle,
							updown_data,
							"localhost",
							port,
							timeout_len,
							flags)) < 0)
		continue;
	      break;
	    }
	  
	  if (rv < 0)
	    goto cleanup;
	}
      else
	{
	  if (_cerebro_updown_get_updown_data(handle,
					      updown_data,
					      "localhost",
					      port,
					      timeout_len,
					      flags) < 0)
	    goto cleanup;
	}
    }
  else
    {
      if (_cerebro_updown_get_updown_data(handle,
					  updown_data,
					  hostname,
					  port,
					  timeout_len,
					  flags) < 0)
	goto cleanup;
    }



  if (handle->loaded_state & CEREBRO_UPDOWN_DATA_LOADED)
    {
      struct cerebro_updown_data *updown_data_temp;

      updown_data_temp = (struct cerebro_updown_data *)handle->updown_data;

      if (_cerebro_updown_data_check(handle, updown_data_temp) < 0)
        goto cleanup;

      if (flags == CEREBRO_UPDOWN_UP_AND_DOWN_NODES)
        {
          if (updown_data_temp->up_nodes)
            hostlist_destroy(updown_data_temp->up_nodes);
          if (updown_data_temp->down_nodes)
            hostlist_destroy(updown_data_temp->down_nodes);
        }
      else if (flags == CEREBRO_UPDOWN_UP_NODES)
        {
          if (updown_data_temp->up_nodes)
            hostlist_destroy(updown_data_temp->up_nodes);
          if (updown_data_temp->down_nodes)
            updown_data->down_nodes = updown_data_temp->down_nodes;
        }
      else
        {
          if (updown_data_temp->down_nodes)
            hostlist_destroy(updown_data_temp->down_nodes);
          if (updown_data_temp->up_nodes)
            updown_data->up_nodes = updown_data_temp->up_nodes;
        }
      free(updown_data_temp);
      handle->loaded_state &= ~CEREBRO_UPDOWN_DATA_LOADED;
      handle->updown_data = NULL;
    }

  handle->loaded_state |= CEREBRO_UPDOWN_DATA_LOADED;
  handle->updown_data = (void *)updown_data;
  handle->errnum = CEREBRO_ERR_SUCCESS;
  return 0;
  
 cleanup:
  if (updown_data)
    {
      if (updown_data->up_nodes)
        {
          hostlist_destroy(updown_data->up_nodes);
          updown_data->up_nodes = NULL;
        }
      if (updown_data->down_nodes)
        {
          hostlist_destroy(updown_data->down_nodes);
          updown_data->down_nodes = NULL;
        }
      free(updown_data);
    }
  return -1;
}

/* 
 * _cerebro_updown_get_nodes
 *
 * Common function for cerebro_updown_get_up_nodes and
 * cerebro_updown_get_down_nodes.
 * 
 * Returns 0 on success, -1 on error
 */
static int
_cerebro_updown_get_nodes(cerebro_t handle, 
                          char *buf, 
                          unsigned int buflen,
                          int up_down_flag)
{
  struct cerebro_updown_data *updown_data;
  hostlist_t hl;

  if (_cerebro_updown_data_loaded_check(handle) < 0)
    return -1;

  if (!buf || buflen <= 0)
    {
      handle->errnum = CEREBRO_ERR_PARAMETERS;
      return -1;
    }

  updown_data = (struct cerebro_updown_data *)handle->updown_data;

  if (up_down_flag == CEREBRO_UPDOWN_UP_NODES)
    hl = updown_data->up_nodes;
  else
    hl = updown_data->down_nodes;

  if (!hl)
    {
      handle->errnum = CEREBRO_ERR_NOT_LOADED;
      return -1;
    }

  if (hostlist_ranged_string(hl, buflen, buf) < 0)
    {
      handle->errnum = CEREBRO_ERR_OVERFLOW;
      return -1;
    }

  handle->errnum = CEREBRO_ERR_SUCCESS;
  return 0;
}

int 
cerebro_updown_get_up_nodes(cerebro_t handle, char *buf, unsigned int buflen)
{
  return _cerebro_updown_get_nodes(handle, buf, buflen, CEREBRO_UPDOWN_UP_NODES);
}
 
int 
cerebro_updown_get_down_nodes(cerebro_t handle, char *buf, unsigned int buflen)
{
  return _cerebro_updown_get_nodes(handle, buf, buflen, CEREBRO_UPDOWN_DOWN_NODES);
}
 
/* 
 * _cerebro_updown_is_node
 *
 * Common function for cerebro_updown_is_node_up and
 * cerebro_updown_is_node_down.
 * 
 * Returns boolean on success, -1 on error
 */
static int
_cerebro_updown_is_node(cerebro_t handle, 
                        const char *node,
                        int up_down_flag)
{
  struct cerebro_updown_data *updown_data;
  char buffer[CEREBRO_MAXNODENAMELEN+1];
  hostlist_t hl;
  int rv, ret;

  if (_cerebro_updown_data_loaded_check(handle) < 0)
    return -1;

  if (!node)
    {
      handle->errnum = CEREBRO_ERR_PARAMETERS;
      return -1;
    }

  /* Special case: We can do better than the clusterlist module
   * default for cerebro_clusterlist_node_in_cluster.
   */
  if (handle->loaded_state & CEREBRO_CLUSTERLIST_MODULE_FOUND)
    {
      if ((rv = cerebro_clusterlist_node_in_cluster(node)) < 0)
	{
	  handle->errnum = CEREBRO_ERR_CLUSTERLIST_MODULE;
	  return -1;
	}

      if (!rv)
	{
	  handle->errnum = CEREBRO_ERR_NODE_NOTFOUND;
	  return -1;
	}

      memset(buffer, '\0', CEREBRO_MAXNODENAMELEN+1);
      
      if (cerebro_clusterlist_get_nodename(node,
					   buffer, 
					   CEREBRO_MAXNODENAMELEN) < 0)
	{
	  handle->errnum = CEREBRO_ERR_CLUSTERLIST_MODULE;
	  return -1;
	}
    }
  else
    {
      if (hostlist_find(updown_data->up_nodes, node) < 0
	  && hostlist_find(updown_data->down_nodes, node) < 0)
	{
	  handle->errnum = CEREBRO_ERR_NODE_NOTFOUND;
	  return -1;
	}
    }
  

  updown_data = (struct cerebro_updown_data *)handle->updown_data;

  if (up_down_flag == CEREBRO_UPDOWN_UP_NODES)
    hl = updown_data->up_nodes;
  else
    hl = updown_data->down_nodes;

  if (!hl)
    {
      handle->errnum = CEREBRO_ERR_NOT_LOADED;
      return -1;
    }

  rv = hostlist_find(hl, buffer);
 
  if (rv != -1)
    ret = 1;
  else
    ret = 0;

  handle->errnum = CEREBRO_ERR_SUCCESS;
  return ret;
}

int 
cerebro_updown_is_node_up(cerebro_t handle, const char *node)
{
  return _cerebro_updown_is_node(handle, node, CEREBRO_UPDOWN_UP_NODES);
}
 
int 
cerebro_updown_is_node_down(cerebro_t handle, const char *node)
{
  return _cerebro_updown_is_node(handle, node, CEREBRO_UPDOWN_DOWN_NODES);
}
 
/* 
 * _cerebro_updown_count
 *
 * Common function for cerebro_updown_up_count and
 * cerebro_updown_down_count.
 * 
 * Returns count on success, -1 on error
 */
static int
_cerebro_updown_count(cerebro_t handle, int up_down_flag)
{
  struct cerebro_updown_data *updown_data;
  hostlist_t hl;
  int count;

  if (_cerebro_updown_data_loaded_check(handle) < 0)
    return -1;

  updown_data = (struct cerebro_updown_data *)handle->updown_data;
  
  if (up_down_flag == CEREBRO_UPDOWN_UP_NODES)
    hl = updown_data->up_nodes;
  else
    hl = updown_data->down_nodes;

  if (!hl)
    {
      handle->errnum = CEREBRO_ERR_NOT_LOADED;
      return -1;
    }

  count = hostlist_count(hl);
  handle->errnum = CEREBRO_ERR_SUCCESS;
  return count;
}

int 
cerebro_updown_up_count(cerebro_t handle)
{
  return _cerebro_updown_count(handle, CEREBRO_UPDOWN_UP_NODES);
}

int 
cerebro_updown_down_count(cerebro_t handle)
{
  return _cerebro_updown_count(handle, CEREBRO_UPDOWN_DOWN_NODES);
}

int
cerebro_updown_unload_data(cerebro_t handle)
{
  if (cerebro_handle_check(handle) < 0)
    return -1;

  if (handle->loaded_state & CEREBRO_UPDOWN_DATA_LOADED)
    {
      struct cerebro_updown_data *updown_data;

      updown_data = (struct cerebro_updown_data *)handle->updown_data;

      if (_cerebro_updown_data_check(handle, updown_data) < 0)
        return -1;

      hostlist_destroy(updown_data->up_nodes);
      updown_data->up_nodes = NULL;
      hostlist_destroy(updown_data->down_nodes);
      updown_data->down_nodes = NULL;
      updown_data->magic = ~CEREBRO_UPDOWN_MAGIC_NUMBER;
      free(updown_data);
    }

  handle->updown_data = NULL;

  handle->loaded_state &= ~CEREBRO_UPDOWN_DATA_LOADED;
  handle->errnum = CEREBRO_ERR_SUCCESS;
  return 0;
}
