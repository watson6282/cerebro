/*****************************************************************************\
 *  $Id: config_module.c,v 1.6 2005-06-27 20:23:38 achu Exp $
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
#include "cerebro/cerebro_config_module.h"
#include "cerebro/cerebro_constants.h"
#include "cerebro/cerebro_error.h"

#include "config_module.h"
#include "module_util.h"

#include "debug.h"
#include "ltdl.h"

/*
 * config_modules
 * config_modules_len
 *
 * dynamic configuration modules to search for by default
 */
char *config_modules[] = {
  "cerebro_config_gendersllnl.so",
  NULL
};
int config_modules_len = 1;

#define CONFIG_FILENAME_SIGNATURE      "cerebro_config_"

#define CONFIG_MODULE_DIR              CONFIG_MODULE_BUILDDIR "/.libs"

#define CONFIG_MODULE_MAGIC_NUMBER     0x33882211

/* 
 * struct config_module
 *
 * config module handle
 */
struct config_module
{
  int32_t magic;
  lt_dlhandle dl_handle;
  struct cerebro_config_module_info *module_info;
};

extern struct cerebro_config_module_info default_config_module_info;
extern int module_setup_count;

/* 
 * _config_module_loader
 *
 * If compiled statically, attempt to load the module specified by the
 * module name.
 *
 * If compiled dynamically, attempt to load the module specified by
 * the module_path.
 *
 * Return 1 is module is loaded, 0 if not, -1 on fatal error
 */
static int 
_config_module_loader(void *handle, char *module)
{
  lt_dlhandle dl_handle = NULL;
  struct cerebro_config_module_info *module_info = NULL;
  config_module_t config_handle = (config_module_t)handle;

  if (!module_setup_count)
    {
      CEREBRO_DBG(("cerebro_module_library uninitialized"));
      return -1;
    }

  if (!config_handle)
    {
      CEREBRO_DBG(("config_handle null"));
      return -1;
    }

  if (config_handle->magic != CONFIG_MODULE_MAGIC_NUMBER)
    {
      CEREBRO_DBG(("config_handle magic number invalid"));
      return -1;
    }

  if (!module)
    {
      CEREBRO_DBG(("module null"));
      return -1;
    }
  
  if (!(dl_handle = lt_dlopen(module)))
    {
      CEREBRO_DBG(("lt_dlopen: module=%s, %s", module, lt_dlerror()));
      goto cleanup;
    }

  /* clear lt_dlerror */
  lt_dlerror();

  if (!(module_info = lt_dlsym(dl_handle, "config_module_info")))
    {
      const char *err = lt_dlerror();
      if (err)
	CEREBRO_DBG(("lt_dlsym: module=%s, %s", module, err));
      goto cleanup;
    }

  if (!module_info->config_module_name
      || !module_info->setup
      || !module_info->cleanup
      || !module_info->load_default)
    {
      CEREBRO_DBG(("invalid module info"));
      goto cleanup;
    }

  config_handle->dl_handle = dl_handle;
  config_handle->module_info = module_info;
  return 1;

 cleanup:
  if (dl_handle)
    lt_dlclose(dl_handle);
  return 0;
}

config_module_t
config_module_load(void)
{
  struct config_module *config_handle = NULL;
  int rv;

  if (module_setup() < 0)
    return NULL;

  if (!(config_handle = (struct config_module *)malloc(sizeof(struct config_module))))
    {
      CEREBRO_DBG(("malloc: %s", strerror(errno)));
      return NULL;
    }
  memset(config_handle, '\0', sizeof(struct config_module));
  config_handle->magic = CONFIG_MODULE_MAGIC_NUMBER;

#if CEREBRO_DEBUG
  if ((rv = find_known_module(CONFIG_MODULE_DIR,
			      config_modules,
			      config_modules_len,
			      _config_module_loader,
			      config_handle)) < 0)
    goto cleanup;

  if (rv)
    goto out;
#endif /* CEREBRO_DEBUG */

  if ((rv = find_known_module(CEREBRO_MODULE_DIR,
			      config_modules,
			      config_modules_len,
			      _config_module_loader,
			      config_handle)) < 0)
    goto cleanup;

  if (rv)
    goto out;

  if ((rv = find_modules(CEREBRO_MODULE_DIR,
			 CONFIG_FILENAME_SIGNATURE,
			 _config_module_loader,
			 config_handle,
			 1)) < 0)
    goto cleanup;

  if (rv)
    goto out;

  config_handle->dl_handle = NULL;
  config_handle->module_info = &default_config_module_info;
 out:
  return config_handle;

 cleanup:
  if (config_handle)
    {
      if (config_handle->dl_handle)
        lt_dlclose(config_handle->dl_handle);
      free(config_handle);
    }
  module_cleanup();
  return NULL;
}

/*
 * config_module_handle_check
 *
 * Check for proper config module handle
 *
 * Returns 0 on success, -1 on error
 */
static int
config_module_handle_check(config_module_t config_handle)
{
  if (!config_handle 
      || config_handle->magic != CONFIG_MODULE_MAGIC_NUMBER
      || !config_handle->module_info)
    {
      CEREBRO_DBG(("invalid config_handle"));
      return -1;
    }

  return 0;
}

int
config_module_unload(config_module_t config_handle)
{
  if (config_module_handle_check(config_handle) < 0)
    return -1;
  
  config_module_cleanup(config_handle);

  config_handle->magic = ~CONFIG_MODULE_MAGIC_NUMBER;
  if (config_handle->dl_handle)
    lt_dlclose(config_handle->dl_handle);
  config_handle->module_info = NULL;
  free(config_handle);

  module_cleanup();
  return 0;
}

char *
config_module_name(config_module_t config_handle)
{
  if (config_module_handle_check(config_handle) < 0)
    return NULL;

  return (config_handle->module_info)->config_module_name;
}

int
config_module_setup(config_module_t config_handle)
{
  if (config_module_handle_check(config_handle) < 0)
    return -1;
  
  return ((*(config_handle->module_info)->setup)());
}

int
config_module_cleanup(config_module_t config_handle)
{
  if (config_module_handle_check(config_handle) < 0)
    return -1;
  
  return ((*(config_handle->module_info)->cleanup)());
}

int
config_module_load_default(config_module_t config_handle,
			   struct cerebro_config *conf)
{
  if (config_module_handle_check(config_handle) < 0)
    return -1;
  
  return ((*(config_handle->module_info)->load_default)(conf));
}
