/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
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
 * @file scp_v1_mng.c
 * @brief scp version 1 implementation - management
 * @author Jay Sorg, Simone Fedele
 *
 */

#if defined(HAVE_CONFIG_H)
#include <config_ac.h>
#endif

#include "sesman.h"

#include "libscp.h"

static void parseCommonStates(enum SCP_SERVER_STATES_E e, const char *f);

/******************************************************************************/
enum SCP_SERVER_STATES_E
scp_v1_mng_process_msg(struct trans *atrans, struct SCP_SESSION *s)
{
    long data;
    enum SCP_SERVER_STATES_E e;
    struct SCP_DISCONNECTED_SESSION *slist = 0;
    int scount;
    int end = 0;

    data = auth_userpass(s->username, s->password, NULL);
    /*LOG_DEVEL(LOG_LEVEL_DEBUG, "user: %s\npass: %s", s->username, s->password);*/

    if (!data)
    {
        scp_v1s_mng_deny_connection(atrans, "Login failed");
        LOG(LOG_LEVEL_INFO,
            "[MNG] Login failed for user %s. Connection terminated", s->username);
        auth_end(data);
        return SCP_SERVER_STATE_END;
    }

    /* testing if login is allowed */
    if (0 == access_login_mng_allowed(s->username))
    {
        scp_v1s_mng_deny_connection(atrans, "Access to Terminal Server not allowed.");
        LOG(LOG_LEVEL_INFO,
            "[MNG] User %s not allowed on TS. Connection terminated", s->username);
        auth_end(data);
        return SCP_SERVER_STATE_END;
    }

    e = scp_v1s_mng_allow_connection(atrans, s);

    end = 1;

    while (end)
    {
        switch (e)
        {
            case SCP_SERVER_STATE_MNG_ACTION:
                LOG(LOG_LEVEL_INFO, "Connection cancelled after session listing");
                break;

            case SCP_SERVER_STATE_MNG_LISTREQ:
                /* list disconnected sessions */
                slist = session_get_byuser(NULL, &scount, SESMAN_SESSION_STATUS_ALL);
                LOG_DEVEL(LOG_LEVEL_DEBUG, "sessions on TS: %d (slist: %p)", scount, slist);
                if (0 == slist)
                {
                    LOG(LOG_LEVEL_INFO, "No sessions on Terminal Server");
                }

                e = scp_v1s_mng_list_sessions(atrans, s, scount, slist);
                g_free(slist);
                break;
            default:
                /* we check the other errors */
                parseCommonStates(e, "scp_v1s_mng_list_sessions()");
                end = 0;
                break;
        }
    }

    /* cleanup */
    auth_end(data);
    return SCP_SERVER_STATE_END;
}

static void parseCommonStates(enum SCP_SERVER_STATES_E e, const char *f)
{
    switch (e)
    {
        case SCP_SERVER_STATE_VERSION_ERR:
            LOG(LOG_LEVEL_WARNING, "version error");
        case SCP_SERVER_STATE_SIZE_ERR:
            /* an unknown scp version was requested, so we shut down the */
            /* connection (and log the fact)                             */
            LOG(LOG_LEVEL_WARNING,
                "protocol violation. connection closed.");
            break;
        case SCP_SERVER_STATE_NETWORK_ERR:
            LOG(LOG_LEVEL_WARNING, "libscp network error.");
            break;
        case SCP_SERVER_STATE_SEQUENCE_ERR:
            LOG(LOG_LEVEL_WARNING, "libscp sequence error.");
            break;
        case SCP_SERVER_STATE_INTERNAL_ERR:
            /* internal error occurred (eg. malloc() error, ecc.) */
            LOG(LOG_LEVEL_ERROR, "libscp internal error occurred.");
            break;
        default:
            /* dummy: scp_v1s_request_password won't generate any other */
            /* error other than the ones before                         */
            LOG(LOG_LEVEL_ALWAYS, "unknown return from %s", f);
            break;
    }
}
