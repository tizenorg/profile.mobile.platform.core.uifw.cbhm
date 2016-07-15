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

#include "cbhmd.h"
#include "cbhmd_eldbus.h"
#include "cbhmd_appdata.h"
#include "cbhmd_handler.h"
#include "cbhmd_item_manager.h"
#include "cbhmd_utils.h"
#include "cbhmd_drawer.h"

#define CBHMD_ATOM_CLIPBOARD_MANAGER_NAME "CLIPBOARD_MANAGER"

#define EDJ_PATH RES_DIR"/edje"
#define APP_EDJ_FILE EDJ_PATH"/cbhmdrawer.edj"
#define GRP_MAIN "cbhmdrawer"

#define ANIM_DURATION 30 // 1 seconds
#define ANIM_FLOPS (0.5/30)
#define DEFAULT_WIDTH 720
#define COMBINED_ITEM_IMAGE_WIDTH 102
#define COMBINED_ITEM_IMAGE_HEIGHT 153
#define BUFF (int)1024 //limited gengrid text buffer

#define EDJE_CLOSE_PART_PREFIX "background/title/close/bg"
#define EDJE_DELETE_MODE_PREFIX "background/title/delete/image"
#define EDJE_DELETE_ALL_BTN_PART_PREFIX "background/title/delete_all/image"

#define TIME_DELAY_LOWER_VIEW 0.1 //Time to delay lower view

static int _cbhmd_drawer_popup_create(Cbhmd_App_Data *ad);
static void __drawer_grid_item_btn_clicked_cb(void *data, Evas_Object *obj,
                                              void *event_info);
static void _cbhmd_drawer_evas_object_focus_set(Evas_Object *obj,
                                                 Eina_Bool enable);

static Evas_Event_Flags __drawer_flick_end_cb(void *data, void *event_info)
{
   Cbhmd_App_Data *ad = data;
   Elm_Gesture_Line_Info *event = event_info;
   int x_diff, y_diff;

   x_diff = event->momentum.x2 - event->momentum.x1;
   y_diff = event->momentum.y2 - event->momentum.y1;

   if (y_diff > 0 && y_diff > abs(x_diff))
     cbhmd_drawer_hide(ad);

   return EVAS_EVENT_FLAG_NONE;
}

static void _cbhmd_drawer_textonly_mode_set(Cbhmd_Drawer_Data *dd)
{
   Cbhmd_Cnp_Item *item = NULL;

   Elm_Object_Item *gitem = elm_gengrid_first_item_get(dd->gengrid);
   char *entry_text = NULL;

   while (gitem)
     {
        item = elm_object_item_data_get(gitem);
        if (!item)
          return;
        if (item->type_index == ATOM_INDEX_IMAGE)
          {
             if (dd->paste_text_only)
               elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
             else elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
          }

        if (item->gitem_style == GRID_ITEM_STYLE_COMBINED)
          {
             entry_text = cbhmd_convert_get_entry_string(item->ad, item->type_index,
                                                         item->data);
             entry_text = evas_textblock_text_markup_to_utf8(NULL, entry_text);

             if (dd->paste_text_only && entry_text && !strlen(entry_text))
               elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
             else elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
             if (entry_text)
               free(entry_text);
          }

        gitem = elm_gengrid_item_next_get(gitem);
     }
}

void cbhmd_drawer_text_only_mode_set(Cbhmd_App_Data *ad, Eina_Bool textonly)
{
   FN_CALL();
   Cbhmd_Drawer_Data *dd = ad->drawer;
   if (dd->paste_text_only != textonly)
     dd->paste_text_only = textonly;
   DBG("paste textonly mode = %d", textonly);

   _cbhmd_drawer_textonly_mode_set(dd);
}

Eina_Bool cbhmd_drawer_text_only_mode_get(Cbhmd_App_Data *ad)
{
   Cbhmd_Drawer_Data *dd = ad->drawer;
   return dd->paste_text_only;
}

static int _cbhmd_drawer_edj_load(Cbhmd_Drawer_Data *dd, const char *file,
                                   const char *group)
{
   RETV_IF(NULL == dd, CBHM_ERROR_INVALID_PARAMETER);
   RETV_IF(NULL == dd->main_win, CBHM_ERROR_INVALID_PARAMETER);

   Evas_Object *main_win = dd->main_win;
   Evas_Object *layout = elm_layout_add(main_win);
   if (!layout)
     {
        ERR("elm_layout_add() Fail");
        return CBHM_ERROR_NO_DATA;
     }

   if (!elm_layout_file_set(layout, file, group))
     {
        ERR("elm_layout_file_set() Fail(%s)", file);
        evas_object_del(layout);
        return CBHM_ERROR_NO_DATA;
     }

   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(main_win, layout);

   evas_object_show(layout);

   dd->main_layout = layout;

   return CBHM_ERROR_NONE;
}

static void __drawer_gengrid_select_cb(void *data, Evas_Object *obj,
                                       void *event)
{
   Cbhmd_Cnp_Item *item = data;
   Cbhmd_App_Data *ad = item->ad;
   Cbhmd_Drawer_Data *dd = ad->drawer;

   elm_gengrid_item_selected_set(event, EINA_FALSE);
   if (dd->del_btn_clicked)
     {
        item_delete_by_Cbhmd_Cnp_Item(ad, item);
        dd->del_btn_clicked = EINA_FALSE;
     }
   else
     {
        if (delete_mode)
          return;

        if (dd->popup_activate)
          return;

        if (item->type_index != ATOM_INDEX_IMAGE || !dd->paste_text_only)
          {
             ad->selected_item = item;
#ifdef HAVE_X11
             if (is_cbhm_selection_owner(ad, ECORE_X_SELECTION_SECONDARY))
               dd->item_clicked = EINA_TRUE;
             else
               set_selection_owner(ad, ECORE_X_SELECTION_SECONDARY, NULL);
#endif
#ifdef HAVE_WAYLAND
             cbhmd_eldbus_send_item_clicked_signal(ad->iface);
             dd->send_item_clicked = EINA_TRUE;
#endif
          }
     }
}

static Eina_Bool __drawer_keydown_cb(void *data, int type, void *event)
{
   if (!data || !event)
     return ECORE_CALLBACK_DONE;
   Cbhmd_App_Data *ad = data;
   Cbhmd_Drawer_Data *dd = ad->drawer;
   Ecore_Event_Key *ev = event;

   if (!SAFE_STRCMP(ev->keyname, "XF86Back"))
     {
        if (dd->popup_activate)
          {
             dd->popup_activate = EINA_FALSE;
             evas_object_del(dd->popup_conform);
             evas_object_del(dd->popup_win);
          }
        else
          if (delete_mode)
            cbhmd_drawer_delete_mode_set(ad, EINA_FALSE);
          else cbhmd_drawer_hide(ad);
     }
   else
     if (!SAFE_STRCMP(ev->keyname, "XF86Home"))
       {
          cbhmd_drawer_hide(ad);
          return ECORE_CALLBACK_PASS_ON;
       }

   return ECORE_CALLBACK_DONE;
}

// Access type callback for title close button
static char *
__drawer_title_close_btn_access_type_cb(void *data, Evas_Object *obj)
{
   Cbhmd_App_Data *ad = (Cbhmd_App_Data *)data;
   if (!ad)
     return NULL;

   return strdup("Button");
}

