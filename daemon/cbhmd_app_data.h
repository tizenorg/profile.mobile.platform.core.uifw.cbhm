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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_APP_DATA_H__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_APP_DATA_H__

typedef struct _Cbhmd_App_Data Cbhmd_App_Data;
typedef struct _Cbhmd_Handler_Target Cbhmd_Handler_Target;
typedef struct _Cbhmd_Drawer_Data Cbhmd_Drawer_Data;
typedef struct _Cbhmd_Cnp_Item Cbhmd_Cnp_Item;
typedef struct _Cbhmd_Storage_Data Cbhmd_Storage_Data;
#ifdef HAVE_X11
typedef struct _Cbhmd_X_Handler_Data Cbhmd_X_Handler_Data;
#endif
#ifdef HAVE_WAYLAND
typedef struct _Cbhmd_Wl_Handler_Data Cbhmd_Wl_Handler_Data;
#endif

#include "cbhmd.h"
#include "cbhmd_storage.h"
#include "cbhmd_handler.h"
#include "cbhmd_convert.h"
#include "cbhmd_drawer.h"
#include "cbhmd_item.h"

struct _Cbhmd_App_Data
{
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

   Eina_Bool (*Drawer_Item_Add_Cb)(Cbhmd_App_Data *ad, Cbhmd_Cnp_Item *item);
   Eina_Bool (*Drawer_Item_Del_Cb)(Cbhmd_App_Data *ad, Cbhmd_Cnp_Item *item);
   Eina_Bool (*Storage_Item_Add_Cb)(Cbhmd_App_Data *ad, Cbhmd_Cnp_Item *item);
   Eina_Bool (*Storage_Item_Del_Cb)(Cbhmd_App_Data *ad, Cbhmd_Cnp_Item *item);
   Eina_Bool (*Storage_Item_Update_Cb)(Cbhmd_App_Data *ad, Cbhmd_Cnp_Item *item);
   Cbhmd_Cnp_Item *(*Storage_Item_Load_Cb)(Cbhmd_Storage_Data *sd, int index);

   Cbhmd_Drawer_Data *drawer;
#ifdef HAVE_X11
   Cbhmd_X_Handler_Data *x_handler;
#endif
#ifdef HAVE_WAYLAND
   Cbhmd_Wl_Handler_Data *wl_handler;
   Eldbus_Service_Interface *iface;
#endif
   Cbhmd_Storage_Data *storage;

   Cbhmd_Cnp_Item *selected_item;
   Cbhmd_Handler_Target targetAtoms[ATOM_INDEX_MAX];
};

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_APP_DATA_H__ */
