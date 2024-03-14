/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2009-2012
 * Copyright (C) Laxmikant Rashinkar 2012
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
/*
   for help see
   http://tronche.com/gui/x/icccm/sec-2.html#s-2
   .../kde/kdebase/workspace/klipper/clipboardpoll.cpp

  Revision:
  Aug 05, 2012:
    Laxmikant Rashinkar (LK dot Rashinkar at gmail.com)
    added clipboard support for BMP images

  http://msdn.microsoft.com/en-us/library/cc241066%28v=prot.20%29.aspx

*/

/*
TARGETS
MULTIPLE
image/tiff
image/jpeg
image/x-MS-bmp
image/x-bmp
image/bmp
image/png
SAVE_TARGETS
TIMESTAMP

wininfo - show window info
xlsatoms - dump atoms

dolphin 1.4 KDE 4.4.5 (debian 6) copy one file
text/uri-list
text/x-moz-url
text/plain
UTF8_STRING
STRING
TEXT
COMPOUND_TEXT
application/x-qiconlist
TARGETS
MULTIPLE
TIMESTAMP
SAVE_TARGETS

dolphin 1.6.1 KDE 4.6.5 (kubuntu 11.04) copy one file
text/uri-list
text/x-moz-url
text/plain
UTF8_STRING
STRING
TEXT
COMPOUND_TEXT
application/x-qiconlist
TARGETS
MULTIPLE
TIMESTAMP
SAVE_TARGETS

kolourpaint 4.4.5 KDE 4.4.5 copy image area
application/x-kolourpaint-selection-400
application/x-qt-image
image/png
image/bw
image/eps
image/epsf
image/epsi
image/pcx
image/rgb
image/rgba
image/sgi
image/tga
image/bmp
image/ico
image/jp2
image/jpeg
image/jpg
image/ppm
PIXMAP
image/tif
image/tiff
image/xbm
image/xpm
image/xv
TARGETS
MULTIPLE
TIMESTAMP
SAVE_TARGETS

kate 3.4.5 KDE 4.4.5 copy text
text/plain
UTF8_STRING
STRING
TEXT
COMPOUND_TEXT
TARGETS
MULTIPLE
TIMESTAMP
SAVE_TARGETS

gimp 2.6.10 copy image area
TIMESTAMP
TARGETS
MULTIPLE
SAVE_TARGETS
image/png
image/bmp
image/x-bmp
image/x-MS-bmp
image/tiff
image/x-icon
image/x-ico
image/x-win-bitmap
image/jpeg

thunar 1.2.1 copy a file
TIMESTAMP
TARGETS
MULTIPLE
x-special/gnome-copied-files
UTF8_STRING

dolphin 1.6.1 KDE 4.6.5 (kubuntu 11.04) copy two files
text/uri-list
/home/jay/temp/jetstream1.txt
/home/jay/temp/jpeg64x64.jpg
0000 66 69 6c 65 3a 2f 2f 2f 68 6f 6d 65 2f 6a 61 79 file:///home/jay
0010 2f 74 65 6d 70 2f 6a 65 74 73 74 72 65 61 6d 31 /temp/jetstream1
0020 2e 74 78 74 0d 0a 66 69 6c 65 3a 2f 2f 2f 68 6f .txt..file:///ho
0030 6d 65 2f 6a 61 79 2f 74 65 6d 70 2f 6a 70 65 67 me/jay/temp/jpeg
0040 36 34 78 36 34 2e 6a 70 67                      64x64.jpg

thunar 1.2.1 (kubuntu 11.04)  copy two files
x-special/gnome-copied-files
/home/jay/temp/jetstream1.txt
/home/jay/temp/jpeg64x64.jpg
0000 63 6f 70 79 0a 66 69 6c 65 3a 2f 2f 2f 68 6f 6d copy.file:///hom
0010 65 2f 6a 61 79 2f 74 65 6d 70 2f 6a 65 74 73 74 e/jay/temp/jetst
0020 72 65 61 6d 31 2e 74 78 74 0d 0a 66 69 6c 65 3a ream1.txt..file:
0030 2f 2f 2f 68 6f 6d 65 2f 6a 61 79 2f 74 65 6d 70 ///home/jay/temp
0040 2f 6a 70 65 67 36 34 78 36 34 2e 6a 70 67 0d 0a /jpeg64x64.jpg..


*/

#if defined(HAVE_CONFIG_H)
#include <config_ac.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>
#include "arch.h"
#include "parse.h"
#include "os_calls.h"
#include "string_calls.h"
#include "chansrv.h"
#include "chansrv_common.h"
#include "chansrv_config.h"
#include "clipboard.h"
#include "clipboard_file.h"
#include "clipboard_common.h"
#include "xcommon.h"
#include "chansrv_fuse.h"
#include "ms-rdpeclip.h"

static char g_bmp_image_header[] =
{
    /* this is known to work */
    //0x42, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

    /* THIS IS BEING SENT BY WIN2008 */
    0x42, 0x4d, 0x16, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00
};

extern int g_cliprdr_chan_id;   /* in chansrv.c */

extern Display *g_display;      /* in xcommon.c */
extern int g_x_socket;          /* in xcommon.c */
extern tbus g_x_wait_obj;       /* in xcommon.c */
extern Screen *g_screen;        /* in xcommon.c */
extern int g_screen_num;        /* in xcommon.c */

extern struct config_chansrv *g_cfg; /* in chansrv.c */

int g_clip_up = 0;

static Atom g_clipboard_atom = 0;      /* CLIPBOARD */
static Atom g_clip_property_atom = 0;  /* XRDP_CLIP_PROPERTY_ATOM */
static Atom g_timestamp_atom = 0;      /* TIMESTAMP */
static Atom g_multiple_atom = 0;       /* MULTIPLE */
static Atom g_targets_atom = 0;        /* TARGETS */
static Atom g_primary_atom = 0;        /* PRIMARY */
static Atom g_secondary_atom = 0;      /* SECONDARY */
static Atom g_get_time_atom = 0;       /* XRDP_GET_TIME_ATOM */
static Atom g_utf8_atom = 0;           /* UTF8_STRING */
static Atom g_image_bmp_atom = 0;      /* image/bmp */
static Atom g_file_atom1 = 0;          /* text/uri-list */
static Atom g_file_atom2 = 0;          /* x-special/gnome-copied-files */
static Atom g_incr_atom = 0;           /* INCR */

static Window g_wnd = 0;
static int g_xfixes_event_base = 0;

static int g_got_selection = 0; /* boolean */
static Time g_selection_time = 0;

static struct stream *g_ins = 0;

/* for image data */
static XSelectionRequestEvent g_saved_selection_req_event;

/* xserver maximum request size in bytes */
static int g_incr_max_req_size = 0;

/* server to client, pasting from linux app to mstsc */
struct clip_s2c g_clip_s2c;
/* client to server, pasting from mstsc to linux app */
struct clip_c2s g_clip_c2s;

/* default version and flags */
static int g_cliprdr_version = CB_CAPS_VERSION_2;
static int g_cliprdr_flags = CB_USE_LONG_FORMAT_NAMES |
                             CB_STREAM_FILECLIP_ENABLED |
                             CB_FILECLIP_NO_FILE_PATHS;

/* from client to server */
/* last received CLIPRDR_FORMAT_LIST(CLIPRDR_FORMAT_ANNOUNCE) */
static int g_formatIds[16];
static int g_num_formatIds = 0;

static int g_file_format_id = -1;

static char g_last_atom_name[256] = "";

/*****************************************************************************/
static char *
get_atom_text(Atom atom)
{
    char *name;
    int failed;

    failed = 0;
    /* sanity check */
    if ((atom < 1) || (atom > 512))
    {
        failed = 1;
    }
    if (!failed)
    {
        name = XGetAtomName(g_display, atom);
        if (name == 0)
        {
            failed = 1;
        }
    }
    if (failed)
    {
        g_snprintf(g_last_atom_name, 255, "unknown atom 0x%8.8x", (int)atom);
        return g_last_atom_name;
    }
    g_strncpy(g_last_atom_name, name, 255);
    XFree(name);
    return g_last_atom_name;
}

/*****************************************************************************/
/* this is one way to get the current time from the x server */
static Time
clipboard_get_server_time(void)
{
    XEvent xevent;
    unsigned char no_text[4];

    /* append nothing */
    no_text[0] = 0;
    XChangeProperty(g_display, g_wnd, g_get_time_atom, XA_STRING, 8,
                    PropModeAppend, no_text, 0);

    /* wait for PropertyNotify */
    do
    {
        XMaskEvent(g_display, PropertyChangeMask, &xevent);
    }
    while (xevent.type != PropertyNotify);

    return xevent.xproperty.time;
}

/*****************************************************************************/
static int
clipboard_find_format_id(int format_id)
{
    int index;

    for (index = 0; index < g_num_formatIds; index++)
    {
        if (g_formatIds[index] == format_id)
        {
            return index;
        }
    }
    return -1;
}