// Access information callback for title close button
static char *
__drawer_title_close_btn_access_info_cb(void *data, Evas_Object *obj)
{
   Cbhmd_App_Data *ad = (Cbhmd_App_Data *)data;
   if (!ad)
     return NULL;

   return strdup(S_CLOSE);
}

// Access type callback for title delete button
static char *
__drawer_title_delete_btn_access_type_cb(void *data, Evas_Object *obj)
{
   Cbhmd_App_Data *ad = (Cbhmd_App_Data *)data;
   if (!ad)
     return NULL;

   return strdup("Button");
}

// Access information callback for title delete button
static char *
__drawer_title_delete_btn_access_info_cb(void *data, Evas_Object *obj)
{
   Cbhmd_App_Data *ad = (Cbhmd_App_Data *)data;
   if (!ad)
     return NULL;

   if (delete_mode)
     return strdup(S_DONE);
   else return strdup(S_DELETE);
}

// Access type callback for title delete all button
static char *
__drawer_title_delete_all_btn_access_type_cb(void *data, Evas_Object *obj)
{
   Cbhmd_App_Data *ad = (Cbhmd_App_Data *)data;
   if (!ad)
     return NULL;

   return strdup("Button");
}

// Access information callback for title delete all button
static char *
__drawer_title_delete_all_btn_access_info_cb(void *data, Evas_Object *obj)
{
   Cbhmd_App_Data *ad = (Cbhmd_App_Data *)data;
   if (!ad)
     return NULL;

   return strdup(S_DELETE_ALL);
}

// Access activate callback for title close button
static void __drawer_title_close_btn_access_activate_cb(void *data,
                                                        Evas_Object *obj, Elm_Object_Item *item)
{
   Cbhmd_App_Data *ad = (Cbhmd_App_Data *)data;
   if (!ad)
     return;

   cbhmd_drawer_hide(ad);
}

// Access activate callback for title delete button
static void __drawer_title_delete_btn_access_activate_cb(void *data,
                                                         Evas_Object *obj, Elm_Object_Item *item)
{
   Cbhmd_App_Data *ad = (Cbhmd_App_Data *)data;
   if (!ad)
     return;

   cbhmd_drawer_delete_mode_set(ad, !delete_mode);
}

// Access activate callback for title delete all button
static void __drawer_title_delete_all_btn_access_activate_cb(void *data,
                                                             Evas_Object *obj, Elm_Object_Item *item)
{
   FN_CALL();
   int ret;
   Cbhmd_App_Data *ad = (Cbhmd_App_Data *)data;

   RET_IF(NULL == ad);

   Cbhmd_Drawer_Data *dd = ad->drawer;

   if ((item_count_get(ad, ATOM_INDEX_COUNT_ALL) - dd->locked_item_count) != 0)
     {
        ret = _cbhmd_drawer_popup_create(ad);
        if (CBHM_ERROR_NONE != ret)
          {
             ERR("_cbhmd_drawer_popup_create() Fail(%d)", ret);
             return;
          }
     }
}

static int _cbhmd_drawer_effect_and_focus_set(Cbhmd_App_Data *ad)
{
   FN_CALL();
   int ret;

   RETV_IF(NULL == ad, CBHM_ERROR_INVALID_PARAMETER);

#ifdef HAVE_X11
   ret = cbhmd_x_drawer_effect_and_focus_set(ad);
   if (CBHM_ERROR_NONE != ret)
     {
        ERR("cbhmd_x_drawer_effect_and_focus_set() Fail(%d)", ret);
        return ret;
     }
#endif
#ifdef HAVE_WAYLAND
   ret = cbhmd_wl_drawer_effect_and_focus_set(ad);
   if (CBHM_ERROR_NONE != ret)
     {
        ERR("cbhmd_wl_drawer_effect_and_focus_set() Fail(%d)", ret);
        return ret;
     }
#endif

   return CBHM_ERROR_NONE;
}

static Eina_Bool __drawer_item_add(Cbhmd_App_Data *ad, Cbhmd_Cnp_Item *item)
{
   Eina_Bool duplicated = EINA_FALSE;
   Cbhmd_Drawer_Data *dd = ad->drawer;
   Cbhmd_Cnp_Item *gitem_data = NULL;
   Eina_List *l = NULL;
   Eina_List *l_next = NULL;
   Eina_List *itemlist = eina_list_next(ad->item_list);

   if (!item)
     return EINA_FALSE;

   EINA_LIST_FOREACH_SAFE(itemlist, l, l_next, gitem_data)
     {
        if ((gitem_data->type_index == item->type_index)
            && (!SAFE_STRCMP(item->data, gitem_data->data)))
          {
             DBG("duplicated data = %s", (char *)item->data);
             duplicated = EINA_TRUE;
             item_delete_by_Cbhmd_Cnp_Item(ad, gitem_data);
             break;
          }
     }

   if (item->locked)
     dd->locked_item_count++;

   if (item->gitem_style == GRID_ITEM_STYLE_TEXT)
     item->gitem = elm_gengrid_item_prepend(dd->gengrid, &dd->gic_text, item,
                                            __drawer_gengrid_select_cb, item);
   else
     if (item->gitem_style == GRID_ITEM_STYLE_IMAGE)
       item->gitem = elm_gengrid_item_prepend(dd->gengrid, &dd->gic_image,
                                              item, __drawer_gengrid_select_cb, item);
     else item->gitem = elm_gengrid_item_prepend(dd->gengrid,
                                                 &dd->gic_combined, item, __drawer_gengrid_select_cb, item);

   return duplicated;
}

static Eina_Bool __drawer_item_del(Cbhmd_App_Data *ad, Cbhmd_Cnp_Item *item)
{
   if (item->gitem)
     {
        elm_object_item_del(item->gitem);
        item->gitem = NULL;
     }
   return EINA_TRUE;
}

static char *__drawer_grid_text_get(void *data, Evas_Object *obj,
                                    const char *part)
{
   FN_CALL();
   Cbhmd_Cnp_Item *item = data;
   char text_to_show[BUFF ];
   if (!item)
     return NULL;

   if (!SAFE_STRCMP(part, "elm.text")) /* text */
     {
        char *entry_text = cbhmd_convert_get_entry_string(item->ad,
                                                          item->type_index, item->data);

        /* limiting the grid text to show to avoid UI blockage in case of large text */
        if (entry_text)
          {
             SAFE_STRNCPY(text_to_show, entry_text, (BUFF - 1));
             strncat(text_to_show, "\0", 1);
          }

        if (entry_text)
          return strdup(text_to_show);
        else return SAFE_STRDUP(item->data);
     }
   return NULL;
}

static Evas_Object *__drawer_grid_content_get(void *data, Evas_Object *obj,
                                              const char *part)
{
   Cbhmd_Cnp_Item *item = data;

   if (!SAFE_STRCMP(part, "delbtn/img")) /* text */
     {
        Evas_Object *btn = elm_button_add(obj);
        elm_object_style_set(btn, "delete_icon");
        evas_object_propagate_events_set(btn, EINA_FALSE);
        evas_object_smart_callback_add(btn, "clicked",
                                       __drawer_grid_item_btn_clicked_cb, item);
        return btn;
     }
   else return NULL;
}

