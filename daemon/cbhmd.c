/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
#include <appcore-efl.h>

#ifdef HAVE_X11
#include <Ecore_X.h>
#endif

#include <systemd/sd-daemon.h>

#ifdef X_BASED
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XIproto.h>
#endif

#include "cbhmd_utils.h"
#include "cbhmd_appdata.h"
#include "cbhmd_handler.h"
#include "cbhmd_convert.h"
#include "cbhmd_eldbus.h"
#include "cbhmd.h"

#define APP_BASE_SCALE 2.6

static Cbhmd_App_Data *cbhmd_ad = NULL;
static int cbhmd_log_domain = -1;

static int app_create(void *data)
{
   FN_CALL();
   int ret = CBHM_ERROR_NONE;
   Cbhmd_App_Data *ad = (Cbhmd_App_Data *)data;

   /* init connectivity */
   ret = cbhmd_eldbus_init(ad);
   if (CBHM_ERROR_NONE != ret)
     {
        ERR("cbhmd_eldbus_init() Fail(%d)", ret);
        return EXIT_FAILURE;
     }

   /* init UI */
   elm_app_base_scale_set(APP_BASE_SCALE);

   ret = cbhmd_drawer_display_init(ad);
   if (CBHM_ERROR_NONE != ret)
     {
        ERR("cbhmd_drawer_display_init() Fail(%d)", ret);
        return EXIT_FAILURE;
     }

   cbhmd_log_domain = eina_log_domain_register("cbhm", EINA_COLOR_LIGHTBLUE);
   if (!cbhmd_log_domain)
     {
        ERR("eina_log_domain_register() Fail");
        cbhmd_log_domain = EINA_LOG_DOMAIN_GLOBAL;
     }

   /* PATH in tizen 3.0 beta : /opt/home/owner/data/cbhm/.cbhm_files */
   if (!ecore_file_exists(COPIED_DATA_STORAGE_DIR))
     {
        if (!ecore_file_mkpath(COPIED_DATA_STORAGE_DIR))
          WARN("ecore_file_mkpath() fail");
     }

   ret = cbhmd_drawer_event_window_create(ad);
   if (CBHM_ERROR_NONE != ret)
     {
        ERR("cbhmd_drawer_event_window_create() Fail(%d)", ret);
        return EXIT_FAILURE;
     }

   cbhmd_drawer_event_window_set_title(ad);

   if (!ecore_init()) return EXIT_FAILURE;

   if (!ecore_evas_init()) return EXIT_FAILURE;

   if (!edje_init()) return EXIT_FAILURE;

   ad->magic = CBHM_MAGIC;

   cbhmd_convert_target_atoms_init(ad);

   if (!(ad->drawer = cbhmd_drawer_init(ad))) return EXIT_FAILURE;

   /* to be identified by E20 */
   elm_win_role_set(ad->drawer->main_win, "cbhm");

   ret = cbhmd_handler_init(ad);
   if (CBHM_ERROR_NONE != ret)
     {
        ERR("cbhmd_handler_init() Fail", ret);
        return EXIT_FAILURE;
     }

   if (!(ad->storage = cbhmd_storage_init(ad))) return EXIT_FAILURE;

#ifdef HAVE_X11
   cbhmd_x_handler_slot_item_count_set(ad);
   set_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD, NULL);
#endif

   return 0;
}

static int app_terminate(void *data)
{
   Cbhmd_App_Data *ad = data;

   item_clear_all(ad);
   cbhmd_drawer_deinit(ad->drawer);

#ifdef HAVE_X11
   cbhmd_x_handler_deinit(ad->xhandler);
#endif
#ifdef HAVE_WAYLAND
   cbhmd_wl_handler_deinit(ad->wlhandler);
#endif

   depose_storage(ad->storage);
   cbhmd_convert_target_atoms_deinit(ad);

   FREE(ad);

   eina_log_domain_unregister(cbhmd_log_domain);
   cbhmd_log_domain = -1;

   cbhmd_eldbus_deinit();

   return 0;
}

static int app_pause(void *data)
{
   Cbhmd_App_Data *ad = data;
#ifdef HAVE_X11
   Ecore_X_Illume_Clipboard_State state = ecore_x_e_illume_clipboard_state_get(ad->x_active_win);
   if(state == ECORE_X_ILLUME_CLIPBOARD_STATE_ON)
#endif
     {
        cbhmd_drawer_hide(ad);
     }
   return 0;
}

static int app_resume(void *data)
{
   return 0;
}

static int app_reset(bundle *b, void *data)
{
   return 0;
}

static int __lang_changed(void *event_info, void *data)
{
   return 0;
}

int main(int argc, char *argv[])
{
   Cbhmd_App_Data *ad;

   struct appcore_ops ops = {
        .create = app_create,
        .terminate = app_terminate,
        .pause = app_pause, 
        .resume = app_resume,
        .reset = app_reset,
   };

   ad = calloc(1, sizeof(Cbhmd_App_Data));
   ops.data = ad;
   cbhmd_ad = ad;

   //appcore_set_i18n(PACKAGE, LOCALE_DIR);
   appcore_set_event_callback(APPCORE_EVENT_LANG_CHANGE, __lang_changed, NULL);

   // Notyfication to systemd
   sd_notify(1, "READY=1");

   return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}