/*****************************************************************************/
/* returns error */
int
clipboard_init(void)
{
    struct stream *s;
    int size;
    int rv;
    int input_mask;
    int dummy;
    int ver_maj;
    int ver_min;
    Status st;

    LOG_DEVEL(LOG_LEVEL_INFO, "clipboard_init:");

    if (g_clip_up)
    {
        return 0;
    }

    xfuse_init();
    xcommon_init();
    g_incr_max_req_size = XMaxRequestSize(g_display) * 4 - 24;
    g_memset(&g_clip_c2s, 0, sizeof(g_clip_c2s));
    g_memset(&g_clip_s2c, 0, sizeof(g_clip_s2c));
    rv = 0;
    if (rv == 0)
    {
        g_clipboard_atom = XInternAtom(g_display, "CLIPBOARD", False);

        if (g_clipboard_atom == None)
        {
            LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_init: XInternAtom failed");
            rv = 3;
        }
    }

    if (rv == 0)
    {
        if (!XFixesQueryExtension(g_display, &g_xfixes_event_base, &dummy))
        {
            LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_init: no xfixes");
            rv = 5;
        }
    }

    if (rv == 0)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_init: g_xfixes_event_base %d",
                  g_xfixes_event_base);
        st = XFixesQueryVersion(g_display, &ver_maj, &ver_min);
        LOG(LOG_LEVEL_DEBUG, "clipboard_init st %d, maj %d min %d", st,
            ver_maj, ver_min);
        g_clip_property_atom = XInternAtom(g_display, "XRDP_CLIP_PROPERTY_ATOM",
                                           False);
        g_get_time_atom = XInternAtom(g_display, "XRDP_GET_TIME_ATOM",
                                      False);
        g_timestamp_atom = XInternAtom(g_display, "TIMESTAMP", False);
        g_targets_atom = XInternAtom(g_display, "TARGETS", False);
        g_multiple_atom = XInternAtom(g_display, "MULTIPLE", False);
        g_primary_atom = XInternAtom(g_display, "PRIMARY", False);
        g_secondary_atom = XInternAtom(g_display, "SECONDARY", False);
        g_utf8_atom = XInternAtom(g_display, "UTF8_STRING", False);

        g_image_bmp_atom = XInternAtom(g_display, "image/bmp", False);
        g_file_atom1 = XInternAtom(g_display, "text/uri-list", False);
        g_file_atom2 = XInternAtom(g_display, "x-special/gnome-copied-files", False);
        g_incr_atom = XInternAtom(g_display, "INCR", False);

        if (g_image_bmp_atom == None)
        {
            LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_init: g_image_bmp_atom was "
                      "not allocated");
        }

        g_wnd = XCreateSimpleWindow(g_display, RootWindowOfScreen(g_screen),
                                    0, 0, 4, 4, 0, 0, 0);
        input_mask = StructureNotifyMask | PropertyChangeMask;
        XSelectInput(g_display, g_wnd, input_mask);
        //XMapWindow(g_display, g_wnd);
        XFixesSelectSelectionInput(g_display, g_wnd,
                                   g_clipboard_atom,
                                   XFixesSetSelectionOwnerNotifyMask |
                                   XFixesSelectionWindowDestroyNotifyMask |
                                   XFixesSelectionClientCloseNotifyMask);
    }

    make_stream(s);
    if (rv == 0)
    {
        /* set clipboard caps first */
        init_stream(s, 8192);
        /* CLIPRDR_HEADER */
        out_uint16_le(s, CB_CLIP_CAPS); /* msgType */
        out_uint16_le(s, 0); /* msgFlags */
        out_uint32_le(s, 16); /* dataLen */
        out_uint16_le(s, 1); /* cCapabilitiesSets */
        out_uint16_le(s, 0); /* pad1 */
        /* CLIPRDR_GENERAL_CAPABILITY */
        out_uint16_le(s, CB_CAPSTYPE_GENERAL); /* capabilitySetType */
        out_uint16_le(s, 12); /* lengthCapability */
        out_uint32_le(s, g_cliprdr_version); /* version */
        out_uint32_le(s, g_cliprdr_flags); /* generalFlags */
        out_uint32_le(s, 0); /* extra 4 bytes ? */
        s_mark_end(s);
        size = (int)(s->end - s->data);
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_init: data out, sending "
                  "CB_CLIP_CAPS (clip_msg_id = 1)");
        rv = send_channel_data(g_cliprdr_chan_id, s->data, size);
        if (rv != 0)
        {
            LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_init: send_channel_data failed "
                      "rv = %d", rv);
            rv = 4;
        }
    }

    if (rv == 0)
    {
        /* report clipboard ready */
        init_stream(s, 8192);
        out_uint16_le(s, CB_MONITOR_READY); /* msgType */
        out_uint16_le(s, 0); /* msgFlags */
        out_uint32_le(s, 0); /* dataLen */
        out_uint32_le(s, 0); /* extra 4 bytes ? */
        s_mark_end(s);
        size = (int)(s->end - s->data);
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_init: data out, sending "
                  "CB_MONITOR_READY (clip_msg_id = 1)");
        rv = send_channel_data(g_cliprdr_chan_id, s->data, size);
        if (rv != 0)
        {
            LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_init: send_channel_data failed "
                      "rv = %d", rv);
            rv = 4;
        }
    }
    free_stream(s);

    if (rv == 0)
    {
        g_clip_up = 1;
        make_stream(g_ins);
        init_stream(g_ins, 8192);
    }
    else
    {
        LOG_DEVEL(LOG_LEVEL_ERROR, "xrdp-chansrv: clipboard_init: error on exit");
    }

    return rv;
}

/*****************************************************************************/
int
clipboard_deinit(void)
{
    LOG_DEVEL(LOG_LEVEL_INFO, "clipboard_deinit:");
    if (g_wnd != 0)
    {
        XDestroyWindow(g_display, g_wnd);
        g_wnd = 0;
    }

    xfuse_deinit();

    g_free(g_clip_c2s.data);
    g_clip_c2s.data = 0;
    g_free(g_clip_s2c.data);
    g_clip_s2c.data = 0;

    free_stream(g_ins);
    g_ins = 0;
    g_clip_up = 0;
    return 0;
}

/*****************************************************************************/
static int
clipboard_send_data_request(int format_id)
{
    struct stream *s;
    int size;
    int rv;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_data_request:");
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_data_request: %d", format_id);
    g_clip_c2s.in_request = 1;
    make_stream(s);
    init_stream(s, 8192);
    out_uint16_le(s, CB_FORMAT_DATA_REQUEST); /* 4 CLIPRDR_DATA_REQUEST */
    out_uint16_le(s, 0); /* status */
    out_uint32_le(s, 4); /* length */
    out_uint32_le(s, format_id);
    out_uint32_le(s, 0);
    s_mark_end(s);
    size = (int)(s->end - s->data);
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_data_request: data out, sending "
              "CLIPRDR_DATA_REQUEST (clip_msg_id = 4)");
    rv = send_channel_data(g_cliprdr_chan_id, s->data, size);
    free_stream(s);
    return rv;
}

/*****************************************************************************/
static int
clipboard_send_format_ack(void)
{
    struct stream *s;
    int size;
    int rv;

    make_stream(s);
    init_stream(s, 8192);
    out_uint16_le(s, CB_FORMAT_LIST_RESPONSE); /* 3 CLIPRDR_FORMAT_ACK */
    out_uint16_le(s, CB_RESPONSE_OK); /* 1 status */
    out_uint32_le(s, 0); /* length */
    out_uint32_le(s, 0); /* extra 4 bytes */
    s_mark_end(s);
    size = (int)(s->end - s->data);
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_format_ack: data out, sending "
              "CLIPRDR_FORMAT_ACK (clip_msg_id = 3)");
    rv = send_channel_data(g_cliprdr_chan_id, s->data, size);
    free_stream(s);
    return rv;
}

/*****************************************************************************/
/* returns number of bytes written */
int
clipboard_out_unicode(struct stream *s, const char *text, int num_chars)
{
    int index;
    int lnum_chars;
    twchar *ltext;

    if ((num_chars < 1) || (text == 0))
    {
        return 0;
    }

    lnum_chars = g_mbstowcs(0, text, num_chars);

    if (lnum_chars < 0)
    {
        return 0;
    }

    ltext = (twchar *) g_malloc((num_chars + 1) * sizeof(twchar), 1);
    g_mbstowcs(ltext, text, num_chars);
    index = 0;

    while (index < num_chars)
    {
        out_uint16_le(s, ltext[index]);
        index++;
    }

    g_free(ltext);
    return index * 2;
}

/*****************************************************************************/
/* returns number of bytes read */
int
clipboard_in_unicode(struct stream *s, char *text, int *num_chars)
{
    int index;
    twchar *ltext;
    twchar chr;

    if ((num_chars == 0) || (*num_chars < 1) || (text == 0))
    {
        return 0;
    }
    ltext = (twchar *) g_malloc(512 * sizeof(twchar), 1);
    index = 0;
    while (s_check_rem(s, 2))
    {
        in_uint16_le(s, chr);
        if (index < 511)
        {
            ltext[index] = chr;
        }
        index++;
        if (chr == 0)
        {
            break;
        }
    }
    *num_chars = g_wcstombs(text, ltext, *num_chars);
    g_free(ltext);
    return index * 2;
}

static char windows_native_format[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/*****************************************************************************/
static int
clipboard_send_format_announce(int xrdp_clip_type)
{
    struct stream *s;
    int size;
    int rv;
    char *holdp;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_format_announce:");
    make_stream(s);
    init_stream(s, 8192);
    out_uint16_le(s, CB_FORMAT_LIST); /* 2 CLIPRDR_FORMAT_ANNOUNCE */
    out_uint16_le(s, 0); /* status */
    holdp = s->p;
    out_uint32_le(s, 0); /* set later */
    if (g_cliprdr_flags & CB_USE_LONG_FORMAT_NAMES)
    {
        switch (xrdp_clip_type)
        {
            case XRDP_CB_FILE:
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_format_announce: XRDP_CB_FILE");
                /* canned response for "file" */
                out_uint32_le(s, 0x0000c0bc);
                clipboard_out_unicode(s, "FileGroupDescriptorW", 21);
                out_uint32_le(s, 0x0000c0ba);
                clipboard_out_unicode(s, "FileContents", 13);
                out_uint32_le(s, 0x0000c0c1);
                clipboard_out_unicode(s, "DropEffect", 11);
                break;
            case XRDP_CB_BITMAP:
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_format_announce: XRDP_CB_BITMAP");
                /* canned response for "bitmap" */
                out_uint32_le(s, 0x0000c004);
                clipboard_out_unicode(s, "Native", 7);
                out_uint32_le(s, 0x00000003);
                clipboard_out_unicode(s, "", 1);
                out_uint32_le(s, 0x00000008);
                clipboard_out_unicode(s, "", 1);
                out_uint32_le(s, 0x00000011);
                clipboard_out_unicode(s, "", 1);
                break;
            case XRDP_CB_TEXT:
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_format_announce: XRDP_CB_TEXT");
                /* canned response for "bitmap" */
                out_uint32_le(s, 0x0000000d);
                clipboard_out_unicode(s, "", 1);
                out_uint32_le(s, 0x00000010);
                clipboard_out_unicode(s, "", 1);
                out_uint32_le(s, 0x00000001);
                clipboard_out_unicode(s, "", 1);
                out_uint32_le(s, 0x00000007);
                clipboard_out_unicode(s, "", 1);
                break;
            default:
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_format_announce: unknown "
                          "xrdp_clip_type %d", xrdp_clip_type);
                break;
        }
    }
    else /* old method */
    {
        switch (xrdp_clip_type)
        {
            case XRDP_CB_FILE:
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_format_announce: XRDP_CB_FILE");
                /* canned response for "file" */
                out_uint32_le(s, 0x0000c0bc);
                out_uint8p(s, windows_native_format, sizeof(windows_native_format));
                out_uint32_le(s, 0x0000c0ba);
                out_uint8p(s, windows_native_format, sizeof(windows_native_format));
                out_uint32_le(s, 0x0000c0c1);
                out_uint8p(s, windows_native_format, sizeof(windows_native_format));
                break;
            case XRDP_CB_BITMAP:
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_format_announce: XRDP_CB_BITMAP");
                /* canned response for "bitmap" */
                out_uint32_le(s, 0x0000c004);
                out_uint8p(s, windows_native_format, sizeof(windows_native_format));
                out_uint32_le(s, 0x00000003);
                out_uint8p(s, windows_native_format, sizeof(windows_native_format));
                out_uint32_le(s, 0x00000008);
                out_uint8p(s, windows_native_format, sizeof(windows_native_format));
                out_uint32_le(s, 0x00000011);
                out_uint8p(s, windows_native_format, sizeof(windows_native_format));
                break;
            case XRDP_CB_TEXT:
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_format_announce: XRDP_CB_TEXT");
                /* canned response for "bitmap" */
                out_uint32_le(s, 0x0000000d);
                out_uint8p(s, windows_native_format, sizeof(windows_native_format));
                out_uint32_le(s, 0x00000010);
                out_uint8p(s, windows_native_format, sizeof(windows_native_format));
                out_uint32_le(s, 0x00000001);
                out_uint8p(s, windows_native_format, sizeof(windows_native_format));
                out_uint32_le(s, 0x00000007);
                out_uint8p(s, windows_native_format, sizeof(windows_native_format));
                break;
            default:
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_format_announce: unknown "
                          "xrdp_clip_type %d", xrdp_clip_type);
                break;
        }
    }
    size = (int)(s->p - holdp);
    size -= 4;
    holdp[0] = (size >> 0) & 0xff;
    holdp[1] = (size >> 8) & 0xff;
    holdp[2] = (size >> 16) & 0xff;
    holdp[3] = (size >> 24) & 0xff;
    out_uint32_le(s, 0);
    s_mark_end(s);
    size = (int)(s->end - s->data);
    LOG_DEVEL_HEXDUMP(LOG_LEVEL_TRACE, "clipboard data:", s->data, size);
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_format_announce: data out, sending "
              "CLIPRDR_FORMAT_ANNOUNCE (clip_msg_id = 2)");
    rv = send_channel_data(g_cliprdr_chan_id, s->data, size);
    free_stream(s);
    return rv;
}

