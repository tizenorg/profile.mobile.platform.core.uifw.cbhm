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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_CLIPDRAWER_X_H__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_CLIPDRAWER_X_H__

#include <Ecore_Wayland.h>

#include "cbhmd_appdata.h"

void cbhmd_wl_drawer_focus_set(Ecore_Wl_Window *wl_main_win, Eina_Bool enable);
int cbhmd_wl_drawer_effect_and_focus_set(Cbhmd_App_Data *ad);
void cbhmd_wl_drawer_event_window_set_title(Ecore_Wl_Window *wl_event_win);

void cbhmd_wl_drawer_set_transient(Ecore_Wl_Window *transient_win,
      Ecore_Wl_Window *toplevel_win);
void cbhmd_wl_drawer_unset_transient(Ecore_Wl_Window *wl_main_win);

Ecore_Wl_Window* cbhmd_wl_drawer_isf_ise_window_get();

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_CLIPDRAWER_X_H__ */
