/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2015
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 *
 * @file access.c
 * @brief User access control code
 * @author Simone Fedele
 *
 */

#if defined(HAVE_CONFIG_H)
#include <config_ac.h>
#endif

#include "sesman.h"
#include "string_calls.h"

/******************************************************************************/
int
access_login_allowed(const char *user)
{
    int gid;
    int ok;

    if ((0 == g_strncmp(user, "root", 5)) && (0 == g_cfg->sec.allow_root))
    {
        LOG(LOG_LEVEL_WARNING,
            "ROOT login attempted, but root login is disabled");
        return 0;
    }

    if ((0 == g_cfg->sec.ts_users_enable) && (0 == g_cfg->sec.ts_always_group_check))
    {
        LOG(LOG_LEVEL_INFO, "Terminal Server Users group is disabled, allowing authentication");
        return 1;
    }

    if (0 != g_getuser_info(user, &gid, 0, 0, 0, 0))
    {
        LOG(LOG_LEVEL_ERROR, "Cannot read user info! - login denied");
        return 0;
    }

    if (g_cfg->sec.ts_users == gid)
    {
        LOG(LOG_LEVEL_DEBUG, "ts_users is user's primary group");
        return 1;
    }

    if (0 != g_check_user_in_group(user, g_cfg->sec.ts_users, &ok))
    {
        LOG(LOG_LEVEL_ERROR, "Cannot read group info! - login denied");
        return 0;
    }

    if (ok)
    {
        return 1;
    }

    LOG(LOG_LEVEL_INFO, "login denied for user %s", user);

    return 0;
}

/******************************************************************************/
int
access_login_mng_allowed(const char *user)
{
    int gid;
    int ok;

    if ((0 == g_strncmp(user, "root", 5)) && (0 == g_cfg->sec.allow_root))
    {
        LOG(LOG_LEVEL_WARNING,
            "[MNG] ROOT login attempted, but root login is disabled");
        return 0;
    }

    if (0 == g_cfg->sec.ts_admins_enable)
    {
        LOG(LOG_LEVEL_INFO, "[MNG] Terminal Server Admin group is disabled, "
            "allowing authentication");
        return 1;
    }

    if (0 != g_getuser_info(user, &gid, 0, 0, 0, 0))
    {
        LOG(LOG_LEVEL_ERROR, "[MNG] Cannot read user info! - login denied");
        return 0;
    }

    if (g_cfg->sec.ts_admins == gid)
    {
        LOG(LOG_LEVEL_INFO, "[MNG] ts_users is user's primary group");
        return 1;
    }

    if (0 != g_check_user_in_group(user, g_cfg->sec.ts_admins, &ok))
    {
        LOG(LOG_LEVEL_ERROR, "[MNG] Cannot read group info! - login denied");
        return 0;
    }

    if (ok)
    {
        return 1;
    }

    LOG(LOG_LEVEL_INFO, "[MNG] login denied for user %s", user);

    return 0;
}