/*****************************************************************************/
static int
clipboard_send_data_response_for_image(const char *data, int data_size)
{
    struct stream *s;
    int size;
    int rv;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_data_response_for_image: data_size %d",
              data_size);
    make_stream(s);
    init_stream(s, 64 + data_size);
    out_uint16_le(s, CB_FORMAT_DATA_RESPONSE); /* 5 CLIPRDR_DATA_RESPONSE */
    out_uint16_le(s, CB_RESPONSE_OK); /* 1 status */
    out_uint32_le(s, data_size); /* length */
    out_uint8p(s, data, data_size);
    out_uint32_le(s, 0);
    s_mark_end(s);
    size = (int)(s->end - s->data);
    rv = send_channel_data(g_cliprdr_chan_id, s->data, size);
    free_stream(s);
    return rv;
}

/*****************************************************************************/
static int
clipboard_send_data_response_for_text(const char *data, int data_size)
{
    struct stream *s;
    int size;
    int rv;
    int num_chars;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_data_response_for_text: data_size %d",
              data_size);
    LOG_DEVEL_HEXDUMP(LOG_LEVEL_TRACE, "clipboard send data response:", data, data_size);
    num_chars = g_mbstowcs(0, data, 0);
    if (num_chars < 0)
    {
        LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_send_data_response_for_text: "
                  "bad string");
        num_chars = 0;
    }
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_data_response_for_text: data_size %d "
              "num_chars %d", data_size, num_chars);
    make_stream(s);
    init_stream(s, 64 + num_chars * 2);
    out_uint16_le(s, CB_FORMAT_DATA_RESPONSE); /* 5 CLIPRDR_DATA_RESPONSE */
    out_uint16_le(s, CB_RESPONSE_OK); /* 1 status */
    out_uint32_le(s, num_chars * 2 + 2); /* length */
    if (clipboard_out_unicode(s, data, num_chars) != num_chars * 2)
    {
        LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_send_data_response_for_text: error "
                  "clipboard_out_unicode didn't write right number of bytes");
    }
    out_uint16_le(s, 0); /* nil for string */
    out_uint32_le(s, 0);
    s_mark_end(s);
    size = (int)(s->end - s->data);
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_data_response_for_text: data out, "
              "sending CLIPRDR_DATA_RESPONSE (clip_msg_id = 5) size %d "
              "num_chars %d", size, num_chars);
    rv = send_channel_data(g_cliprdr_chan_id, s->data, size);
    free_stream(s);
    return rv;
}

/*****************************************************************************/
static int
clipboard_send_data_response(int xrdp_clip_type, const char *data, int data_size)
{
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_data_response:");
    if (data != 0)
    {
        if (xrdp_clip_type == XRDP_CB_FILE)
        {
            return clipboard_send_data_response_for_file(data, data_size);
        }
        else if (xrdp_clip_type == XRDP_CB_BITMAP)
        {
            return clipboard_send_data_response_for_image(data, data_size);
        }
        else if (xrdp_clip_type == XRDP_CB_TEXT)
        {
            return clipboard_send_data_response_for_text(data, data_size);
        }
        else
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_send_data_response: unknown "
                      "xrdp_clip_type %d", xrdp_clip_type);
        }
    }
    else
    {
        LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_send_data_response: data is nil");
    }
    return 0;
}

/*****************************************************************************/
static int
clipboard_set_selection_owner(void)
{
    Window owner;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_set_selection_owner:");
    g_selection_time = clipboard_get_server_time();
    XSetSelectionOwner(g_display, g_clipboard_atom, g_wnd, g_selection_time);
    owner = XGetSelectionOwner(g_display, g_clipboard_atom);

    if (owner != g_wnd)
    {
        g_got_selection = 0;
        return 1;
    }

    g_got_selection = 1;
    return 0;
}

/*****************************************************************************/
static int
clipboard_provide_selection_c2s(XSelectionRequestEvent *req, Atom type)
{
    XEvent xev;
    long val1[2];

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_provide_selection_c2s: bytes %d",
              g_clip_c2s.total_bytes);
    if (g_clip_c2s.total_bytes < g_incr_max_req_size)
    {
        XChangeProperty(g_display, req->requestor, req->property,
                        type, 8, PropModeReplace,
                        (tui8 *)g_clip_c2s.data, g_clip_c2s.total_bytes);
        g_memset(&xev, 0, sizeof(xev));
        xev.xselection.type = SelectionNotify;
        xev.xselection.send_event = True;
        xev.xselection.display = req->display;
        xev.xselection.requestor = req->requestor;
        xev.xselection.selection = req->selection;
        xev.xselection.target = req->target;
        xev.xselection.property = req->property;
        xev.xselection.time = req->time;
        XSendEvent(g_display, req->requestor, False, NoEventMask, &xev);
    }
    else
    {
        /* start the INCR process */
        g_clip_c2s.incr_in_progress = 1;
        g_clip_c2s.incr_bytes_done = 0;
        g_clip_c2s.type = type;
        g_clip_c2s.property = req->property;
        g_clip_c2s.window = req->requestor;
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_provide_selection_c2s: start INCR property %s "
                  "type %s", get_atom_text(req->property),
                  get_atom_text(type));
        val1[0] = g_clip_c2s.total_bytes;
        val1[1] = 0;
        XChangeProperty(g_display, req->requestor, req->property,
                        g_incr_atom, 32, PropModeReplace, (tui8 *)val1, 1);
        /* we need events from that other window */
        XSelectInput(g_display, req->requestor, PropertyChangeMask);
        g_memset(&xev, 0, sizeof(xev));
        xev.xselection.type = SelectionNotify;
        xev.xselection.send_event = True;
        xev.xselection.display = req->display;
        xev.xselection.requestor = req->requestor;
        xev.xselection.selection = req->selection;
        xev.xselection.target = req->target;
        xev.xselection.property = req->property;
        xev.xselection.time = req->time;
        XSendEvent(g_display, req->requestor, False, NoEventMask, &xev);
    }
    return 0;
}

/*****************************************************************************/
static int
clipboard_provide_selection(XSelectionRequestEvent *req, Atom type, int format,
                            char *data, int length)
{
    XEvent xev;
    int bytes;

    bytes = FORMAT_TO_BYTES(format);
    bytes *= length;
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_provide_selection: bytes %d", bytes);
    if (bytes < g_incr_max_req_size)
    {
        XChangeProperty(g_display, req->requestor, req->property,
                        type, format, PropModeReplace, (tui8 *)data, length);
        g_memset(&xev, 0, sizeof(xev));
        xev.xselection.type = SelectionNotify;
        xev.xselection.send_event = True;
        xev.xselection.display = req->display;
        xev.xselection.requestor = req->requestor;
        xev.xselection.selection = req->selection;
        xev.xselection.target = req->target;
        xev.xselection.property = req->property;
        xev.xselection.time = req->time;
        XSendEvent(g_display, req->requestor, False, NoEventMask, &xev);
        return 0;
    }
    return 1;
}

/*****************************************************************************/
static int
clipboard_refuse_selection(XSelectionRequestEvent *req)
{
    XEvent xev;

    g_memset(&xev, 0, sizeof(xev));
    xev.xselection.type = SelectionNotify;
    xev.xselection.send_event = True;
    xev.xselection.display = req->display;
    xev.xselection.requestor = req->requestor;
    xev.xselection.selection = req->selection;
    xev.xselection.target = req->target;
    xev.xselection.property = None;
    xev.xselection.time = req->time;
    XSendEvent(g_display, req->requestor, False, NoEventMask, &xev);
    return 0;
}

/*****************************************************************************/
/* sent by client or server when its local system clipboard is
   updated with new clipboard data; contains Clipboard Format ID
   and name pairs of new Clipboard Formats on the clipboard. */
static int
clipboard_process_format_announce(struct stream *s, int clip_msg_status,
                                  int clip_msg_len)
{
    int formatId;
    int count;
    int bytes;
    char desc[256];
    char *holdp;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_format_announce: "
              "CLIPRDR_FORMAT_ANNOUNCE");
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_format_announce %d", clip_msg_len);
    clipboard_send_format_ack();

    xfuse_clear_clip_dir();
    g_clip_c2s.converted = 0;

    desc[0] = 0;
    g_num_formatIds = 0;
    while (clip_msg_len > 3)
    {
        in_uint32_le(s, formatId);
        clip_msg_len -= 4;
        if (g_cliprdr_flags & CB_USE_LONG_FORMAT_NAMES)
        {
            /* CLIPRDR_LONG_FORMAT_NAME */
            count = 255;
            bytes = clipboard_in_unicode(s, desc, &count);
            clip_msg_len -= bytes;
        }
        else
        {
            /* CLIPRDR_SHORT_FORMAT_NAME */
            /* 32 ASCII 8 characters or 16 Unicode characters */
            count = 15;
            holdp = s->p;
            clipboard_in_unicode(s, desc, &count);
            s->p = holdp + 32;
            desc[15] = 0;
            clip_msg_len -= 32;
        }
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_format_announce: formatId 0x%8.8x "
                  "wszFormatName [%s] clip_msg_len %d", formatId, desc,
                  clip_msg_len);
        if (g_num_formatIds <= 15)
        {
            g_formatIds[g_num_formatIds] = formatId;
            g_num_formatIds++;
        }
        if (g_num_formatIds > 15)
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_format_announce: max formats");
        }

        /* format id for file copy copy seems to keep changing */
        /* seen 0x0000c0c8, 0x0000c0ed */
        if (g_strcmp(desc, "FileGroupDescriptorW") == 0)
        {
            g_file_format_id = formatId;
        }
    }

    if ((g_num_formatIds > 0) &&
            (g_clip_c2s.incr_in_progress == 0) && /* don't interrupt incr */
            (g_clip_s2c.incr_in_progress == 0))
    {
        if (clipboard_set_selection_owner() != 0)
        {
            LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_process_format_announce: "
                      "XSetSelectionOwner failed");
        }
    }

    return 0;
}

/*****************************************************************************/
/* response to CB_FORMAT_LIST; used to indicate whether
   processing of the Format List PDU was successful */
static int
clipboard_process_format_ack(struct stream *s, int clip_msg_status,
                             int clip_msg_len)
{
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_format_ack: CLIPRDR_FORMAT_ACK");
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_format_ack:");
    return 0;
}

