#ifndef _URBDRC_H_
#define _URBDRC_H_

#include "chansrv.h"

struct urbdrc_drdynvc_procs
{
    int (*drdynvc_open) (const char*, int, struct chansrv_drdynvc_procs*, int*);
    int (*drdynvc_data) (int, const char*, int);
    int (*drdynvc_send_data) (int, const char*, int);
};

int urbdrc_init(struct urbdrc_drdynvc_procs *procs);
int urbdrc_deinit(void);

int urbdrc_check_wait_objs(void);
int urbdrc_get_wait_objs(tbus *objs, int *count, int *timeout);

#endif // _URBDRC_H_