static Evas_Object *__drawer_grid_image_content_get(void *data,
                                                    Evas_Object *obj, const char *part)
{
   Cbhmd_Cnp_Item *item = data;
   Cbhmd_App_Data *ad;
   Cbhmd_Drawer_Data *dd;
   Elm_Object_Item *gitem;
   Evas_Object *sicon;

   if (!item)
     return NULL;

   ad = item->ad;
   dd = ad->drawer;
   gitem = item->gitem;

   if (!SAFE_STRCMP(part, "elm.swallow.content"))
     { /* uri */
        int w, h, iw, ih;
        int grid_image_real_w = dd->grid_image_item_w;
        int grid_image_real_h = dd->grid_image_item_h;
        //double scale;

        sicon = evas_object_image_filled_add(evas_object_evas_get(obj));
        evas_object_image_load_size_set(sicon, grid_image_real_w,
                                        grid_image_real_h);
        evas_object_image_file_set(sicon, item->file, NULL);
        evas_object_image_preload(sicon, EINA_FALSE);
        evas_object_image_size_get(sicon, &w, &h);

        //scale = elm_config_scale_get();

        if (w <= 0 || h <= 0)
          return NULL;

        if (w > grid_image_real_w || h > grid_image_real_h)
          {
             if (w > h)
               {
                  iw = ELM_SCALE_SIZE(240);
                  ih = ELM_SCALE_SIZE(159);
               }
             else
               if (w < h)
                 {
                    iw = ELM_SCALE_SIZE(104);
                    ih = ELM_SCALE_SIZE(159);
                 }
               else
                 {
                    iw = ELM_SCALE_SIZE(159);
                    ih = ELM_SCALE_SIZE(159);
                 }
          }
        else
          {
             iw = w;
             ih = h;
          }

        evas_object_resize(sicon, iw, ih);
        evas_object_size_hint_min_set(sicon, iw, ih);

        if (dd->paste_text_only)
          elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
        else elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
     }
   else
     if (!SAFE_STRCMP(part, "delbtn/img"))
       { /* text */
          Evas_Object *btn = elm_button_add(obj);
          elm_object_style_set(btn, "delete_icon");
          evas_object_propagate_events_set(btn, EINA_FALSE);
          evas_object_smart_callback_add(btn, "clicked",
                                         __drawer_grid_item_btn_clicked_cb, item);
          return btn;
       }
     else return NULL;

   return sicon;
}

static Evas_Object *__drawer_grid_combined_content_get(void *data,
                                                       Evas_Object *obj, const char *part)
{
   Cbhmd_Cnp_Item *item = data;
   Cbhmd_App_Data *ad;
   Cbhmd_Drawer_Data *dd;
   Elm_Object_Item *gitem;
   Evas_Object *sicon;
   char *entry_text = NULL;

   if (!item)
     return NULL;

   ad = item->ad;
   dd = ad->drawer;
   gitem = item->gitem;

   if (!SAFE_STRCMP(part, "elm.swallow.content")) /* uri */
     {
        int w, h, iw, ih;
        int grid_image_real_w = dd->grid_image_item_w;
        int grid_image_real_h = dd->grid_image_item_h;

        sicon = evas_object_image_filled_add(evas_object_evas_get(obj));
        evas_object_image_load_size_set(sicon, grid_image_real_w,
                                        grid_image_real_h);
        evas_object_image_file_set(sicon, item->file, NULL);
        evas_object_image_preload(sicon, EINA_FALSE);
        evas_object_image_size_get(sicon, &w, &h);

        if (w <= 0 || h <= 0)
          return NULL;

        if (w > COMBINED_ITEM_IMAGE_WIDTH || h > COMBINED_ITEM_IMAGE_HEIGHT)
          {
             if (h >= w)
               {
                  iw = (float)grid_image_real_h / h * w;
                  if (iw > COMBINED_ITEM_IMAGE_WIDTH)
                    {
                       ih = (float)COMBINED_ITEM_IMAGE_WIDTH / w * h;
                       iw = COMBINED_ITEM_IMAGE_WIDTH;
                    }
                  else
                    {
                       ih = COMBINED_ITEM_IMAGE_HEIGHT;
                    }
               }
             else
               {
                  ih = (float)COMBINED_ITEM_IMAGE_WIDTH / w * h;
                  iw = COMBINED_ITEM_IMAGE_WIDTH;
               }
          }
        else
          {
             iw = w;
             ih = h;
          }

        evas_object_resize(sicon, iw, ih);
        evas_object_size_hint_min_set(sicon, iw, ih);
        entry_text = cbhmd_convert_get_entry_string(item->ad, item->type_index,
                                                    item->data);

        if (entry_text)
          {
             entry_text = evas_textblock_text_markup_to_utf8(NULL, entry_text);
             if (entry_text)
               SAFE_STRNCAT(entry_text, "\0", 1);
          }

        if (dd->paste_text_only && entry_text && !strlen(entry_text))
          elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
        else elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
        if (entry_text)
          free(entry_text);
     }
   else
     if (!SAFE_STRCMP(part, "delbtn/img"))
       { /* text */
          Evas_Object *btn = elm_button_add(obj);
          elm_object_style_set(btn, "delete_icon");
          evas_object_propagate_events_set(btn, EINA_FALSE);
          evas_object_smart_callback_add(btn, "clicked",
                                         __drawer_grid_item_btn_clicked_cb, item);
          return btn;
       }
     else return NULL;

   return sicon;
}

static void __drawer_grid_realized_cb(void *data, Evas_Object *obj,
                                      void *event_info)
{
   Elm_Object_Item *gitem = (Elm_Object_Item *)event_info;
   //  Cbhmd_Cnp_Item *item;

   if (gitem)
     {
        //      item = elm_object_item_data_get(gitem);

        if (delete_mode)
          elm_object_item_signal_emit(gitem, "elm,state,show,delbtn", "elm");
        else elm_object_item_signal_emit(gitem, "elm,state,hide,delbtn", "elm");
     }
}

static void __drawer_ok_btn_clicked_cb(void *data, Evas_Object *obj,
                                       void *event_info)
{
   Cbhmd_App_Data *ad = data;
   Cbhmd_Drawer_Data *dd = ad->drawer;
   Elm_Object_Item *gitem = elm_gengrid_first_item_get(dd->gengrid);

   while (gitem)
     {
        Cbhmd_Cnp_Item *gitem_data = elm_object_item_data_get(gitem);
        gitem = elm_gengrid_item_next_get(gitem);

        if (!gitem_data->locked)
          item_delete_by_Cbhmd_Cnp_Item(ad, gitem_data);
     }

   if (item_count_get(ad, ATOM_INDEX_COUNT_ALL) == 0)
     cbhmd_drawer_hide(ad);
   else
     {
        dd->popup_activate = EINA_FALSE;
        evas_object_del(dd->popup_conform);
        evas_object_del(dd->popup_win);
     }
}

static void __drawer_cancel_btn_clicked_cb(void *data, Evas_Object *obj,
                                           void *event_info)
{
   Cbhmd_App_Data *ad = data;
   Cbhmd_Drawer_Data *dd = ad->drawer;

   dd->popup_activate = EINA_FALSE;
   evas_object_del(dd->popup_conform);
   evas_object_del(dd->popup_win);
}

static void __drawer_grid_item_btn_clicked_cb(void *data, Evas_Object *obj,
                                              void *event_info)
{
   Cbhmd_Cnp_Item *item = data;
   Cbhmd_App_Data *ad = item->ad;
   Cbhmd_Drawer_Data *dd = ad->drawer;

   if (dd->anim_status != STATUS_NONE)
     return;

   item_delete_by_Cbhmd_Cnp_Item(ad, item);
}