/*****************************************************************************/
static int
clipboard_send_data_response_failed(void)
{
    struct stream *s;
    int size;
    int rv;

    LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_send_data_response_failed:");
    make_stream(s);
    init_stream(s, 64);
    out_uint16_le(s, CB_FORMAT_DATA_RESPONSE); /* 5 CLIPRDR_DATA_RESPONSE */
    out_uint16_le(s, CB_RESPONSE_FAIL); /* 2 status */
    out_uint32_le(s, 0);
    s_mark_end(s);
    size = (int)(s->end - s->data);
    rv = send_channel_data(g_cliprdr_chan_id, s->data, size);
    free_stream(s);
    return rv;
}

/*****************************************************************************/
/* sent from server to client
 * sent by recipient of CB_FORMAT_LIST; used to request data for one
 * of the formats that was listed in CB_FORMAT_LIST */
static int
clipboard_process_data_request(struct stream *s, int clip_msg_status,
                               int clip_msg_len)
{
    int requestedFormatId;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_request: "
              "CLIPRDR_DATA_REQUEST");
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_request:");
    LOG_DEVEL(LOG_LEVEL_DEBUG, "  %d", g_clip_s2c.xrdp_clip_type);
    in_uint32_le(s, requestedFormatId);
    switch (requestedFormatId)
    {
        case CB_FORMAT_FILE: /* 0xC0BC */
            if ((g_clip_s2c.xrdp_clip_type == XRDP_CB_FILE) && g_clip_s2c.converted)
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_request: CB_FORMAT_FILE");
                clipboard_send_data_response(XRDP_CB_FILE, g_clip_s2c.data,
                                             g_clip_s2c.total_bytes);
            }
            else
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_request: CB_FORMAT_FILE, "
                          "calling XConvertSelection to g_utf8_atom");
                g_clip_s2c.xrdp_clip_type = XRDP_CB_FILE;
                XConvertSelection(g_display, g_clipboard_atom, g_clip_s2c.type,
                                  g_clip_property_atom, g_wnd, CurrentTime);
            }
            break;
        case CB_FORMAT_DIB: /* 0x0008 */
            if ((g_clip_s2c.xrdp_clip_type == XRDP_CB_BITMAP) && g_clip_s2c.converted)
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_request: CB_FORMAT_DIB");
                clipboard_send_data_response(XRDP_CB_BITMAP, g_clip_s2c.data,
                                             g_clip_s2c.total_bytes);
            }
            else
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_request: CB_FORMAT_DIB, "
                          "calling XConvertSelection to g_image_bmp_atom");
                g_clip_s2c.xrdp_clip_type = XRDP_CB_BITMAP;
                XConvertSelection(g_display, g_clipboard_atom, g_image_bmp_atom,
                                  g_clip_property_atom, g_wnd, CurrentTime);
            }
            break;
        case CB_FORMAT_UNICODETEXT: /* 0x000D */
            if ((g_clip_s2c.xrdp_clip_type == XRDP_CB_TEXT) && g_clip_s2c.converted)
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_request: CB_FORMAT_UNICODETEXT");
                clipboard_send_data_response(XRDP_CB_TEXT, g_clip_s2c.data,
                                             g_clip_s2c.total_bytes);
            }
            else
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_request: CB_FORMAT_UNICODETEXT, "
                          "calling XConvertSelection to g_utf8_atom");
                g_clip_s2c.xrdp_clip_type = XRDP_CB_TEXT;
                XConvertSelection(g_display, g_clipboard_atom, g_utf8_atom,
                                  g_clip_property_atom, g_wnd, CurrentTime);
            }
            break;
        default:
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_request: unknown type %d",
                      requestedFormatId);
            clipboard_send_data_response_failed();
            break;
    }
    return 0;
}

/*****************************************************************************/
/* client to server */
/* sent as a reply to CB_FORMAT_DATA_REQUEST; used to indicate whether
   processing of the CB_FORMAT_DATA_REQUEST was successful; if processing
   was successful, CB_FORMAT_DATA_RESPONSE includes contents of requested
   clipboard data. */
static int
clipboard_process_data_response_for_image(struct stream *s,
        int clip_msg_status,
        int clip_msg_len)
{
    XSelectionRequestEvent *lxev;
    int len;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_response_for_image: "
              "CLIPRDR_DATA_RESPONSE_FOR_IMAGE");
    lxev = &g_saved_selection_req_event;
    len = (int)(s->end - s->p);
    if (len < 1)
    {
        return 0;
    }
    if (g_clip_c2s.type != g_image_bmp_atom)
    {
        return 0;
    }
    g_free(g_clip_c2s.data);
    g_clip_c2s.data = (char *) g_malloc(len + 14, 0);
    if (g_clip_c2s.data == 0)
    {
        g_clip_c2s.total_bytes = 0;
        return 0;
    }
    g_clip_c2s.total_bytes = len;
    g_clip_c2s.read_bytes_done = g_clip_c2s.total_bytes;
    g_memcpy(g_clip_c2s.data, g_bmp_image_header, 14);
    in_uint8a(s, g_clip_c2s.data + 14, len);
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_response_for_image: calling "
              "clipboard_provide_selection_c2s");
    clipboard_provide_selection_c2s(lxev, lxev->target);
    return 0;
}

/**************************************************************************//**
 * Process a CB_FORMAT_DATA_RESPONSE for an X client requesting a file list
 *
 * @param s Stream containing CLIPRDR_FILELIST ([MS-RDPECLIP])
 * @param clip_msg_status msgFlags from Clipboard PDU Header
 * @param clip_msg_len dataLen from Clipboard PDU Header
 *
 * @return Status
 */
static int
clipboard_process_data_response_for_file(struct stream *s,
        int clip_msg_status,
        int clip_msg_len)
{
    XSelectionRequestEvent *lxev;
    int rv = 0;

    LOG_DEVEL(LOG_LEVEL_TRACE, "clipboard_process_data_response_for_file: ");
    lxev = &g_saved_selection_req_event;

    const int flist_size = 1024 * 1024;
    g_free(g_clip_c2s.data);
    g_clip_c2s.data = (char *)g_malloc(flist_size, 0);
    if (g_clip_c2s.data == NULL)
    {
        LOG(LOG_LEVEL_ERROR, "clipboard_process_data_response_for_file: "
            "Can't allocate memory");
        rv = 1;
    }
    /* text/uri-list */
    else if (g_clip_c2s.type == g_file_atom1)
    {
        rv = clipboard_c2s_in_files(s, g_clip_c2s.data, flist_size,
                                    "file://");
    }
    /* x-special/gnome-copied-files */
    else if (g_clip_c2s.type == g_file_atom2)
    {
        g_strcpy(g_clip_c2s.data, "copy\n");
        rv = clipboard_c2s_in_files(s, g_clip_c2s.data + 5, flist_size - 5,
                                    "file://");
    }
    else if ((g_clip_c2s.type == XA_STRING) ||
             (g_clip_c2s.type == g_utf8_atom))
    {
        if (g_cfg->use_nautilus3_flist_format)
        {
            /*
             * This file list format is only used by GNOME 3
             * versions >= 3.29.92. It is not used by GNOME 4. Remove
             * this workaround when GNOME 3 is no longer supported by
             * long-term distros */
#define LIST_PREFIX "x-special/nautilus-clipboard\ncopy\n"
#define LIST_PREFIX_LEN (sizeof(LIST_PREFIX) - 1)
            g_strcpy(g_clip_c2s.data, LIST_PREFIX);
            rv = clipboard_c2s_in_files(s,
                                        g_clip_c2s.data + LIST_PREFIX_LEN,
                                        flist_size - LIST_PREFIX_LEN - 1,
                                        "file://");
            g_strcat(g_clip_c2s.data, "\n");
#undef LIST_PREFIX_LEN
#undef LIST_PREFIX
        }
        else
        {
            rv = clipboard_c2s_in_files(s, g_clip_c2s.data, flist_size, "");
        }
    }
    else
    {
        LOG_DEVEL(LOG_LEVEL_ERROR,
                  "clipboard_process_data_response_for_file: "
                  "Unrecognised target");
        rv = 1;
    }

    if (rv != 0 && g_clip_c2s.data != NULL)
    {
        g_clip_c2s.data[0] = '\0';
    }

    g_clip_c2s.total_bytes =
        (g_clip_c2s.data == NULL) ? 0 : g_strlen(g_clip_c2s.data);
    g_clip_c2s.read_bytes_done = g_clip_c2s.total_bytes;
    clipboard_provide_selection_c2s(lxev, lxev->target);

    return rv;
}

/*****************************************************************************/
/* client to server */
/* sent as a reply to CB_FORMAT_DATA_REQUEST; used to indicate whether
   processing of the CB_FORMAT_DATA_REQUEST was successful; if processing was
   successful, CB_FORMAT_DATA_RESPONSE includes contents of requested
   clipboard data. */
/*****************************************************************************/
static int
clipboard_process_data_response(struct stream *s, int clip_msg_status,
                                int clip_msg_len)
{
    XSelectionRequestEvent *lxev;
    twchar *wtext;
    twchar wchr;
    int len;
    int index;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_response:");
    lxev = &g_saved_selection_req_event;
    g_clip_c2s.in_request = 0;
    if (g_clip_c2s.xrdp_clip_type == XRDP_CB_BITMAP)
    {
        clipboard_process_data_response_for_image(s, clip_msg_status,
                clip_msg_len);
        return 0;
    }
    if (g_clip_c2s.xrdp_clip_type == XRDP_CB_FILE)
    {
        clipboard_process_data_response_for_file(s, clip_msg_status,
                clip_msg_len);
        return 0;
    }
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_data_response: "
              "CLIPRDR_DATA_RESPONSE");
    len = (int)(s->end - s->p);
    if (len < 1)
    {
        return 0;
    }
    wtext = (twchar *) g_malloc(((len / 2) + 1) * sizeof(twchar), 0);
    if (wtext == 0)
    {
        return 0;
    }
    index = 0;
    while (s_check_rem(s, 2))
    {
        in_uint16_le(s, wchr);
        wtext[index] = wchr;
        if (wchr == 0)
        {
            break;
        }
        index++;
    }
    wtext[index] = 0;
    g_free(g_clip_c2s.data);
    g_clip_c2s.data = 0;
    g_clip_c2s.total_bytes = 0;
    len = g_wcstombs(0, wtext, 0);
    if (len >= 0)
    {
        g_clip_c2s.data = (char *) g_malloc(len + 16, 0);
        if (g_clip_c2s.data == 0)
        {
            g_free(wtext);
            return 0;
        }
        g_wcstombs(g_clip_c2s.data, wtext, len + 1);
    }
    if (g_clip_c2s.data != 0)
    {
        g_clip_c2s.total_bytes = g_strlen(g_clip_c2s.data);
        g_clip_c2s.read_bytes_done = g_clip_c2s.total_bytes;
        clipboard_provide_selection_c2s(lxev, lxev->target);
    }
    g_free(wtext);
    return 0;
}

