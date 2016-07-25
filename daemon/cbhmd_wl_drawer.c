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

#include "cbhmd_wl_drawer.h"
#include "cbhmd_app_data.h"

void
cbhmd_wl_drawer_focus_set(Ecore_Wl_Window *wl_main_win, Eina_Bool enable)
{
   /* FIXME : focus should be set */
}

int
cbhmd_wl_drawer_effect_and_focus_set(Cbhmd_App_Data *ad, Cbhmd_Drawer_Data *dd)
{
   RETV_IF(NULL == ad, CBHM_ERROR_INVALID_PARAMETER);
   RETV_IF(NULL == dd, CBHM_ERROR_INVALID_PARAMETER);
   RETV_IF(NULL == dd->main_win, CBHM_ERROR_INVALID_PARAMETER);

   /* disable main window effect */
   if (-1 == elm_win_aux_hint_add(dd->main_win, "wm.policy.win.effect.disable", "1"))
     {
        ERR("elm_win_aux_hint_add() Fail");
        return CBHM_ERROR_NOT_SUPPORTED;
     }

   cbhmd_drawer_focus_set(ad, EINA_FALSE);
   ecore_wl_flush();

   return CBHM_ERROR_NONE;
}

void
cbhmd_wl_drawer_set_transient(Ecore_Wl_Window *transient_win,
                              Ecore_Wl_Window *toplevel_win)
{
   RET_IF(NULL == transient_win);
   RET_IF(NULL == toplevel_win);

   /* FIXME : "transient" relation should be set */
}

void
cbhmd_wl_drawer_unset_transient(Ecore_Wl_Window *wl_main_win)
{
}

void
cbhmd_wl_drawer_event_window_set_title(Ecore_Wl_Window *wl_event_win)
{
   /* FIXME : After decide whether use wl_event_win or not, use below func */
   //   ecore_wl_window_title_set(wl_event_win,
   //           CLIPBOARD_MANAGER_WINDOW_TITLE_STRING);
   //   ecore_wl_flush();
}

Ecore_Wl_Window *
cbhmd_wl_drawer_isf_ise_window_get()
{
   /* FIXME : Need to impl. */
   return 0;
}
