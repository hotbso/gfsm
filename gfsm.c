/*

MIT License

Copyright (c) 2023 Holger Teutsch

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "XPLMPlugin.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMProcessing.h"
#include "XPLMMenus.h"

#define VERSION "1.0"

static char pref_path[512];
static const char *psep;
static XPLMMenuID menu_id;
static int enable_item;
static int gfsm_enabled;

static XPLMDataRef date_day_dr, latitude_dr, gfsm_season_dr;
static int day_prev;
static int gfsm_season = 0; /* init as disabled before using pref file */
static int cached_season = -10;

static void
log_msg(const char *fmt, ...)
{
    char line[1024];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line) - 3, fmt, ap);
    strcat(line, "\n");
    XPLMDebugString("gfsm: ");
    XPLMDebugString(line);
    va_end(ap);
}

static void
save_pref()
{
    FILE *f = fopen(pref_path, "w");
    if (NULL == f)
        return;

    fprintf(f, "%d,%d", gfsm_enabled, gfsm_season);  // cache current season
    fclose(f);
}


static void
load_pref()
{
    FILE *f  = fopen(pref_path, "r");
    if (NULL == f)
        return;

    if (1 <= fscanf(f, "%i,%i", &gfsm_enabled, &cached_season))
        log_msg("From pref: gfsm_enabled: %d, cached_season: %d", gfsm_enabled, cached_season);
    else {
        gfsm_enabled = 0;
        log_msg("Error readinf pref");
    }
    fclose(f);
}

// Accessor for the "GlobalForests/season" dataref
static int
read_season_acc(void *ref)
{
    return gfsm_season;
}

// set season according to standard values
static void
set_season(void)
{
    if (! gfsm_enabled) {
        gfsm_season = 0;
        return;
    }

    int day = XPLMGetDatai(date_day_dr);
    if (day == day_prev)
        return;

    if (day < 80)
        gfsm_season = -2;
    else if (day < 170)
        gfsm_season = 1;
    else if (day < 260)
        gfsm_season = 2;
    else if (day < 350)
        gfsm_season = -1;
    else
        gfsm_season = -2;

    double lat = XPLMGetDatad(latitude_dr);
    if (lat < 0)
        gfsm_season = -gfsm_season;

    log_msg("day: %d->%d, season: %d", day_prev, day, gfsm_season);
    day_prev = day;
}

static void
menu_cb(void *menu_ref, void *item_ref)
{
    if ((int *)item_ref == &gfsm_enabled) {
        gfsm_enabled = !gfsm_enabled;
        XPLMCheckMenuItem(menu_id, enable_item,
                          gfsm_enabled ? xplm_Menu_Checked : xplm_Menu_Unchecked);
        set_season();
        save_pref();
    }
}

PLUGIN_API int
XPluginStart(char *out_name, char *out_sig, char *out_desc)
{
    XPLMMenuID menu;
    int sub_menu;

    strcpy(out_name, "gfsm " VERSION);
    strcpy(out_sig, "gfsm.hotbso");
    strcpy(out_desc, "A plugin that manages GlobalForests seasons");

    /* Always use Unix-native paths on the Mac! */
    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);

    psep = XPLMGetDirectorySeparator();

    /* set pref path */
    XPLMGetPrefsPath(pref_path);
    XPLMExtractFileAndPath(pref_path);
    strcat(pref_path, psep);
    strcat(pref_path, "gfsm.prf");

    menu = XPLMFindPluginsMenu();
    sub_menu = XPLMAppendMenuItem(menu, "GlobalForests Manager", NULL, 1);
    menu_id = XPLMCreateMenu("GlobalForests Manager", menu, sub_menu, menu_cb, NULL);
    enable_item = XPLMAppendMenuItem(menu_id, "Orthophoto mode", &gfsm_enabled, 0);

    load_pref();
    XPLMCheckMenuItem(menu_id, enable_item,
                      gfsm_enabled ? xplm_Menu_Checked : xplm_Menu_Unchecked);

    if ((cached_season >= -2) && gfsm_enabled)
        gfsm_season = cached_season;
    else
        set_season();   /* likely to be always spring here */

    date_day_dr = XPLMFindDataRef("sim/time/local_date_days");
    latitude_dr = XPLMFindDataRef("sim/flightmodel/position/latitude");

    gfsm_season_dr = XPLMRegisterDataAccessor("GlobalForests/season", xplmType_Int, 0, read_season_acc,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL);

    if (NULL == date_day_dr || NULL == gfsm_season_dr || NULL == latitude_dr) {
        log_msg("Sorry, can't define dataref");
        return 0;
    }

    return 1;
}


PLUGIN_API void
XPluginStop(void)
{
}


PLUGIN_API void
XPluginDisable(void)
{
    save_pref();
}


PLUGIN_API int
XPluginEnable(void)
{
    return 1;
}

PLUGIN_API void
XPluginReceiveMessage(XPLMPluginID in_from, long in_msg, void *in_param)
{
    /* try to catch any event that may come prior to a scenery reload */
    set_season();
}