/*****************************************************************************/
static int
clipboard_process_clip_caps(struct stream *s, int clip_msg_status,
                            int clip_msg_len)
{
    int cCapabilitiesSets;
    int capabilitySetType;
    int lengthCapability;
    int index;
    int version;
    int flags;
    char *holdp;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_clip_caps:");
    //LOG_DEVEL_HEXDUMP(LOG_LEVEL_TRACE, "", s->p, s->end - s->p);
    in_uint16_le(s, cCapabilitiesSets);
    in_uint8s(s, 2); /* pad */
    for (index = 0; index < cCapabilitiesSets; index++)
    {
        holdp = s->p;
        in_uint16_le(s, capabilitySetType);
        in_uint16_le(s, lengthCapability);
        switch (capabilitySetType)
        {
            case CB_CAPSTYPE_GENERAL:
                in_uint32_le(s, version); /* version */
                in_uint32_le(s, flags); /* generalFlags */
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_clip_caps: "
                          "g_cliprdr_version %d version %d "
                          "g_cliprdr_flags 0x%x flags 0x%x",
                          g_cliprdr_version, version,
                          g_cliprdr_flags, flags);
                if (version < g_cliprdr_version)
                {
                    g_cliprdr_version = version;
                }
                g_cliprdr_flags &= flags;
                break;
            default:
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_process_clip_caps: unknown "
                          "capabilitySetType %d", capabilitySetType);
                break;
        }
        s->p = holdp + lengthCapability;
    }
    return 0;
}

/*****************************************************************************/
static int
ss_part(char *data, int data_bytes)
{
    int index;
    char *text;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "ss_part: data_bytes %d read_bytes_done %d "
              "incr_bytes_done %d", data_bytes,
              g_clip_c2s.read_bytes_done,
              g_clip_c2s.incr_bytes_done);
    /* copy to buffer */
    if (g_clip_c2s.type == g_utf8_atom)
    {
        /* todo unicode */
        text = (char *)g_malloc(data_bytes, 0);
        index = 0;
        data_bytes /= 2;
        while (index < data_bytes)
        {
            text[index] = data[index * 2];
            index++;
        }
        text[index] = 0;
        g_memcpy(g_clip_c2s.data + g_clip_c2s.read_bytes_done, text, data_bytes);
        g_clip_c2s.read_bytes_done += data_bytes;
        g_free(text);
    }
    else
    {
        g_memcpy(g_clip_c2s.data + g_clip_c2s.read_bytes_done, data, data_bytes);
        g_clip_c2s.read_bytes_done += data_bytes;
    }
    if (g_clip_c2s.incr_in_progress)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "ss_part: incr_in_progress set");
        return 0;
    }
    if (g_clip_c2s.read_bytes_done <= g_clip_c2s.incr_bytes_done)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "ss_part: read_bytes_done < incr_bytes_done");
        return 0;
    }
    data = g_clip_c2s.data + g_clip_c2s.incr_bytes_done;
    data_bytes = g_clip_c2s.read_bytes_done - g_clip_c2s.incr_bytes_done;
    if (data_bytes > g_incr_max_req_size)
    {
        data_bytes = g_incr_max_req_size;
    }
    g_clip_c2s.incr_bytes_done += data_bytes;
    XChangeProperty(g_display, g_clip_c2s.window,
                    g_clip_c2s.property, g_clip_c2s.type, 8,
                    PropModeReplace, (tui8 *)data, data_bytes);
    g_clip_c2s.incr_in_progress = 1;
    return 0;
}

/*****************************************************************************/
static int
ss_end(void)
{
    char *data;
    int data_bytes;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "ss_end:");
    g_clip_c2s.doing_response_ss = 0;
    g_clip_c2s.in_request = 0;

    if (g_clip_c2s.incr_in_progress)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "ss_end: incr_in_progress set");
        return 0;
    }
    if (g_clip_c2s.read_bytes_done <= g_clip_c2s.incr_bytes_done)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "ss_end: read_bytes_done < incr_bytes_done");
        return 0;
    }
    data = g_clip_c2s.data + g_clip_c2s.incr_bytes_done;
    data_bytes = g_clip_c2s.read_bytes_done - g_clip_c2s.incr_bytes_done;
    if (data_bytes > g_incr_max_req_size)
    {
        data_bytes = g_incr_max_req_size;
    }
    g_clip_c2s.incr_bytes_done += data_bytes;
    XChangeProperty(g_display, g_clip_c2s.window,
                    g_clip_c2s.property, g_clip_c2s.type, 8,
                    PropModeReplace, (tui8 *)data, data_bytes);
    g_clip_c2s.incr_in_progress = 1;
    return 0;
}

/*****************************************************************************/
static int
ss_start(char *data, int data_bytes, int total_bytes)
{
    XEvent xev;
    XSelectionRequestEvent *req;
    long val1[2];
    int incr_bytes;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "ss_start: data_bytes %d total_bytes %d",
              data_bytes, total_bytes);
    req = &g_saved_selection_req_event;

    incr_bytes = total_bytes;
    if (req->target == g_image_bmp_atom)
    {
        incr_bytes += 14;
    }
    else if (req->target == g_utf8_atom)
    {
        incr_bytes /= 2;
    }
    val1[0] = incr_bytes; /* a guess */
    val1[1] = 0;

    g_clip_c2s.doing_response_ss = 1;
    g_clip_c2s.incr_bytes_done = 0;
    g_clip_c2s.read_bytes_done = 0;
    g_clip_c2s.type = req->target;
    g_clip_c2s.property = req->property;
    g_clip_c2s.window = req->requestor;
    g_free(g_clip_c2s.data);
    g_clip_c2s.data = (char *)g_malloc(incr_bytes + 64, 0);
    g_clip_c2s.total_bytes = incr_bytes;

    XChangeProperty(g_display, req->requestor, req->property,
                    g_incr_atom, 32, PropModeReplace, (tui8 *)val1, 1);
    /* we need events from that other window */
    XSelectInput(g_display, req->requestor, PropertyChangeMask);
    g_memset(&xev, 0, sizeof(xev));
    xev.xselection.type = SelectionNotify;
    xev.xselection.send_event = True;
    xev.xselection.display = req->display;
    xev.xselection.requestor = req->requestor;
    xev.xselection.selection = req->selection;
    xev.xselection.target = req->target;
    xev.xselection.property = req->property;
    xev.xselection.time = req->time;
    XSendEvent(g_display, req->requestor, False, NoEventMask, &xev);

    if (req->target == g_image_bmp_atom)
    {
        g_memcpy(g_clip_c2s.data, g_bmp_image_header, 14);
        g_clip_c2s.read_bytes_done = 14;
    }

    g_clip_c2s.incr_in_progress = 1;

    ss_part(data, data_bytes);

    return 0;
}

/*****************************************************************************/
int
clipboard_data_in(struct stream *s, int chan_id, int chan_flags, int length,
                  int total_length)
{
    int clip_msg_id;
    int clip_msg_len;
    int clip_msg_status;
    int rv;
    struct stream *ls;
    char *holdp;

    if (!g_clip_up)
    {
        LOG_DEVEL(LOG_LEVEL_ERROR, "aborting clipboard_data_in - clipboard has not "
                  "been initialized");
        /* we return 0 here to indicate no protocol problem occurred */
        return 0;
    }

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_data_in: chan_id %d "
              "chan_flags 0x%x length %d total_length %d "
              "in_request %d g_ins->size %d",
              chan_id, chan_flags, length, total_length,
              g_clip_c2s.in_request, g_ins->size);

    if (g_clip_c2s.doing_response_ss)
    {
        ss_part(s->p, length);
        if ((chan_flags & 3) == 2)
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_data_in: calling ss_end");
            ss_end();
        }
        return 0;
    }

    if (g_clip_c2s.in_request)
    {
        if (total_length > 32 * 1024)
        {
            if ((chan_flags & 3) == 1)
            {
                holdp = s->p;
                in_uint16_le(s, clip_msg_id);
                in_uint16_le(s, clip_msg_status);
                in_uint32_le(s, clip_msg_len);
                if (clip_msg_id == CB_FORMAT_DATA_RESPONSE)
                {
                    ss_start(s->p, length - 8, total_length - 8);
                    return 0;
                }
                s->p = holdp;
            }
        }
    }

    if ((chan_flags & 3) == 3)
    {
        ls = s;
    }
    else
    {
        if (chan_flags & 1)
        {
            init_stream(g_ins, total_length);
        }

        in_uint8a(s, g_ins->end, length);
        g_ins->end += length;

        if ((chan_flags & 2) == 0)
        {
            return 0;
        }

        ls = g_ins;
    }

    in_uint16_le(ls, clip_msg_id);
    in_uint16_le(ls, clip_msg_status);
    in_uint32_le(ls, clip_msg_len);

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_data_in: clip_msg_id %d "
              "clip_msg_status %d clip_msg_len %d",
              clip_msg_id, clip_msg_status, clip_msg_len);
    rv = 0;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_data_in: %d", clip_msg_id);
    switch (clip_msg_id)
    {
        /* sent by client or server when its local system clipboard is   */
        /* updated with new clipboard data; contains Clipboard Format ID */
        /* and name pairs of new Clipboard Formats on the clipboard.     */
        case CB_FORMAT_LIST: /* 2 CLIPRDR_FORMAT_ANNOUNCE */
            rv = clipboard_process_format_announce(ls, clip_msg_status,
                                                   clip_msg_len);
            break;
        /* response to CB_FORMAT_LIST; used to indicate whether */
        /* processing of the Format List PDU was successful     */
        case CB_FORMAT_LIST_RESPONSE: /* 3 CLIPRDR_FORMAT_ACK */
            rv = clipboard_process_format_ack(ls, clip_msg_status,
                                              clip_msg_len);
            break;
        /* sent by recipient of CB_FORMAT_LIST; used to request data for one */
        /* of the formats that was listed in CB_FORMAT_LIST                  */
        case CB_FORMAT_DATA_REQUEST: /* 4 CLIPRDR_DATA_REQUEST */
            rv = clipboard_process_data_request(ls, clip_msg_status,
                                                clip_msg_len);
            break;
        /* sent as a reply to CB_FORMAT_DATA_REQUEST; used to indicate */
        /* whether processing of the CB_FORMAT_DATA_REQUEST was        */
        /* successful; if processing was successful,                   */
        /* CB_FORMAT_DATA_RESPONSE includes contents of requested      */
        /* clipboard data.                                             */
        case CB_FORMAT_DATA_RESPONSE: /* 5 CLIPRDR_DATA_RESPONSE */
            rv = clipboard_process_data_response(ls, clip_msg_status,
                                                 clip_msg_len);
            break;
        case CB_CLIP_CAPS: /* 7 */
            rv = clipboard_process_clip_caps(ls, clip_msg_status,
                                             clip_msg_len);
            break;
        case CB_FILECONTENTS_REQUEST: /* 8 */
            rv = clipboard_process_file_request(ls, clip_msg_status,
                                                clip_msg_len);
            break;
        case CB_FILECONTENTS_RESPONSE: /* 9 */
            rv = clipboard_process_file_response(ls, clip_msg_status,
                                                 clip_msg_len);
            break;
        default:
            LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_data_in: unknown clip_msg_id %d", clip_msg_id);
            break;
    }

    XFlush(g_display);
    return rv;
}

/*****************************************************************************/
/* this happens when a new app copies something to the clipboard
   'CLIPBOARD' Atom
   typedef struct
   {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    int subtype;
    Window owner;
    Atom selection;
    Time timestamp;
    Time selection_timestamp;
   } XFixesSelectionNotifyEvent; */