static void __drawer_layout_clicked_cb(void *data, Evas_Object *obj,
                                       const char *emission, const char *source)
{
   int ret;
   Cbhmd_App_Data *ad = data;

   if (ad->drawer->anim_status != STATUS_NONE)
     return;

   if (!SAFE_STRNCMP(source, EDJE_CLOSE_PART_PREFIX,
                     SAFE_STRLEN(EDJE_CLOSE_PART_PREFIX)))
     {
        cbhmd_drawer_hide(ad);
     }
   else
     if (!SAFE_STRNCMP(source, EDJE_DELETE_MODE_PREFIX,
                       SAFE_STRLEN(EDJE_DELETE_MODE_PREFIX)))
       {
          cbhmd_drawer_delete_mode_set(ad, !delete_mode);
       }
     else
       if (!SAFE_STRNCMP(source, EDJE_DELETE_ALL_BTN_PART_PREFIX,
                         SAFE_STRLEN(EDJE_DELETE_ALL_BTN_PART_PREFIX)))
         {
            ret = _cbhmd_drawer_popup_create(ad);
            if (CBHM_ERROR_NONE != ret)
              {
                 ERR("_cbhmd_drawer_popup_create() Fail(%d)", ret);
              }
         }
       else
         {
            DBG("ignore source(%s)", source);
         }
}

