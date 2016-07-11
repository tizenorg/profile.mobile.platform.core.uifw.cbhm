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

typedef struct _AppData AppData;
typedef struct _TargetHandler TargetHandler;
typedef struct _ClipdrawerData ClipdrawerData;
typedef struct _CNP_ITEM CNP_ITEM;
#ifdef HAVE_X11
typedef struct _XHandlerData XHandlerData;
#endif
#ifdef HAVE_WAYLAND
typedef struct _WlHandlerData WlHandlerData;
#endif
typedef struct _StorageData StorageData;

#include "cbhmd.h"
#include "cbhmd_storage.h"
#include "cbhmd_handler.h"
#include "cbhmd_converter.h"
#include "cbhmd_clipdrawer.h"
#include "cbhmd_item_manager.h"

struct _AppData {
	int magic;
#ifdef HAVE_X11
	Ecore_X_Display *x_disp;
	Ecore_X_Window x_root_win;
	Ecore_X_Window x_event_win;
	Ecore_X_Window x_active_win;
#endif
#ifdef HAVE_WAYLAND
	const char *wl_disp;
	Ecore_Wl_Window *wl_root_win;
	Ecore_Wl_Window *wl_event_win;
	Ecore_Wl_Window *wl_active_win;
#endif
	Eina_List *item_list;

	Eina_Bool (*draw_item_add)(AppData *ad, CNP_ITEM *item);
	Eina_Bool (*draw_item_del)(AppData *ad, CNP_ITEM *item);
	Eina_Bool (*storage_item_add)(AppData *ad, CNP_ITEM *item);
	Eina_Bool (*storage_item_del)(AppData *ad, CNP_ITEM *item);
	Eina_Bool (*storage_item_update)(AppData *ad, CNP_ITEM *item);
	CNP_ITEM *(*storage_item_load)(StorageData *sd, int index);
	ClipdrawerData *clipdrawer;
#ifdef HAVE_X11
	XHandlerData *xhandler;
#endif
#ifdef HAVE_WAYLAND
	WlHandlerData *wlhandler;
	Eldbus_Service_Interface *iface;
	Eina_Bool send_item_clicked;
#endif
	StorageData *storage;

	CNP_ITEM *clip_selected_item;
	TargetHandler targetAtoms[ATOM_INDEX_MAX];
};

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_APPDATA_H__ */