static int
clipboard_event_selection_owner_notify(XEvent *xevent)
{
    XFixesSelectionNotifyEvent *lxevent;

    lxevent = (XFixesSelectionNotifyEvent *)xevent;
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_owner_notify: 0x%lx", lxevent->owner);
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_owner_notify: "
              "window %ld subtype %d owner %ld g_wnd %ld",
              lxevent->window, lxevent->subtype, lxevent->owner, g_wnd);

    if (lxevent->owner == g_wnd)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_owner_notify: matches g_wnd");
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_owner_notify: skipping, "
                  "owner == g_wnd");
        g_got_selection = 1;
        return 0;
    }

    g_got_selection = 0;
    if (lxevent->owner != 0) /* nil owner comes when selection */
    {
        /* window is closed */
        XConvertSelection(g_display, g_clipboard_atom, g_targets_atom,
                          g_clip_property_atom, g_wnd, lxevent->timestamp);
    }
    return 0;
}

/*****************************************************************************/
/* returns error
   get a window property from wnd */
static int
clipboard_get_window_property(Window wnd, Atom prop, Atom *type, int *fmt,
                              int *n_items, char **xdata, int *xdata_size)
{
    int lfmt;
    int lxdata_size;
    unsigned long ln_items;
    unsigned long llen_after;
    tui8 *lxdata;
    Atom ltype;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_get_window_property:");
    LOG_DEVEL(LOG_LEVEL_DEBUG, "  prop %ld name %s", prop, get_atom_text(prop));
    lxdata = 0;
    ltype = 0;
    XGetWindowProperty(g_display, wnd, prop, 0, 0, 0,
                       AnyPropertyType, &ltype, &lfmt, &ln_items,
                       &llen_after, &lxdata);
    if (lxdata != 0)
    {
        XFree(lxdata);
    }
    if (ltype == 0)
    {
        /* XGetWindowProperty failed */
        return 4;
    }
    if (llen_after < 1)
    {
        /* no data, ok */
        return 0;
    }
    lxdata = 0;
    ltype = 0;
    XGetWindowProperty(g_display, wnd, prop, 0, (llen_after + 3) / 4, 0,
                       AnyPropertyType, &ltype, &lfmt, &ln_items,
                       &llen_after, &lxdata);
    if (ltype == 0)
    {
        /* XGetWindowProperty failed */
        if (lxdata != 0)
        {
            XFree(lxdata);
        }
        return 1;
    }
    lxdata_size = FORMAT_TO_BYTES(lfmt);
    lxdata_size *= ln_items;
    if (lxdata_size < 1)
    {
        /* should not happen */
        if (lxdata != 0)
        {
            XFree(lxdata);
        }
        return 2;
    }
    if (llen_after > 0)
    {
        /* should not happen */
        if (lxdata != 0)
        {
            XFree(lxdata);
        }
        return 3;
    }
    if (xdata != 0)
    {
        *xdata = (char *) g_malloc(lxdata_size, 0);
        g_memcpy(*xdata, lxdata, lxdata_size);
    }
    if (lxdata != 0)
    {
        XFree(lxdata);
    }
    if (xdata_size != 0)
    {
        *xdata_size = lxdata_size;
    }
    if (fmt != 0)
    {
        *fmt = (int)lfmt;
    }
    if (n_items != 0)
    {
        *n_items = (int)ln_items;
    }
    if (type != 0)
    {
        *type = ltype;
    }
    return 0;
}

/*****************************************************************************/
/* returns error
   process the SelectionNotify X event, uses XSelectionEvent
   typedef struct {
     int type;             // SelectionNotify
     unsigned long serial; // # of last request processed by server
     Bool send_event;      // true if this came from a SendEvent request
     Display *display;     // Display the event was read from
     Window requestor;
     Atom selection;
     Atom target;
     Atom property;        // atom or None
     Time time;
   } XSelectionEvent; */
static int
clipboard_event_selection_notify(XEvent *xevent)
{
    XSelectionEvent *lxevent;
    char *data;
    int data_size;
    int n_items;
    int fmt;
    int rv;
    int index;
    int got_string;
    int got_utf8;
    int got_bmp_image;
    int send_format_announce;
    Atom got_file_atom;
    Atom atom;
    Atom *atoms;
    Atom type;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_notify:");
    data_size = 0;
    n_items = 0;
    fmt = 0;
    got_string = 0;
    got_utf8 = 0;
    got_bmp_image = 0;
    got_file_atom = 0;
    send_format_announce = 0;
    rv = 0;
    data = 0;
    type = 0;
    lxevent = (XSelectionEvent *)xevent;

    if (lxevent->property == None)
    {
        LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_event_selection_notify: clip could "
                  "not be converted");
        rv = 1;
    }

    if (rv == 0)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_notify: wnd 0x%lx prop %s",
                  lxevent->requestor,
                  get_atom_text(lxevent->property));
        rv = clipboard_get_window_property(lxevent->requestor, lxevent->property,
                                           &type, &fmt,
                                           &n_items, &data, &data_size);
        if (rv != 0)
        {
            LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_event_selection_notify: "
                      "clipboard_get_window_property failed error %d", rv);
            return 0;
        }
        //LOG_DEVEL_HEXDUMP(LOG_LEVEL_TRACE, "", data, data_size);
        XDeleteProperty(g_display, lxevent->requestor, lxevent->property);
        if (type == g_incr_atom)
        {
            /* nothing more to do here, the data is coming in through
               PropertyNotify */
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_notify: type is INCR "
                      "data_size %d property name %s type %s", data_size,
                      get_atom_text(lxevent->property),
                      get_atom_text(lxevent->type));
            g_clip_s2c.incr_in_progress = 1;
            g_clip_s2c.property = lxevent->property;
            g_clip_s2c.type = lxevent->target;
            g_clip_s2c.total_bytes = 0;
            g_free(g_clip_s2c.data);
            g_clip_s2c.data = 0;
            //LOG_DEVEL_HEXDUMP(LOG_LEVEL_TRACE, "", data, sizeof(long));
            g_free(data);
            return 0;
        }
    }

    if (rv == 0)
    {
        if (lxevent->selection == g_clipboard_atom)
        {
            if (lxevent->target == g_targets_atom)
            {
                /* 32 implies long */
                if ((type == XA_ATOM) && (fmt == 32))
                {
                    atoms = (Atom *)data;
                    for (index = 0; index < n_items; index++)
                    {
                        atom = atoms[index];
                        LOG_DEVEL(LOG_LEVEL_DEBUG,
                                  "clipboard_event_selection_notify: 0x%lx %s 0x%lx",
                                  atom, get_atom_text(atom), XA_STRING);
                        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_notify: 0x%lx %s",
                                  atom, get_atom_text(atom));
                        if (atom == g_utf8_atom)
                        {
                            got_utf8 = 1;
                        }
                        else if (atom == XA_STRING)
                        {
                            got_string = 1;
                        }
                        else if (atom == g_image_bmp_atom)
                        {
                            got_bmp_image = 1;
                        }
                        else if ((atom == g_file_atom1) || (atom == g_file_atom2))
                        {
                            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_notify: file");
                            got_file_atom = atom;
                        }
                        else
                        {
                            LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_event_selection_notify: unknown atom 0x%lx", atom);
                        }
                    }
                }
                else
                {
                    LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_event_selection_notify: error, "
                              "target is 'TARGETS' and type[%ld] or fmt[%d] not right, "
                              "should be type[%ld], fmt[%d]", type, fmt, XA_ATOM, 32);
                }
            }
            else if (lxevent->target == g_utf8_atom)
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_notify: UTF8_STRING "
                          "data_size %d", data_size);
                if ((g_clip_s2c.incr_in_progress == 0) && (data_size > 0))
                {
                    g_free(g_clip_s2c.data);
                    g_clip_s2c.total_bytes = data_size;
                    g_clip_s2c.data = (char *) g_malloc(g_clip_s2c.total_bytes + 1, 0);
                    g_memcpy(g_clip_s2c.data, data, g_clip_s2c.total_bytes);
                    g_clip_s2c.data[g_clip_s2c.total_bytes] = 0;
                    if (g_clip_s2c.xrdp_clip_type == XRDP_CB_FILE)
                    {
                        if (g_cfg->restrict_outbound_clipboard & CLIP_RESTRICT_FILE)
                        {
                            LOG(LOG_LEVEL_DEBUG,
                                "outbound clipboard(file) UTF8_STRING(%s) is restricted because of config",
                                g_clip_s2c.data);
                        }
                        else
                        {
                            clipboard_send_data_response_for_file(g_clip_s2c.data,
                                                                  g_clip_s2c.total_bytes);
                        }
                    }
                    else
                    {
                        if (g_cfg->restrict_outbound_clipboard & CLIP_RESTRICT_TEXT)
                        {
                            LOG(LOG_LEVEL_DEBUG,
                                "outbound clipboard(text) UTF8_STRING(%s) is restricted because of config",
                                g_clip_s2c.data);
                        }
                        else
                        {
                            clipboard_send_data_response_for_text(g_clip_s2c.data,
                                                                  g_clip_s2c.total_bytes);
                        }
                    }

                }
            }
            else if (lxevent->target == XA_STRING)
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_notify: XA_STRING "
                          "data_size %d", data_size);
                if ((g_clip_s2c.incr_in_progress == 0) && (data_size > 0))
                {
                    g_free(g_clip_s2c.data);
                    g_clip_s2c.total_bytes = data_size;
                    g_clip_s2c.data = (char *) g_malloc(g_clip_s2c.total_bytes + 1, 0);
                    g_memcpy(g_clip_s2c.data, data, g_clip_s2c.total_bytes);
                    g_clip_s2c.data[g_clip_s2c.total_bytes] = 0;
                    if (g_cfg->restrict_outbound_clipboard & CLIP_RESTRICT_TEXT)
                    {
                        LOG(LOG_LEVEL_DEBUG,
                            "outbound clipboard(text) XA_STRING(%s) is restricted because of config",
                            g_clip_s2c.data);
                    }
                    else
                    {
                        clipboard_send_data_response_for_text(g_clip_s2c.data,
                                                              g_clip_s2c.total_bytes);
                    }

                }
            }
            else if (lxevent->target == g_image_bmp_atom)
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_notify: image/bmp "
                          "data_size %d", data_size);
                if ((g_clip_s2c.incr_in_progress == 0) && (data_size > 14))
                {
                    g_free(g_clip_s2c.data);

                    if (g_cfg->restrict_outbound_clipboard & CLIP_RESTRICT_IMAGE)
                    {
                        LOG(LOG_LEVEL_DEBUG,
                            "outbound clipboard(image) image/bmp is restricted because of config");
                    }
                    else
                    {
                        g_clip_s2c.total_bytes = data_size;
                        g_clip_s2c.data = (char *) g_malloc(data_size, 0);
                        g_memcpy(g_clip_s2c.data, data, data_size);
                        clipboard_send_data_response_for_image(g_clip_s2c.data + 14,
                                                               data_size - 14);
                    }

                }
            }
            else if (lxevent->target == g_file_atom1)
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_notify: text/uri-list "
                          "data_size %d", data_size);
                if ((g_clip_s2c.incr_in_progress == 0) && (data_size > 0))
                {
                    g_free(g_clip_s2c.data);
                    g_clip_s2c.total_bytes = data_size;
                    g_clip_s2c.data = (char *) g_malloc(g_clip_s2c.total_bytes + 1, 0);
                    g_memcpy(g_clip_s2c.data, data, g_clip_s2c.total_bytes);
                    g_clip_s2c.data[g_clip_s2c.total_bytes] = 0;
                    if (g_cfg->restrict_outbound_clipboard & CLIP_RESTRICT_FILE)
                    {
                        LOG(LOG_LEVEL_DEBUG,
                            "outbound clipboard(file) text/uri-list(%s) is restricted because of config",
                            g_clip_s2c.data);
                    }
                    else
                    {
                        clipboard_send_data_response_for_file(g_clip_s2c.data,
                                                              g_clip_s2c.total_bytes);

                    }

                }
            }
            else if (lxevent->target == g_file_atom2)
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_notify: x-special/gnome-copied-files "
                          "data_size %d", data_size);
                if ((g_clip_s2c.incr_in_progress == 0) && (data_size > 0))
                {
                    g_free(g_clip_s2c.data);
                    g_clip_s2c.total_bytes = data_size;
                    g_clip_s2c.data = (char *) g_malloc(g_clip_s2c.total_bytes + 1, 0);
                    g_memcpy(g_clip_s2c.data, data, g_clip_s2c.total_bytes);
                    g_clip_s2c.data[g_clip_s2c.total_bytes] = 0;
                    if (g_cfg->restrict_outbound_clipboard & CLIP_RESTRICT_FILE)
                    {
                        LOG(LOG_LEVEL_DEBUG,
                            "outbound clipboard(file) x-special/gnome-copied-files(%s) is restricted because of config",
                            g_clip_s2c.data);
                    }
                    else
                    {
                        clipboard_send_data_response_for_file(g_clip_s2c.data,
                                                              g_clip_s2c.total_bytes);
                    }

                }
            }
            else
            {
                LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_event_selection_notify: "
                          "unknown target");
            }
        }
        else
        {
            LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_event_selection_notify: "
                      "unknown selection");
        }
    }

    if (got_file_atom != 0)
    {
        /* text/uri-list or x-special/gnome-copied-files */

        if (g_cfg->restrict_outbound_clipboard & CLIP_RESTRICT_FILE)
        {
            LOG(LOG_LEVEL_DEBUG,
                "outbound clipboard(file) is restricted because of config");
        }
        else
        {
            g_clip_s2c.type = got_file_atom;
            g_clip_s2c.xrdp_clip_type = XRDP_CB_FILE;
            g_clip_s2c.converted = 0;
            g_clip_s2c.clip_time = lxevent->time;
            send_format_announce = 1;
        }

    }
    else if (got_utf8)
    {

        if (g_cfg->restrict_outbound_clipboard & CLIP_RESTRICT_TEXT)
        {
            LOG(LOG_LEVEL_DEBUG,
                "outbound clipboard(text) is restricted because of config");
        }
        else
        {
            g_clip_s2c.type = g_utf8_atom;
            g_clip_s2c.xrdp_clip_type = XRDP_CB_TEXT;
            g_clip_s2c.converted = 0;
            g_clip_s2c.clip_time = lxevent->time;
            send_format_announce = 1;
        }

    }
    else if (got_string)
    {

        /*
         * In most cases, when copying text, TARGETS atom and UTF8_STRING atom exists,
         * it means that this code block which checks STRING atom might not be never executed
         * in recent platforms.
         * Use echo foo | xclip -selection clipboard -noutf8 to reproduce it.
         */
        if (g_cfg->restrict_outbound_clipboard & CLIP_RESTRICT_TEXT)
        {
            LOG(LOG_LEVEL_DEBUG,
                "outbound clipboard(text) is restricted because of config");
        }
        else
        {
            g_clip_s2c.type = XA_STRING;
            g_clip_s2c.xrdp_clip_type = XRDP_CB_TEXT;
            g_clip_s2c.converted = 0;
            g_clip_s2c.clip_time = lxevent->time;
            send_format_announce = 1;
        }

    }
    else if (got_bmp_image)
    {

        if (g_cfg->restrict_outbound_clipboard & CLIP_RESTRICT_IMAGE)
        {
            LOG(LOG_LEVEL_DEBUG,
                "outbound clipboard(image) is restricted because of config");
        }
        else
        {
            g_clip_s2c.type = g_image_bmp_atom;
            g_clip_s2c.xrdp_clip_type = XRDP_CB_BITMAP;
            g_clip_s2c.converted = 0;
            g_clip_s2c.clip_time = lxevent->time;
            send_format_announce = 1;
        }

    }

    if (send_format_announce)
    {
        if (clipboard_send_format_announce(g_clip_s2c.xrdp_clip_type) != 0)
        {
            rv = 4;
        }
    }

    g_free(data);
    return rv;
}