static int _cbhmd_drawer_popup_create(Cbhmd_App_Data *ad)
{
   FN_CALL();
   int w = 0, h = 0;
   Evas_Object *btn1;
   Evas_Object *btn2;
   Cbhmd_Drawer_Data *dd;
   int rotations[4] = {0, 90, 180, 270};

   RETV_IF(NULL == ad, CBHM_ERROR_INVALID_PARAMETER);
   RETV_IF(NULL == ad->drawer, CBHM_ERROR_INVALID_PARAMETER);

   dd = ad->drawer;

   if (dd->popup_activate == EINA_TRUE)
     return CBHM_ERROR_NONE;

   dd->popup_activate = EINA_TRUE;

   if (!(dd->popup_win = elm_win_add(NULL, "delete popup", ELM_WIN_MENU)))
     {
        ERR("elm_win_add() Fail");
        return CBHM_ERROR_NO_DATA;
     }

#ifdef HAVE_X11
   ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
#endif
   evas_object_resize(dd->popup_win, w, h);
   elm_win_alpha_set(dd->popup_win, EINA_TRUE);

#ifdef HAVE_X11
   ecore_x_icccm_name_class_set(elm_win_xwindow_get(dd->popup_win),
                                "APP_POPUP", "APP_POPUP");
#endif

   _cbhmd_drawer_evas_object_focus_set(dd->popup_win, EINA_FALSE);

#ifdef HAVE_X11
   cbhmd_x_drawer_set_transient(elm_win_xwindow_get(dd->popup_win),
                                dd->x_main_win);
#endif
#ifdef HAVE_WAYLAND
   cbhmd_wl_drawer_set_transient(elm_win_wl_window_get(dd->popup_win),
                                 dd->wl_main_win);
#endif

   elm_win_wm_rotation_available_rotations_set(dd->popup_win, rotations, 4);
   evas_object_show(dd->popup_win);

   dd->popup_conform = elm_conformant_add(dd->popup_win);
   evas_object_size_hint_weight_set(dd->popup_conform, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   elm_win_resize_object_add(dd->popup_win, dd->popup_conform);
   evas_object_show(dd->popup_conform);

   elm_win_conformant_set(dd->popup_win, EINA_TRUE);

   dd->cbhm_popup = elm_popup_add(dd->popup_win);

   elm_object_part_text_set(dd->cbhm_popup, "title,text", S_DELETE_Q);
   elm_object_text_set(dd->cbhm_popup, S_DELETE_ALL_Q);

   btn1 = elm_button_add(dd->cbhm_popup);
   elm_object_style_set(btn1, "popup");
   elm_object_text_set(btn1, S_CANCEL);
   elm_object_part_content_set(dd->cbhm_popup, "button1", btn1);
   evas_object_smart_callback_add(btn1, "clicked",
                                  __drawer_cancel_btn_clicked_cb, ad);

   btn2 = elm_button_add(dd->cbhm_popup);
   elm_object_style_set(btn2, "popup");
   elm_object_text_set(btn2, S_DELETE);
   elm_object_part_content_set(dd->cbhm_popup, "button2", btn2);
   evas_object_smart_callback_add(btn2, "clicked", __drawer_ok_btn_clicked_cb,
                                  ad);

   evas_object_show(dd->cbhm_popup);

   return CBHM_ERROR_NONE;
}

static void _cbhmd_drawer_evas_object_focus_set(Evas_Object *obj,
                                                 Eina_Bool enable)
{
#ifdef HAVE_X11
   cbhmd_x_drawer_focus_set(elm_win_xwindow_get(obj), enable);
#endif
#ifdef HAVE_WAYLAND
   cbhmd_wl_drawer_focus_set(elm_win_wl_window_get(obj), enable);
#endif
}

void cbhmd_drawer_focus_set(Cbhmd_App_Data *ad, Eina_Bool enable)
{
   Cbhmd_Drawer_Data *dd;

   RET_IF(NULL == ad);

   if (!ad->drawer)
     {
        DBG("drawer is not created yet");
        return;
     }

   RET_IF(NULL == ad->drawer->wl_main_win);

   dd = ad->drawer;

#ifdef HAVE_X11
   cbhmd_x_drawer_focus_set(dd->x_main_win, enable);
#endif
#ifdef HAVE_WAYLAND
   cbhmd_wl_drawer_focus_set(dd->wl_main_win, enable);
#endif
}

static Evas_Object* _cbhmd_drawer_win_add(Cbhmd_Drawer_Data *dd,
                                           const char *name)
{
   Evas_Object *win = elm_win_add(NULL, name, ELM_WIN_UTILITY);
   if (!win)
     {
        ERR("elm_win_add() Fail");
        return NULL;
     }
   elm_win_title_set(win, name);
   elm_win_borderless_set(win, EINA_TRUE);
#ifdef HAVE_X11
   ecore_x_window_size_get(ecore_x_window_root_first_get(), &dd->root_w,
                           &dd->root_h);
#endif
#ifdef HAVE_WAYLAND
   ecore_wl_screen_size_get(&dd->root_w, &dd->root_h);
#endif
   DBG("root_w: %d, root_h: %d", dd->root_w, dd->root_h);
   //evas_object_resize(win, dd->root_w, dd->root_h);
   //elm_config_scale_set((double)dd->root_w/DEFAULT_WIDTH);

   return win;
}

static void _cbhmd_drawer_set_geometry(Cbhmd_App_Data *ad)
{
   FN_CALL();
   Cbhmd_Drawer_Data *dd = ad->drawer;
   Evas_Coord x, y, w, h;
   int angle = elm_win_rotation_get(dd->main_win);

#ifdef HAVE_X11
   if (!ad->x_active_win)
     {
        ERR("x_active_win is NULL");
        return;
     }
#endif
#ifdef HAVE_WAYLAND
   if (!ad->wl_active_win)
     {
        //      ERR("wl_active_win is NULL");
        return;
     }
#endif

   if (angle == 90 || angle == 270)
     {
        h = dd->landscape_height;
        x = 0;
        y = dd->root_w - h;
        w = dd->root_h;
     }
   else
     {
        h = dd->height;
        x = 0;
        y = dd->root_h - h;
        w = dd->root_w;
     }

   if (!h)
     w = 0;

   DBG("[CBHM] change degree geometry... (%d, %d, %d x %d)", x, y, w, h);

#ifdef HAVE_X11
   ecore_x_e_illume_clipboard_geometry_set(ad->x_active_win, x, y, w, h);
#endif
#ifdef HAVE_WAYLAND
   ecore_wl_window_clipboard_geometry_set(ad->wl_active_win, x, y, w, h);
#endif
}

void cbhmd_drawer_set_rotation(Cbhmd_App_Data *ad)
{
   FN_CALL();
   Cbhmd_Drawer_Data *dd = ad->drawer;
   int angle = elm_win_rotation_get(dd->main_win);
   int x, y, w, h;

   if (angle == 180) // reverse
     {
        h = dd->height;
        x = 0;
        y = 0;
        w = dd->root_w;
     }
   else
     if (angle == 90) // right rotate
       {
          h = dd->landscape_height;
          x = dd->root_w - h;
          y = 0;
          w = dd->root_h;
       }
     else
       if (angle == 270) // left rotate
         {
            h = dd->landscape_height;
            x = 0;
            y = 0;
            w = dd->root_h;
         }
       else // angle == 0
         {
            h = dd->height;
            x = 0;
            y = dd->root_h - h;
            w = dd->root_w;
         }

   evas_object_resize(dd->main_win, w, h);
   evas_object_move(dd->main_win, x, y);
   _cbhmd_drawer_set_geometry(ad);
}
/*
   static Eina_Bool _get_anim_pos(Cbhmd_Drawer_Data *dd, int *sp, int *ep)
   {
   if (!sp || !ep)
   return EINA_FALSE;

   int angle = dd->o_degree;
   int anim_start, anim_end;

   if (angle == 180) // reverse
   {
   anim_start = -(dd->root_h - dd->height);
   anim_end = 0;
   }
   else if (angle == 90) // right rotate
   {
   anim_start = dd->root_w;
   anim_end = anim_start - dd->landscape_height;
   }
   else if (angle == 270) // left rotate
   {
   anim_start = -(dd->root_w - dd->landscape_height);
   anim_end = 0;
   }
   else // angle == 0
   {
   anim_start = dd->root_h;
   anim_end = anim_start - dd->height;
   }

 *sp = anim_start;
 *ep = anim_end;
 return EINA_TRUE;
 }

 static Eina_Bool _do_anim_delta_pos(Cbhmd_Drawer_Data *dd, int sp, int ep, int ac, int *dp)
 {
 if (!dp)
 return EINA_FALSE;

 int angle = dd->o_degree;
 int delta;
 double posprop;
 posprop = 1.0*ac/ANIM_DURATION;

 if (angle == 180) // reverse
 {
 delta = (int)((ep-sp)*posprop);
 evas_object_move(dd->main_win, 0, sp+delta);
 }
 else if (angle == 90) // right rotate
 {
 delta = (int)((ep-sp)*posprop);
 evas_object_move(dd->main_win, sp+delta, 0);
 }
 else if (angle == 270) // left rotate
 {
 delta = (int)((ep-sp)*posprop);
 evas_object_move(dd->main_win, sp+delta, 0);
 }
 else // angle == 0
 {
 delta = (int)((sp-ep)*posprop);
 evas_object_move(dd->main_win, 0, sp-delta);
 }

 *dp = delta;

 return EINA_TRUE;
 }

static void stop_animation(Cbhmd_App_Data *ad)
{
   FN_CALL();
   Cbhmd_Drawer_Data *dd = ad->drawer;
   dd->anim_status = STATUS_NONE;
   if (dd->anim_timer)
     {
        ecore_timer_del(dd->anim_timer);
        dd->anim_timer = NULL;
     }

   _cbhmd_drawer_set_geometry(ad);
}

static Eina_Bool anim_pos_calc_cb(void *data)
{
   Cbhmd_App_Data *ad = data;
   Cbhmd_Drawer_Data *dd = ad->drawer;
   int anim_start, anim_end, delta;

   _get_anim_pos(dd, &anim_start, &anim_end);

   if (dd->anim_status == SHOW_ANIM)
     {
        if (dd->anim_count > ANIM_DURATION)
          {
             dd->anim_count = ANIM_DURATION;
             stop_animation(ad);
             return EINA_FALSE;
          }
        _do_anim_delta_pos(dd, anim_start, anim_end, dd->anim_count, &delta);
        if (dd->anim_count == 1)
          evas_object_show(dd->main_win);
        dd->anim_count++;
     }
   else if (dd->anim_status == HIDE_ANIM)
     {
        if (dd->anim_count < 0)
          {
             dd->anim_count = 0;
             elm_object_signal_emit(dd->main_layout, "elm,state,hide,historyitems", "elm");
             edje_object_message_signal_process(elm_layout_edje_get(dd->main_layout));
             evas_object_hide(dd->main_win);
             elm_win_lower(dd->main_win);
             cbhmd_x_drawer_unset_transient(dd->x_main_win);
             stop_animation(ad);
             cbhmd_drawer_delete_mode_set(ad, EINA_FALSE);
             return EINA_FALSE;
          }
        _do_anim_delta_pos(dd, anim_start, anim_end, dd->anim_count, &delta);
        dd->anim_count--;
     }
   else
     {
        stop_animation(ad);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static Eina_Bool clipdrawer_anim_effect(Cbhmd_App_Data *ad, AnimStatus atype)
{
   FN_CALL();
   Cbhmd_Drawer_Data *dd = ad->drawer;
   if (atype == dd->anim_status)
     {
        WARN("Warning: Animation effect is already in progress.");
        return EINA_FALSE;
     }

   dd->anim_status = atype;

   if (dd->anim_timer)
     ecore_timer_del(dd->anim_timer);
   dd->anim_timer = ecore_timer_add(ANIM_FLOPS, anim_pos_calc_cb, ad);

   return EINA_TRUE;
}
*/

static Eina_Bool _cbhmd_drawer_gengrid_show_timer_cb(void *data)
{
   Cbhmd_Drawer_Data *dd = data;
   /* If the gengrid has some images, evas would have some heavy works for
    * image drawings. In this case, window would be delayed in showing up. In
    * order to avoid this problem, we show the gengrid after window is shown.
    */
   elm_object_signal_emit(dd->main_layout, "elm,state,show,historyitems",
                          "elm");
   return ECORE_CALLBACK_CANCEL;
}

void __drawer_wm_rotation_changed_cb(void *data, Evas_Object * obj, void *event)
{
   if (!data)
     return;

   Cbhmd_App_Data *ad = data;

   cbhmd_drawer_set_rotation(ad);
}

void cbhmd_drawer_delete_mode_set(Cbhmd_App_Data* ad, Eina_Bool del_mode)
{
   Cbhmd_Drawer_Data *dd = ad->drawer;
   Elm_Object_Item *gitem = elm_gengrid_first_item_get(dd->gengrid);
   Cbhmd_Cnp_Item *item = NULL;

   if (gitem)
     delete_mode = del_mode;
   else delete_mode = EINA_FALSE;

   if (!dd->main_layout)
     return;
   if (delete_mode)
     {
        elm_object_part_text_set(dd->main_layout, "panel_function_delete",
                                 S_DONE);
        elm_object_signal_emit(dd->main_layout, "elm,state,hide,delete_all",
                               "elm");
     }
   else
     {
        elm_object_part_text_set(dd->main_layout, "panel_function_delete",
                                 S_DELETE);

        if (item_count_get(ad, ATOM_INDEX_COUNT_ALL) == 0)
          {
             elm_object_signal_emit(dd->main_layout, "elm,state,show,delete_all",
                                    "elm");
             elm_object_signal_emit(dd->main_layout, "elm,state,disable,del",
                                    "elm");
             elm_object_part_content_unset(dd->main_layout, "historyitems");
             evas_object_hide(dd->gengrid);
             elm_object_part_content_set(dd->main_layout, "historyitems",
                                         dd->noc_layout);
          }
        else elm_object_signal_emit(dd->main_layout, "elm,state,show,delete_all",
                                    "elm");
     }

   while (gitem)
     {
        item = elm_object_item_data_get(gitem);
        if (!item)
          return;

        if (!item->locked)
          {
             if (delete_mode)
               elm_object_item_signal_emit(gitem, "elm,state,show,delbtn", "elm");
             else elm_object_item_signal_emit(gitem, "elm,state,hide,delbtn",
                                              "elm");
          }

        gitem = elm_gengrid_item_next_get(gitem);
     }
}

int cbhmd_drawer_event_window_create(Cbhmd_App_Data *ad)
{
   FN_CALL();

   if (!ad)
     return CBHM_ERROR_INVALID_PARAMETER;

#ifdef HAVE_X11
   int ret = cbhmd_x_drawer_event_window_create(ad);
   if (CBHM_ERROR_NONE != ret)
     {
        ERR("cbhmd_x_drawer_event_window_create() Fail(%d)", ret);
        return ret;
     }
#endif
#ifdef HAVE_WAYLAND
   return CBHM_ERROR_NONE;
#endif
   return CBHM_ERROR_NOT_SUPPORTED;
}

void cbhmd_drawer_event_window_set_title(Cbhmd_App_Data *ad)
{
#ifdef HAVE_X11
   cbhmd_x_drawer_event_window_set_title(ad->x_event_win, ad->x_root_win);
#endif
#ifdef HAVE_WAYLAND
   cbhmd_wl_drawer_event_window_set_title(ad->wl_event_win);
#endif
}

int cbhmd_drawer_display_init(Cbhmd_App_Data *ad)
{
#ifdef HAVE_X11
   if (!ecore_x_init(ad->x_disp))
     {
        ERR("ecore_x_init() Fail");
        return CBHM_ERROR_CONNECTION_REFUSED;
     }
#endif
#ifdef HAVE_WAYLAND
   if (!ecore_wl_init(ad->wl_disp))
     {
        ERR("ecore_wl_init() Fail");
        return CBHM_ERROR_CONNECTION_REFUSED;
     }
#endif

   return CBHM_ERROR_NONE;
}

void cbhmd_drawer_show(Cbhmd_App_Data* ad)
{
   FN_CALL();
   Cbhmd_Drawer_Data *dd = ad->drawer;
#ifdef HAVE_X11
   Ecore_X_Window x_transient_win = ad->x_active_win;
   Ecore_X_Window x_isf_ise_win = 0;
   Ecore_X_Virtual_Keyboard_State isf_ise_state;
#endif
#ifdef HAVE_WAYLAND
   /* FIXME : error: storage size of 'wl_transient_win' isn't known */
   //  Ecore_Wl_Window wl_transient_win = ad->wl_active_win;
#endif
#ifdef HAVE_WAYLAND
   Ecore_Wl_Window *wl_isf_ise_win;
   Ecore_Wl_Virtual_Keyboard_State isf_ise_state;
   Ecore_Wl_Input *input = NULL;
   const char *types[10] =
     {  0,};
   int i = -1;
#endif
   int rotations[4] = {0, 90, 180, 270};

   if (dd->main_layout)
     {
        int item_count = item_count_get(ad, ATOM_INDEX_COUNT_ALL)
           - dd->locked_item_count;
        if (0 == item_count)
          {
             elm_object_signal_emit(dd->main_layout, "elm,state,disable,del",
                                    "elm");
          }

        elm_object_part_text_set(dd->main_layout, "panel_title", S_CLIPBOARD);
        elm_object_part_text_set(dd->main_layout, "panel_function_delete_all",
                                 S_DELETE_ALL);
        elm_object_part_text_set(dd->main_layout, "panel_function_delete",
                                 S_DELETE);
     }

   if (dd->main_win)
     {
#ifdef HAVE_X11
        isf_ise_state = ecore_x_e_virtual_keyboard_state_get(ad->x_active_win);
        if (isf_ise_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON)
          {
             x_isf_ise_win = cbhmd_x_drawer_isf_ise_window_get();
             if (x_isf_ise_win)
               {
                  x_transient_win = x_isf_ise_win;
                  ecore_x_e_illume_clipboard_state_set(x_transient_win,
                                                       ECORE_X_ILLUME_CLIPBOARD_STATE_ON);
               }
          }
        cbhmd_x_drawer_set_transient(dd->x_main_win, x_transient_win);
#endif
#ifdef HAVE_WAYLAND
        isf_ise_state = ecore_wl_window_keyboard_state_get(ad->wl_active_win);
        if (isf_ise_state == ECORE_WL_VIRTUAL_KEYBOARD_STATE_ON)
          {
             wl_isf_ise_win = cbhmd_wl_drawer_isf_ise_window_get();
             if (wl_isf_ise_win)
               {
                  ecore_wl_window_clipboard_state_set(wl_isf_ise_win, EINA_TRUE);
               }
          }
        /* FIXME : "transient" relation should be set */
        //      cbhmd_wl_drawer_set_transient(dd->wl_main_win, wl_transient_win);
#endif

        elm_win_wm_rotation_available_rotations_set(dd->main_win, rotations, 4);
        evas_object_smart_callback_add(dd->main_win, "wm,rotation,changed",
                                       __drawer_wm_rotation_changed_cb, ad);
        cbhmd_drawer_delete_mode_set(ad, EINA_FALSE);
        cbhmd_drawer_set_rotation(ad);
        evas_object_show(dd->main_win);
        elm_win_activate(dd->main_win);
        INFO("clipboard window is shown");

#ifdef HAVE_X11
        ecore_x_e_illume_clipboard_state_set(ad->x_active_win,
                                             ECORE_X_ILLUME_CLIPBOARD_STATE_ON);
        utilx_grab_key(ad->x_disp, dd->x_main_win, "XF86Back",
                       TOP_POSITION_GRAB);
        utilx_grab_key(ad->x_disp, dd->x_main_win, "XF86Home", SHARED_GRAB);
#endif
#ifdef HAVE_WAYLAND
        ecore_wl_window_clipboard_state_set(ad->wl_active_win, EINA_TRUE);
        /* FIXME: review the parameters */
        ecore_wl_window_keygrab_set(ad->wl_active_win, "XF86BACK", 0, 0, 0,
                                    ECORE_WL_WINDOW_KEYGRAB_TOPMOST);
        ecore_wl_window_keygrab_set(ad->wl_active_win, "XF86HOME", 0, 0, 0,
                                    ECORE_WL_WINDOW_KEYGRAB_TOPMOST);

        input = ecore_wl_input_get();
        /* FIXME: types support should re-written */
        types[++i] = "application/x-elementary-markup";
        ecore_wl_dnd_selection_set(input, types);

#endif
        /*0.125 is experimentally decided */
        ecore_timer_add(0.125, _cbhmd_drawer_gengrid_show_timer_cb, dd);
     }

   if (item_count_get(ad, ATOM_INDEX_COUNT_ALL) == 0)
     {
        elm_object_part_content_unset(dd->main_layout, "historyitems");
        evas_object_hide(dd->gengrid);
        elm_object_part_content_set(dd->main_layout, "historyitems",
                                    dd->noc_layout);
     }
   else
     {
        elm_object_part_content_unset(dd->main_layout, "historyitems");
        evas_object_hide(dd->noc_layout);
        elm_object_part_content_set(dd->main_layout, "historyitems", dd->gengrid);
     }
}

static Eina_Bool cbhmd_drawer_hide_timer_cb(void *data)
{
   FN_CALL();
   Cbhmd_App_Data *ad = (Cbhmd_App_Data *)data;
   Cbhmd_Drawer_Data *dd = ad->drawer;

   dd->lower_view_timer = NULL;

   if (dd->main_win)
     {
        elm_object_signal_emit(dd->main_layout, "elm,state,hide,historyitems",
                               "elm");
        edje_object_message_signal_process(elm_layout_edje_get(dd->main_layout));
        Elm_Object_Item *it = elm_gengrid_first_item_get(dd->gengrid);
        elm_gengrid_item_show(it, ELM_GENGRID_ITEM_SCROLLTO_NONE);
        evas_object_hide(dd->main_win);
        INFO("clipboard window is hidden");
        elm_win_lower(dd->main_win);
#ifdef HAVE_X11
        cbhmd_x_drawer_unset_transient(dd->x_main_win);
#endif
#ifdef HAVE_WAYLAND
        cbhmd_wl_drawer_unset_transient(dd->wl_main_win);
#endif
        cbhmd_drawer_delete_mode_set(ad, EINA_FALSE);
     }

   return ECORE_CALLBACK_CANCEL;
}

void cbhmd_drawer_hide(Cbhmd_App_Data* ad)
{
   FN_CALL();
   Cbhmd_Drawer_Data *dd = ad->drawer;
#ifdef HAVE_X11
   Ecore_X_Window x_isf_ise_win = 0;
   Ecore_X_Virtual_Keyboard_State isf_ise_state;
#endif

   if (dd->lower_view_timer)
     return;

   if (dd->main_win)
     {
#ifdef HAVE_X11
        ecore_x_e_illume_clipboard_state_set(ad->x_active_win,
                                             ECORE_X_ILLUME_CLIPBOARD_STATE_OFF);
        ecore_x_e_illume_clipboard_geometry_set(ad->x_active_win, 0, 0, 0, 0);

        isf_ise_state = ecore_x_e_virtual_keyboard_state_get(ad->x_active_win);
        if (isf_ise_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON)
          {
             x_isf_ise_win = cbhmd_x_drawer_isf_ise_window_get();

             if (x_isf_ise_win)
               ecore_x_e_illume_clipboard_state_set(x_isf_ise_win,
                                                    ECORE_X_ILLUME_CLIPBOARD_STATE_OFF);
          }

        utilx_ungrab_key(ad->x_disp, dd->x_main_win, "XF86Back");
        utilx_ungrab_key(ad->x_disp, dd->x_main_win, "XF86Home");
#endif
#ifdef HAVE_WAYLAND
        ecore_wl_window_clipboard_state_set(ad->wl_active_win, EINA_FALSE);
        ecore_wl_window_clipboard_geometry_set(ad->wl_active_win, 0, 0, 0, 0);
        Ecore_Wl_Virtual_Keyboard_State isf_ise_state;
        isf_ise_state = ecore_wl_window_keyboard_state_get(ad->wl_active_win);
        if (isf_ise_state == ECORE_WL_VIRTUAL_KEYBOARD_STATE_ON)
          {
             Ecore_Wl_Window *wl_isf_ise_win;
             wl_isf_ise_win = cbhmd_wl_drawer_isf_ise_window_get();
             ecore_wl_window_clipboard_state_set(wl_isf_ise_win, EINA_FALSE);
          }
        //TODO: review parameters
        ecore_wl_window_keygrab_unset(ad->wl_active_win, "XF86BACK", 0, 0);
        ecore_wl_window_keygrab_unset(ad->wl_active_win, "XF86HOME", 0, 0);
#endif

        if (dd->popup_activate)
          {
             dd->popup_activate = EINA_FALSE;
             evas_object_del(dd->popup_conform);
             evas_object_del(dd->popup_win);
          }
     }

   /* FIXME : This callback is sometimes not called. Does we need to call func
    * directly without using timer? */
   dd->lower_view_timer = ecore_timer_add(TIME_DELAY_LOWER_VIEW,
                                          cbhmd_drawer_hide_timer_cb, ad);
}

Cbhmd_Drawer_Data* cbhmd_drawer_init(Cbhmd_App_Data *ad)
{
   FN_CALL();
   Evas_Object* ly;
   const char *data;
   int ret = CBHM_ERROR_NONE;
   Evas_Object *part_obj, *access_obj;

   Cbhmd_Drawer_Data *dd = calloc(1, sizeof(Cbhmd_Drawer_Data));
   /* create and setting window */
   if (!dd)
     {
        ERR("calloc() Fail");
        return NULL;
     }

   if (!(dd->main_win = _cbhmd_drawer_win_add(dd, APPNAME)))
     {
        ERR("_cbhmd_drawer_win_add() Fail");
        free(dd);
        return NULL;
     }
#ifdef HAVE_X11
   if (!(dd->x_main_win = elm_win_xwindow_get(dd->main_win)))
     {
        ERR("elm_win_xwindow_get() Fail");
        free(dd);
        return NULL;
     }
#endif
#ifdef HAVE_WAYLAND
   if (!(dd->wl_main_win = elm_win_wl_window_get(dd->main_win)))
     {
        ERR("elm_win_wl_window_get() Fail");
        free(dd);
        return NULL;
     }
#endif

   ret = _cbhmd_drawer_effect_and_focus_set(ad);
   if (CBHM_ERROR_NONE != ret)
     {
        ERR("_cbhmd_drawer_effect_and_focus_set() Fail");
        free(dd);
        return NULL;
     }

   /* edj setting */
   ret = _cbhmd_drawer_edj_load(dd, APP_EDJ_FILE, GRP_MAIN);
   if (CBHM_ERROR_NONE != ret)
     {
        ERR("_cbhmd_drawer_edj_load() Fail");
        evas_object_del(dd->main_win);
        free(dd);
        return NULL;
     }

   //double scale = elm_config_scale_get();
   ly = elm_layout_edje_get(dd->main_layout);

   data = edje_object_data_get(ly, "clipboard_height");
   dd->height = data ? atoi(data) : 0;
   dd->height = ELM_SCALE_SIZE(dd->height);

   data = edje_object_data_get(ly, "clipboard_landscape_height");
   dd->landscape_height = data ? atoi(data) : 0;
   dd->landscape_height = ELM_SCALE_SIZE(dd->landscape_height);

   data = edje_object_data_get(ly, "grid_item_bg_w");
   dd->grid_item_bg_w = data ? atoi(data) : 0;
   dd->grid_item_bg_w = ELM_SCALE_SIZE(dd->grid_item_bg_w);

   data = edje_object_data_get(ly, "grid_item_bg_h");
   dd->grid_item_bg_h = data ? atoi(data) : 0;
   dd->grid_item_bg_h = ELM_SCALE_SIZE(dd->grid_item_bg_h);

   data = edje_object_data_get(ly, "grid_image_item_w");
   dd->grid_image_item_w = data ? atoi(data) : 0;
   dd->grid_image_item_w = ELM_SCALE_SIZE(dd->grid_image_item_w);

   data = edje_object_data_get(ly, "grid_image_item_h");
   dd->grid_image_item_h = data ? atoi(data) : 0;
   dd->grid_image_item_h = ELM_SCALE_SIZE(dd->grid_image_item_h);

   /* create and setting gengrid */
   elm_theme_extension_add(NULL, APP_EDJ_FILE);
   elm_object_signal_callback_add(dd->main_layout, "mouse,clicked,1", "*",
                                  __drawer_layout_clicked_cb, ad);

   /* Set accessibility function for title close button */
   part_obj = (Evas_Object *)edje_object_part_object_get(
      elm_layout_edje_get(dd->main_layout), EDJE_CLOSE_PART_PREFIX);
   access_obj = elm_access_object_register(part_obj, dd->main_layout);

   elm_access_info_cb_set(access_obj, ELM_ACCESS_TYPE,
                          __drawer_title_close_btn_access_type_cb, ad);
   elm_access_info_cb_set(access_obj, ELM_ACCESS_INFO,
                          __drawer_title_close_btn_access_info_cb, ad);
   elm_access_activate_cb_set(access_obj,
                              __drawer_title_close_btn_access_activate_cb, ad);

   /* Set access function for title delete button */
   part_obj = (Evas_Object *)edje_object_part_object_get(
      elm_layout_edje_get(dd->main_layout), EDJE_DELETE_MODE_PREFIX);
   access_obj = elm_access_object_register(part_obj, dd->main_layout);

   elm_access_info_cb_set(access_obj, ELM_ACCESS_TYPE,
                          __drawer_title_delete_btn_access_type_cb, ad);
   elm_access_info_cb_set(access_obj, ELM_ACCESS_INFO,
                          __drawer_title_delete_btn_access_info_cb, ad);
   elm_access_activate_cb_set(access_obj,
                              __drawer_title_delete_btn_access_activate_cb, ad);

   /* Set access function for title delete all button */
   part_obj = (Evas_Object *)edje_object_part_object_get(
      elm_layout_edje_get(dd->main_layout),
      EDJE_DELETE_ALL_BTN_PART_PREFIX);
   access_obj = elm_access_object_register(part_obj, dd->main_layout);

   elm_access_info_cb_set(access_obj, ELM_ACCESS_TYPE,
                          __drawer_title_delete_all_btn_access_type_cb, ad);
   elm_access_info_cb_set(access_obj, ELM_ACCESS_INFO,
                          __drawer_title_delete_all_btn_access_info_cb, ad);
   elm_access_activate_cb_set(access_obj,
                              __drawer_title_delete_all_btn_access_activate_cb, ad);

   dd->gengrid = elm_gengrid_add(dd->main_win);
   elm_object_style_set(dd->gengrid, "cbhm");
   elm_object_part_content_set(dd->main_layout, "historyitems", dd->gengrid);
   elm_gengrid_item_size_set(dd->gengrid, dd->grid_item_bg_w,
                             dd->grid_item_bg_h);
   elm_gengrid_align_set(dd->gengrid, 0.0, 0.0);
   elm_gengrid_horizontal_set(dd->gengrid, EINA_TRUE);
   //  elm_gengrid_bounce_set(dd->gengrid, EINA_TRUE, EINA_FALSE);
   elm_gengrid_multi_select_set(dd->gengrid, EINA_FALSE);
   //  evas_object_smart_callback_add(dd->gengrid, "selected", _grid_click_paste, ad);
   evas_object_size_hint_weight_set(dd->gengrid, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);

   elm_gengrid_clear(dd->gengrid);

   evas_object_smart_callback_add(dd->gengrid, "realized",
                                  __drawer_grid_realized_cb, NULL);

   dd->gic_image.item_style = "clipboard/image_style";
   dd->gic_image.func.text_get = NULL;
   dd->gic_image.func.content_get = __drawer_grid_image_content_get;
   dd->gic_image.func.state_get = NULL;
   dd->gic_image.func.del = NULL;

   dd->gic_text.item_style = "clipboard/text_style";
   dd->gic_text.func.text_get = __drawer_grid_text_get;
   dd->gic_text.func.content_get = __drawer_grid_content_get;
   dd->gic_text.func.state_get = NULL;
   dd->gic_text.func.del = NULL;

   dd->gic_combined.item_style = "clipboard/combined_style";
   dd->gic_combined.func.text_get = __drawer_grid_text_get;
   dd->gic_combined.func.content_get = __drawer_grid_combined_content_get;
   dd->gic_combined.func.state_get = NULL;
   dd->gic_combined.func.del = NULL;

   evas_object_show(dd->gengrid);

   ad->draw_item_add = __drawer_item_add;
   ad->draw_item_del = __drawer_item_del;

   dd->keydown_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                                 __drawer_keydown_cb, ad);
   dd->evas = evas_object_evas_get(dd->main_win);

   delete_mode = EINA_FALSE;
   dd->popup_activate = EINA_FALSE;
   dd->item_clicked = EINA_FALSE;
   dd->del_btn_clicked = EINA_FALSE;

   dd->lower_view_timer = NULL;

   dd->event_rect = evas_object_rectangle_add(
      evas_object_evas_get(dd->main_win));
   evas_object_size_hint_weight_set(dd->event_rect, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_color_set(dd->event_rect, 0, 0, 0, 0);
   elm_win_resize_object_add(dd->main_win, dd->event_rect);
   evas_object_repeat_events_set(dd->event_rect, EINA_TRUE);
   evas_object_show(dd->event_rect);
   evas_object_raise(dd->event_rect);

   dd->gesture_layer = elm_gesture_layer_add(dd->main_win);
   elm_gesture_layer_hold_events_set(dd->gesture_layer, EINA_FALSE);
   elm_gesture_layer_attach(dd->gesture_layer, dd->event_rect);
   elm_gesture_layer_cb_set(dd->gesture_layer, ELM_GESTURE_N_FLICKS,
                            ELM_GESTURE_STATE_END, __drawer_flick_end_cb, ad);

   dd->noc_layout = elm_layout_add(dd->main_win);
   elm_layout_theme_set(dd->noc_layout, "layout", "nocontents", "default");
   elm_object_part_text_set(dd->noc_layout, "elm.text", S_NO_ITEMS);

   return dd;
}

void cbhmd_drawer_deinit(Cbhmd_Drawer_Data *dd)
{
#ifdef HAVE_X11
   utilx_ungrab_key(ecore_x_display_get(), dd->x_main_win, "XF86Back");
   utilx_ungrab_key(ecore_x_display_get(), dd->x_main_win, "XF86Home");
#endif
   evas_object_del(dd->main_win);
   if (dd->anim_timer)
     ecore_timer_del(dd->anim_timer);
   if (dd->keydown_handler)
     ecore_event_handler_del(dd->keydown_handler);
   if (dd->event_rect)
     evas_object_del(dd->event_rect);
   if (dd->gesture_layer)
     {
        elm_gesture_layer_cb_set(dd->gesture_layer, ELM_GESTURE_N_FLICKS,
                                 ELM_GESTURE_STATE_END, NULL, NULL);
        evas_object_del(dd->gesture_layer);
     }
   free(dd);
}
