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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_DRAWER_H__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_DRAWER_H__

#ifdef HAVE_X11
#include <Ecore_X.h>
#endif
#ifdef HAVE_WAYLAND
#include <Ecore_Wayland.h>
#endif

#include "cbhmd.h"
#include "cbhmd_app_data.h"
#ifdef HAVE_X11
#include "cbhmd_x_drawer.h"
#endif
#ifdef HAVE_WAYLAND
#include "cbhmd_wl_drawer.h"
#endif

typedef enum _AnimStatus AnimStatus;
enum _AnimStatus
{
   STATUS_NONE = 0,
   SHOW_ANIM,
   HIDE_ANIM
};

struct _Cbhmd_Drawer_Data
{
   Evas_Object *main_win;
   Evas_Object *gengrid;
   Evas_Object *main_layout;
   Elm_Gengrid_Item_Class gic_text;
   Elm_Gengrid_Item_Class gic_image;
   Elm_Gengrid_Item_Class gic_combined;
   Evas_Object *popup;
   Evas_Object *popup_win;
   Evas_Object *popup_conform;
   Evas_Object *cbhm_popup;
   Evas_Object *noc_layout;
   Evas *evas;
   Ecore_Event_Handler *keydown_handler;
   Ecore_Timer *anim_timer;
   Elm_Gengrid_Item_Class gic;
   Ecore_Timer *lower_view_timer;
   int locked_item_count;
#ifdef HAVE_X11
   Ecore_X_Window x_main_win;
#endif
#ifdef HAVE_WAYLAND
   Ecore_Wl_Window *wl_main_win;
#endif

   int o_degree;

   int root_w;
   int root_h;

   int height;
   int landscape_height;
   int grid_item_bg_w;
   int grid_item_bg_h;
   int grid_image_item_w;
   int grid_image_item_h;

   AnimStatus anim_status;
   int anim_count;
   Eina_Bool popup_activate :1;
   Eina_Bool paste_text_only :1;
   Eina_Bool item_clicked :1;
   Eina_Bool del_btn_clicked :1;
   Eina_Bool send_item_clicked :1;
   Eina_Bool http_path :1;
   Evas_Object *event_rect;
   Evas_Object *gesture_layer;
};

Eina_Bool delete_mode;

void cbhmd_drawer_set_rotation(Cbhmd_App_Data *ad);
void cbhmd_drawer_show(Cbhmd_App_Data *ad);
void cbhmd_drawer_hide(Cbhmd_App_Data *ad);
Cbhmd_Drawer_Data* cbhmd_drawer_init(Cbhmd_App_Data *ad);
void cbhmd_drawer_deinit(Cbhmd_Drawer_Data *dd);

void cbhmd_drawer_delete_mode_set(Cbhmd_App_Data *ad, Eina_Bool del_mode);
void cbhmd_drawer_text_only_mode_set(Cbhmd_App_Data *ad, Eina_Bool textonly);
Eina_Bool cbhmd_drawer_text_only_mode_get(Cbhmd_App_Data *ad);

void cbhmd_drawer_focus_set(Cbhmd_App_Data *ad, Eina_Bool enable);
int cbhmd_drawer_event_window_create(Cbhmd_App_Data *ad);
void cbhmd_drawer_event_window_set_title(Cbhmd_App_Data *ad);
int cbhmd_drawer_display_init(Cbhmd_App_Data *ad);

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_DRAWER_H__ */