/*****************************************************************************/
/* returns error
   process the SelectionRequest X event, uses XSelectionRequestEvent
   typedef struct {
     int type;             // SelectionRequest
     unsigned long serial; // # of last request processed by server
     Bool send_event;      // true if this came from a SendEvent request
     Display *display;     // Display the event was read from
     Window owner;
     Window requestor;
     Atom selection;
     Atom target;
     Atom property;
     Time time;
   } XSelectionRequestEvent; */
/*
 * When XGetWindowProperty and XChangeProperty talk about "format 32" it
 * doesn't mean a 32bit value, but actually a long. So 32 means 4 bytes on
 * a 32bit machine and 8 bytes on a 64 machine
 */
static int
clipboard_event_selection_request(XEvent *xevent)
{
    XSelectionRequestEvent *lxev;
    Atom atom_buf[10];
    Atom type;
    int atom_count;
    int fmt;
    int n_items;
    int xdata_size;
    char *xdata;

    lxev = (XSelectionRequestEvent *)xevent;
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: 0x%lx", lxev->property);
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: g_wnd %ld, "
              ".requestor %ld .owner %ld .selection %ld '%s' .target %ld .property %ld",
              g_wnd, lxev->requestor, lxev->owner, lxev->selection,
              get_atom_text(lxev->selection),
              lxev->target, lxev->property);

    if (lxev->property == None)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: "
                  "lxev->property is None");
    }
    else if (lxev->target == g_targets_atom)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: g_targets_atom");
        /* requestor is asking what the selection can be converted to */
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: "
                  "g_targets_atom");
        atom_buf[0] = g_targets_atom;
        atom_buf[1] = g_timestamp_atom;
        atom_buf[2] = g_multiple_atom;
        atom_count = 3;
        if ((g_cfg->restrict_inbound_clipboard & CLIP_RESTRICT_TEXT) == 0)
        {
            atom_buf[atom_count] = XA_STRING;
            atom_count++;
            atom_buf[atom_count] = g_utf8_atom;
            atom_count++;
        }
        if (clipboard_find_format_id(CB_FORMAT_DIB) >= 0 &&
                (g_cfg->restrict_inbound_clipboard & CLIP_RESTRICT_IMAGE) == 0)
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "  reporting image/bmp");
            atom_buf[atom_count] = g_image_bmp_atom;
            atom_count++;
        }
        if (clipboard_find_format_id(g_file_format_id) >= 0 &&
                (g_cfg->restrict_inbound_clipboard & CLIP_RESTRICT_FILE) == 0)
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "  reporting text/uri-list");
            atom_buf[atom_count] = g_file_atom1;
            atom_count++;
            LOG_DEVEL(LOG_LEVEL_DEBUG, "  reporting x-special/gnome-copied-files");
            atom_buf[atom_count] = g_file_atom2;
            atom_count++;
        }
        atom_buf[atom_count] = 0;
        LOG_DEVEL(LOG_LEVEL_DEBUG, "  reporting %d formats", atom_count);
        return clipboard_provide_selection(lxev, XA_ATOM, 32,
                                           (char *)atom_buf, atom_count);
    }
    else if (lxev->target == g_timestamp_atom)
    {
        /* requestor is asking the time I got the selection */
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: "
                  "g_timestamp_atom");
        atom_buf[0] = g_selection_time;
        atom_buf[1] = 0;
        return clipboard_provide_selection(lxev, XA_INTEGER, 32,
                                           (char *)atom_buf, 1);
    }
    else if (lxev->target == g_multiple_atom)
    {
        /* target, property pairs */
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: "
                  "g_multiple_atom");

        xdata = 0;
        if (clipboard_get_window_property(lxev->requestor, lxev->property,
                                          &type, &fmt, &n_items, &xdata,
                                          &xdata_size) == 0)
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: g_multiple_atom "
                      "n_items %d", n_items);
            /* todo */
            g_free(xdata);
        }
    }
    else if ((lxev->target == XA_STRING) || (lxev->target == g_utf8_atom))
    {
        if (clipboard_find_format_id(g_file_format_id) >= 0)
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: "
                      "text requested when files available");

            if (g_cfg->restrict_inbound_clipboard & CLIP_RESTRICT_FILE)
            {
                LOG(LOG_LEVEL_DEBUG,
                    "inbound clipboard %s is restricted because of config",
                    lxev->target == XA_STRING ? "XA_STRING" : "UTF8_STRING");
                clipboard_refuse_selection(lxev);
            }
            else
            {
                g_memcpy(&g_saved_selection_req_event, lxev,
                         sizeof(g_saved_selection_req_event));
                g_clip_c2s.type = lxev->target;
                g_clip_c2s.xrdp_clip_type = XRDP_CB_FILE;
                clipboard_send_data_request(g_file_format_id);
            }

        }
        else
        {

            if (g_cfg->restrict_inbound_clipboard & CLIP_RESTRICT_TEXT)
            {
                LOG(LOG_LEVEL_DEBUG,
                    "inbound clipboard %s is restricted because of config",
                    lxev->target == XA_STRING ? "XA_STRING" : "UTF8_STRING");
                clipboard_refuse_selection(lxev);
            }
            else
            {
                g_memcpy(&g_saved_selection_req_event, lxev,
                         sizeof(g_saved_selection_req_event));
                g_clip_c2s.type = lxev->target;
                g_clip_c2s.xrdp_clip_type = XRDP_CB_TEXT;
                clipboard_send_data_request(CB_FORMAT_UNICODETEXT);
            }

        }
        return 0;
    }
    else if (lxev->target == g_image_bmp_atom)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: image/bmp");
        if ((g_clip_c2s.type == lxev->target) && g_clip_c2s.converted)
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: -------------------------------------------");

            if (g_cfg->restrict_inbound_clipboard & CLIP_RESTRICT_IMAGE)
            {
                LOG(LOG_LEVEL_DEBUG,
                    "inbound clipboard image/bmp converted is restricted because of config");
                clipboard_refuse_selection(lxev);
            }
            else
            {
                clipboard_provide_selection_c2s(lxev, lxev->target);
            }
            return 0;

        }

        if (g_cfg->restrict_inbound_clipboard & CLIP_RESTRICT_IMAGE)
        {
            LOG(LOG_LEVEL_DEBUG,
                "inbound clipboard image/bmp is restricted because of config");
            clipboard_refuse_selection(lxev);
        }
        else
        {
            g_memcpy(&g_saved_selection_req_event, lxev,
                     sizeof(g_saved_selection_req_event));
            g_clip_c2s.type = g_image_bmp_atom;
            g_clip_c2s.xrdp_clip_type = XRDP_CB_BITMAP;
            clipboard_send_data_request(CB_FORMAT_DIB);
        }
        return 0;

    }
    else if (lxev->target == g_file_atom1)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: g_file_atom1");
        if ((g_clip_c2s.type == lxev->target) && g_clip_c2s.converted)
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: -------------------------------------------");
            if (g_cfg->restrict_inbound_clipboard & CLIP_RESTRICT_FILE)
            {
                LOG(LOG_LEVEL_DEBUG,
                    "inbound clipboard text/uri-list is restricted because of config");
                clipboard_refuse_selection(lxev);
                return 0;
            }
            else
            {
                clipboard_provide_selection_c2s(lxev, lxev->target);
                return 0;
            }
        }
        if (g_cfg->restrict_inbound_clipboard & CLIP_RESTRICT_FILE)
        {
            LOG(LOG_LEVEL_DEBUG,
                "inbound clipboard text/uri-list is restricted because of config");
            clipboard_refuse_selection(lxev);
            return 0;
        }
        else
        {
            g_memcpy(&g_saved_selection_req_event, lxev,
                     sizeof(g_saved_selection_req_event));
            g_clip_c2s.type = g_file_atom1;
            g_clip_c2s.xrdp_clip_type = XRDP_CB_FILE;
            clipboard_send_data_request(g_file_format_id);
            return 0;
        }

    }
    else if (lxev->target == g_file_atom2)
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: g_file_atom2");

        if ((g_clip_c2s.type == lxev->target) && g_clip_c2s.converted)
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_request: -------------------------------------------");
            if (g_cfg->restrict_inbound_clipboard & CLIP_RESTRICT_FILE)
            {
                LOG(LOG_LEVEL_DEBUG,
                    "inbound clipboard x-special/gnome-copied-files converted is restricted because of config");
                clipboard_refuse_selection(lxev);
                return 0;
            }
            else
            {
                clipboard_provide_selection_c2s(lxev, lxev->target);
                return 0;
            }
        }
        if (g_cfg->restrict_inbound_clipboard & CLIP_RESTRICT_FILE)
        {
            LOG(LOG_LEVEL_DEBUG,
                "inbound clipboard x-special/gnome-copied-files is restricted because of config");
            clipboard_refuse_selection(lxev);
            return 0;
        }
        else
        {
            g_memcpy(&g_saved_selection_req_event, lxev,
                     sizeof(g_saved_selection_req_event));
            g_clip_c2s.type = g_file_atom2;
            g_clip_c2s.xrdp_clip_type = XRDP_CB_FILE;
            clipboard_send_data_request(g_file_format_id);
            return 0;
        }
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, "clipboard_event_selection_request: unknown "
            "target %s", get_atom_text(lxev->target));
    }

    clipboard_refuse_selection(lxev);
    return 0;
}

