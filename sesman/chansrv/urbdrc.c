#include <dlfcn.h>
#include "urbdrc.h"

static void *g_urbdrc_library_handle;

static int (*g_urbdrc_init)(struct urbdrc_drdynvc_procs *);
static int (*g_urbdrc_deinit)(void);
static int (*g_urbdrc_check_wait_objs)(void);
static int (*g_urbdrc_get_wait_objs)(tbus *, int *, int *);

int 
urbdrc_init(struct urbdrc_drdynvc_procs *procs)
{
    g_urbdrc_library_handle = dlopen("liburbdrc.so", RTLD_LOCAL | RTLD_LAZY);
    if (g_urbdrc_library_handle == NULL)
    {
        return 0;
    }

    g_urbdrc_init = dlsym(g_urbdrc_library_handle, "urbdrc_init");
    if (g_urbdrc_init == NULL)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "Unable to resolve urbdrc_init function adress");
        dlclose(g_urbdrc_library_handle);
        g_urbdrc_library_handle = NULL;
        return 1;
    }

    g_urbdrc_deinit = dlsym(g_urbdrc_library_handle, "urbdrc_deinit");
    if (g_urbdrc_deinit == NULL)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "Unable to resolve urbdrc_deinit function adress");
        dlclose(g_urbdrc_library_handle);
        g_urbdrc_library_handle = NULL;
        g_urbdrc_init = NULL;
        return 1;
    }

    g_urbdrc_check_wait_objs = dlsym(g_urbdrc_library_handle, "urbdrc_check_wait_objs");
    if (g_urbdrc_check_wait_objs == NULL)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "Unable to resolve urbdrc_check_wait_objs function adress");
        dlclose(g_urbdrc_library_handle);
        g_urbdrc_library_handle = NULL;
        g_urbdrc_init = NULL;
        g_urbdrc_deinit = NULL;
        return 1;
    }

    g_urbdrc_get_wait_objs = dlsym(g_urbdrc_library_handle, "urbdrc_get_wait_objs");
    if (g_urbdrc_get_wait_objs == NULL)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "Unable to resolve urbdrc_get_wait_objs function adress");
        dlclose(g_urbdrc_library_handle);
        g_urbdrc_library_handle = NULL;
        g_urbdrc_init = NULL;
        g_urbdrc_deinit = NULL;
        g_urbdrc_check_wait_objs = NULL;
        return 1;
    }

    LOG_DEVEL(LOG_LEVEL_INFO, "The urbdrc module has been successfully loaded");

    return g_urbdrc_init(procs);
}

int
urbdrc_deinit(void)
{
   if (g_urbdrc_deinit != NULL)
   {
       g_urbdrc_deinit();
       dlclose(g_urbdrc_library_handle);
       g_urbdrc_library_handle = NULL;
       g_urbdrc_init = NULL;
       g_urbdrc_deinit = NULL;
       g_urbdrc_check_wait_objs = NULL;
       g_urbdrc_get_wait_objs = NULL;
   };
   return 0;
}

int
urbdrc_check_wait_objs(void)
{
    int ret = 0;

    if (g_urbdrc_check_wait_objs != NULL)
    {
        ret = g_urbdrc_check_wait_objs();
    };

    return ret;
}

int
urbdrc_get_wait_objs(tbus *objs, int *count, int *timeout)
{
    int ret = 0;

    if (g_urbdrc_get_wait_objs != NULL)
    {
        ret = g_urbdrc_get_wait_objs(objs, count, timeout);
    };

    return ret;
}
