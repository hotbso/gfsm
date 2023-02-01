/*

MIT License

Copyright (c) 2023

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
#include <errno.h>

#include "XPLMPlugin.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMProcessing.h"
#include "XPLMMenus.h"
#include "XPWidgets.h"
#include "XPStandardWidgets.h"

#define UNUSED(x) (void)(x)

#define VERSION "1.0-b1"

static float game_loop_cb(float elapsed_last_call,
                float elapsed_last_loop, int counter,
                void *in_refcon);


static char xpdir[512];
static const char *psep;
static XPLMMenuID gfsm_menu;
static int gfsm_enable_item;
static XPWidgetID pref_widget, pref_slider, pref_slider_v, pref_btn;

static char pref_path[512];
static XPLMDataRef date_day_dr, gfsm_season_dr;
static int day, day_prev;
static int gfsm_season = 1;

/* this is a nonessential plugin so if anything goes wrong
   we just disable and protect the sim */
static int gfsm_error_disabled;
static int gfsm_enabled = 1;

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

    //fprintf(f, "%d %d", gfsm_enabled, gfsm_keep);
    fclose(f);
}


static void
load_pref()
{
    FILE *f  = fopen(pref_path, "r");
    if (NULL == f)
        return;

    //fscanf(f, "%i %i", &gfsm_enabled, &gfsm_keep);
    fclose(f);
    //log_msg("From pref: gfsm_enabled: %d, gfsm_keep: %d", gfsm_enabled, gfsm_keep);
}


static int
widget_cb(XPWidgetMessage msg, XPWidgetID widget_id, intptr_t param1, intptr_t param2)
{
    if ((widget_id == pref_widget) && (msg == xpMessage_CloseButtonPushed)) {
        XPHideWidget(pref_widget);
        return 1;
#if 0
    } else if ((widget_id == pref_btn) && (msg == xpMsg_PushButtonPressed)) {
        int valid;
        int k = XPGetWidgetProperty(pref_slider, xpProperty_ScrollBarSliderPosition, &valid);
        if (valid)
            gfsm_keep = k;

        XPHideWidget(pref_widget);
        log_msg("gfsm_keep set to %d", gfsm_keep);
        return 1;

    } else if ((msg == xpMsg_ScrollBarSliderPositionChanged) && ((XPWidgetID)param1 == pref_slider)) {
        int k = XPGetWidgetProperty(pref_slider, xpProperty_ScrollBarSliderPosition, NULL);

        char buff[10];
        snprintf(buff, sizeof(buff), "%d", k);
        XPSetWidgetDescriptor(pref_slider_v, buff);
        return 1;
#endif
    }

    return 0;
}


static void
menu_cb(void *menu_ref, void *item_ref)
{
    if ((int *)item_ref == &gfsm_enabled) {
        gfsm_enabled = !gfsm_enabled;
        XPLMCheckMenuItem(gfsm_menu, gfsm_enable_item,
                          gfsm_enabled ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    }
#if 0
    } else if ((int *)item_ref == &gfsm_keep) {

        /* create gui */
        if (NULL == pref_widget) {
            int left = 200;
            int top = 600;
            int width = 200;
            int height = 100;

            pref_widget = XPCreateWidget(left, top, left + width, top - height,
                                         0, "Autosave Saver", 1, NULL, xpWidgetClass_MainWindow);
            XPSetWidgetProperty(pref_widget, xpProperty_MainWindowHasCloseBoxes, 1);
            XPAddWidgetCallback(pref_widget, widget_cb);
            left += 5; top -= 25;
            XPCreateWidget(left, top, left + width - 2 * 5, top - 15,
                           1, "Autosave copies to keep", 0, pref_widget, xpWidgetClass_Caption);
            top -= 20;
            pref_slider = XPCreateWidget(left, top, left + width - 30, top - 25,
                                         1, "gfsm_keep", 0, pref_widget, xpWidgetClass_ScrollBar);

            char buff[10];
            snprintf(buff, sizeof(buff), "%d", gfsm_keep);
            pref_slider_v = XPCreateWidget(left + width - 25, top, left + width - 2*5 , top - 25,
                                           1, buff, 0, pref_widget, xpWidgetClass_Caption);

            XPSetWidgetProperty(pref_slider, xpProperty_ScrollBarMin, KEEP_MIN);
            XPSetWidgetProperty(pref_slider, xpProperty_ScrollBarMax, KEEP_MAX);
            XPSetWidgetProperty(pref_slider, xpProperty_ScrollBarPageAmount, 1);
            // XPSetWidgetProperty(pref_slider, xpProperty_ScrollBarSliderPosition, gfsm_keep);

            top -= 30;
            pref_btn = XPCreateWidget(left, top, left + width - 2*5, top - 20,
                                      1, "OK", 0, pref_widget, xpWidgetClass_Button);
            XPAddWidgetCallback(pref_btn, widget_cb);
        }

        XPShowWidget(pref_widget);
    }
#endif
}

static int
read_season_acc(void *ref)
{
    return gfsm_season;
}

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
    XPLMMenuID menu;
    int sub_menu;

    /* Always use Unix-native paths on the Mac! */
    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);

    psep = XPLMGetDirectorySeparator();
    XPLMGetSystemPath(xpdir);

    strcpy(out_name, "gfsm " VERSION);
    strcpy(out_sig, "gfsm.hotbso");
    strcpy(out_desc, "A plugin ...");

    /* load preferences */
    XPLMGetPrefsPath(pref_path);
    XPLMExtractFileAndPath(pref_path);
    strcat(pref_path, psep);
    strcat(pref_path, "gfsm.prf");
    load_pref();

    XPLMRegisterFlightLoopCallback(game_loop_cb, 1.0f, NULL);

    menu = XPLMFindPluginsMenu();
    sub_menu = XPLMAppendMenuItem(menu, "gfsm", NULL, 1);
    gfsm_menu = XPLMCreateMenu("gfsm", menu, sub_menu, menu_cb, NULL);
    gfsm_enable_item = XPLMAppendMenuItem(gfsm_menu, "Enabled", &gfsm_enabled, 0);
    XPLMCheckMenuItem(gfsm_menu, gfsm_enable_item,
                      gfsm_enabled ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    //XPLMAppendMenuItem(gfsm_menu, "Configure", &gfsm_keep, 0);
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
    save_pref();
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
    set_season();
}



static float
game_loop_cb(float elapsed_last_call,
             float elapsed_last_loop, int counter, void *in_refcon)
{
    float loop_delay = 1.0f;
    set_season();
    return loop_delay;
}