/*****************************************************************************/
/* returns error
   process the SelectionClear X event, uses XSelectionClearEvent
   typedef struct {
     int type;                // SelectionClear
     unsigned long serial;    // # of last request processed by server
     Bool send_event;         // true if this came from a SendEvent request
     Display *display;        // Display the event was read from
     Window window;
     Atom selection;
     Time time;
} XSelectionClearEvent; */
static int
clipboard_event_selection_clear(XEvent *xevent)
{
    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_selection_clear:");
    return 0;
}

/*****************************************************************************/
/* returns error
   typedef struct {
     int type;                // PropertyNotify
     unsigned long serial;    // # of last request processed by server
     Bool send_event;         // true if this came from a SendEvent request
     Display *display;        // Display the event was read from
     Window window;
     Atom atom;
     Time time;
     int state;               // PropertyNewValue or PropertyDelete
} XPropertyEvent; */
static int
clipboard_event_property_notify(XEvent *xevent)
{
    Atom actual_type_return;
    int actual_format_return;
    unsigned long nitems_returned;
    unsigned long bytes_left;
    unsigned char *data;
    int rv;
    int format_in_bytes;
    int new_data_len;
    int data_bytes;
    char *cptr;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_property_notify: PropertyNotify .window %ld "
              ".state %d .atom %ld %s", xevent->xproperty.window,
              xevent->xproperty.state, xevent->xproperty.atom,
              get_atom_text(xevent->xproperty.atom));

    if (g_clip_c2s.incr_in_progress &&
            (xevent->xproperty.window == g_clip_c2s.window) &&
            (xevent->xproperty.atom == g_clip_c2s.property) &&
            (xevent->xproperty.state == PropertyDelete))
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_property_notify: INCR PropertyDelete");
        /* this is used for when copying a large clipboard to the other app,
           it will delete the property so we know to send the next one */

        if ((g_clip_c2s.data == 0) || (g_clip_c2s.total_bytes < 1))
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_property_notify: INCR error");
            return 0;
        }
        data = (tui8 *)(g_clip_c2s.data + g_clip_c2s.incr_bytes_done);
        data_bytes = g_clip_c2s.read_bytes_done - g_clip_c2s.incr_bytes_done;
        if ((data_bytes < 1) &&
                (g_clip_c2s.read_bytes_done < g_clip_c2s.total_bytes))
        {
            g_clip_c2s.incr_in_progress = 0;
            return 0;
        }
        if (data_bytes > g_incr_max_req_size)
        {
            data_bytes = g_incr_max_req_size;
        }
        g_clip_c2s.incr_bytes_done += data_bytes;
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_property_notify: data_bytes %d", data_bytes);
        XChangeProperty(xevent->xproperty.display, xevent->xproperty.window,
                        xevent->xproperty.atom, g_clip_c2s.type, 8,
                        PropModeReplace, data, data_bytes);
        if (data_bytes < 1)
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_property_notify: INCR done");
            g_clip_c2s.incr_in_progress = 0;
            /* we no longer need property notify */
            XSelectInput(xevent->xproperty.display, xevent->xproperty.window,
                         NoEventMask);
            g_clip_c2s.converted = 1;
        }
    }
    if (g_clip_s2c.incr_in_progress &&
            (xevent->xproperty.window == g_wnd) &&
            (xevent->xproperty.atom == g_clip_s2c.property) &&
            (xevent->xproperty.state == PropertyNewValue))
    {
        LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_property_notify: INCR PropertyNewValue");
        rv = XGetWindowProperty(g_display, g_wnd, g_clip_s2c.property, 0, 0, 0,
                                AnyPropertyType, &actual_type_return, &actual_format_return,
                                &nitems_returned, &bytes_left, &data);

        if (rv != Success)
        {
            return 1;
        }

        if (data != 0)
        {
            XFree(data);
            data = 0;
        }

        if (bytes_left <= 0)
        {
            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_property_notify: INCR done");
            /* clipboard INCR cycle has completed */
            g_clip_s2c.incr_in_progress = 0;
            if (g_clip_s2c.type == g_image_bmp_atom)
            {
                g_clip_s2c.xrdp_clip_type = XRDP_CB_BITMAP;
                //LOG_DEVEL_HEXDUMP(LOG_LEVEL_TRACE, "", g_last_clip_data, 64);
                /* skip header */
                clipboard_send_data_response(g_clip_s2c.xrdp_clip_type,
                                             g_clip_s2c.data + 14,
                                             g_clip_s2c.total_bytes - 14);
            }
            else if ((g_clip_s2c.type == XA_STRING) ||
                     (g_clip_s2c.type == g_utf8_atom))
            {
                g_clip_s2c.xrdp_clip_type = XRDP_CB_TEXT;
                clipboard_send_data_response(g_clip_s2c.xrdp_clip_type,
                                             g_clip_s2c.data,
                                             g_clip_s2c.total_bytes);
            }
            else
            {
                LOG_DEVEL(LOG_LEVEL_ERROR, "clipboard_event_property_notify: error unknown type %ld",
                          g_clip_s2c.type);
                clipboard_send_data_response_failed();
            }

            XDeleteProperty(g_display, g_wnd, g_clip_s2c.property);
        }
        else
        {
            rv = XGetWindowProperty(g_display, g_wnd, g_clip_s2c.property, 0, bytes_left, 0,
                                    AnyPropertyType, &actual_type_return, &actual_format_return,
                                    &nitems_returned, &bytes_left, &data);

            if (rv != Success)
            {
                return 1;
            }

            format_in_bytes = FORMAT_TO_BYTES(actual_format_return);
            new_data_len = nitems_returned * format_in_bytes;
            cptr = (char *) g_malloc(g_clip_s2c.total_bytes + new_data_len, 0);
            g_memcpy(cptr, g_clip_s2c.data, g_clip_s2c.total_bytes);
            g_free(g_clip_s2c.data);

            if (cptr == NULL)
            {
                g_clip_s2c.data = 0;

                /* cannot add any more data */
                if (data != 0)
                {
                    XFree(data);
                }

                XDeleteProperty(g_display, g_wnd, g_clip_s2c.property);
                return 0;
            }

            LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_event_property_notify: new_data_len %d", new_data_len);
            g_clip_s2c.data = cptr;
            g_memcpy(g_clip_s2c.data + g_clip_s2c.total_bytes, data, new_data_len);
            g_clip_s2c.total_bytes += new_data_len;

            if (data)
            {
                XFree(data);
            }

            XDeleteProperty(g_display, g_wnd, g_clip_s2c.property);
        }
    }

    return 0;
}

/*****************************************************************************/
/* returns 0, event handled, 1 unhandled */
int
clipboard_xevent(void *xevent)
{
    XEvent *lxevent;

    LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_xevent: event detected");

    if (!g_clip_up)
    {
        return 1;
    }

    lxevent = (XEvent *)xevent;

    switch (lxevent->type)
    {
        case SelectionNotify:
            clipboard_event_selection_notify(lxevent);
            break;
        case SelectionRequest:
            clipboard_event_selection_request(lxevent);
            break;
        case SelectionClear:
            clipboard_event_selection_clear(lxevent);
            break;
        case MappingNotify:
            break;
        case PropertyNotify:
            clipboard_event_property_notify(lxevent);
            break;
        case UnmapNotify:
            LOG_DEVEL(LOG_LEVEL_DEBUG, "chansrv::clipboard_xevent: got UnmapNotify");
            break;
        case ClientMessage:
            LOG_DEVEL(LOG_LEVEL_DEBUG, "chansrv::clipboard_xevent: got ClientMessage");
            break;
        default:

            if (lxevent->type == g_xfixes_event_base +
                    XFixesSetSelectionOwnerNotify)
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_xevent: got XFixesSetSelectionOwnerNotify");
                clipboard_event_selection_owner_notify(lxevent);
                break;
            }
            if (lxevent->type == g_xfixes_event_base +
                    XFixesSelectionWindowDestroyNotify)
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_xevent: got XFixesSelectionWindowDestroyNotify");
                break;
            }
            if (lxevent->type == g_xfixes_event_base +
                    XFixesSelectionClientCloseNotify)
            {
                LOG_DEVEL(LOG_LEVEL_DEBUG, "clipboard_xevent: got XFixesSelectionClientCloseNotify");
                break;
            }

            /* we didn't handle this message */
            return 1;
    }

    return 0;
}
