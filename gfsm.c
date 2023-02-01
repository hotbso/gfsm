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
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#include "XPLMPlugin.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMProcessing.h"

#define UNUSED(x) (void)(x)

#define VERSION "1.0-b1"

static float game_loop_cb(float elapsed_last_call,
                float elapsed_last_loop, int counter,
                void *in_refcon);


static XPLMDataRef date_day_dr, gfsm_season_dr;
static int day_prev;
static int gfsm_season = 1; /* init to a reasonable value */

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
    int day = XPLMGetDatai(date_day_dr);
    if (day == day_prev)
        return;

    log_msg("day: %d/%d", day_prev, day);
    day_prev = day;

    if (day < 80)
        gfsm_season = 4;
    else if (day < 170)
        gfsm_season = 1;
    else if (day < 260)
        gfsm_season = 2;
    else if (day < 350)
        gfsm_season = 3;
    else
        gfsm_season = 4;

    log_msg("Season: %d", gfsm_season);
}

PLUGIN_API int
XPluginStart(char *out_name, char *out_sig, char *out_desc)
{

    strcpy(out_name, "gfsm " VERSION);
    strcpy(out_sig, "gfsm.hotbso");
    strcpy(out_desc, "A plugin that manages GlobalForests seasons");

    XPLMRegisterFlightLoopCallback(game_loop_cb, 1.0f, NULL);

    date_day_dr = XPLMFindDataRef("sim/time/local_date_days");
    gfsm_season_dr = XPLMRegisterDataAccessor("GlobalForests/season", xplmType_Int, 0, read_season_acc,
                                            NULL, NULL, NULL, NULL,
                                            NULL, NULL, NULL, NULL,
                                            NULL, NULL, NULL, NULL,
                                            NULL);
    if (NULL == date_day_dr || NULL == gfsm_season_dr) {
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
}


PLUGIN_API int
XPluginEnable(void)
{
    set_season();
    return 1;
}

PLUGIN_API void
XPluginReceiveMessage(XPLMPluginID in_from, long in_msg, void *in_param)
{
    UNUSED(in_from);
    /* try to catch any event that may come prior to a scenery reload */
    set_season();
}

/* try to catch any event that may come prior to a scenery reload */

static float
game_loop_cb(float elapsed_last_call,
             float elapsed_last_loop, int counter, void *in_refcon)
{
    float loop_delay = 1.0f;
    set_season();
    return loop_delay;
}
