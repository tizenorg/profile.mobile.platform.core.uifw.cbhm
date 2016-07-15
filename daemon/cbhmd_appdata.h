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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_APPDATA_H__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_APPDATA_H__

typedef struct _cbhmd_app_data cbhmd_app_data_s;
typedef struct _cbhmd_handler_target cbhmd_handler_target_s;
typedef struct _cbhmd_drawer_data cbhmd_drawer_data_s;
typedef struct _cbhmd_cnp_item cbhmd_cnp_item_s;
typedef struct _cbhmd_storage_data cbhmd_storage_data_s;
#ifdef HAVE_X11
typedef struct _cbhmd_x_handler_data cbhmd_x_handler_data_s;
#endif
#ifdef HAVE_WAYLAND
typedef struct _cbhmd_wl_handler_data cbhmd_wl_handler_data_s;
#endif

#include "cbhmd.h"
#include "cbhmd_storage.h"
#include "cbhmd_handler.h"
#include "cbhmd_convert.h"
#include "cbhmd_drawer.h"
#include "cbhmd_item_manager.h"

struct _cbhmd_app_data {
	int magic;
#ifdef HAVE_X11
	Ecore_X_Display *x_disp;
	Ecore_X_Window x_root_win;
	Ecore_X_Window x_event_win;
	Ecore_X_Window x_active_win;
#endif
#ifdef HAVE_WAYLAND
	const char *wl_disp;
	Ecore_Wl_Window *wl_event_win;
	Ecore_Wl_Window *wl_active_win;
#endif
	Eina_List *item_list;

	Eina_Bool (*draw_item_add)(cbhmd_app_data_s *ad, cbhmd_cnp_item_s *item);
	Eina_Bool (*draw_item_del)(cbhmd_app_data_s *ad, cbhmd_cnp_item_s *item);
	Eina_Bool (*storage_item_add)(cbhmd_app_data_s *ad, cbhmd_cnp_item_s *item);
	Eina_Bool (*storage_item_del)(cbhmd_app_data_s *ad, cbhmd_cnp_item_s *item);
	Eina_Bool (*storage_item_update)(cbhmd_app_data_s *ad, cbhmd_cnp_item_s *item);
	cbhmd_cnp_item_s *(*storage_item_load)(cbhmd_storage_data_s *sd, int index);
	cbhmd_drawer_data_s *drawer;
#ifdef HAVE_X11
	cbhmd_x_handler_data_s *xhandler;
#endif
#ifdef HAVE_WAYLAND
	cbhmd_wl_handler_data_s *wlhandler;
	Eldbus_Service_Interface *iface;
#endif
	cbhmd_storage_data_s *storage;

	cbhmd_cnp_item_s *selected_item;
	cbhmd_handler_target_s targetAtoms[ATOM_INDEX_MAX];
};

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_APPDATA_H__ */
