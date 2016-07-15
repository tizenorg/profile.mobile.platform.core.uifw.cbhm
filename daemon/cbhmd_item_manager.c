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

#include "cbhmd_item_manager.h"

#include <time.h>
#include <notification.h>

#include "cbhmd_appdata.h"
#include "cbhmd_convert.h"
#include "cbhmd_drawer.h"
#include "cbhmd_handler.h"
#include "cbhmd_utils.h"

static void show_notification(Cbhmd_Cnp_Item *item, const char *msg)
{
   RET_IF(NULL == item);
   RET_IF(NULL == item->data);

   if (item->len > 0)
     notification_status_message_post(msg);
}

static void item_free(Cbhmd_Cnp_Item *item, Eina_Bool storage)
{
   FN_CALL();

   RET_IF(NULL == item);

   // remove gengrid
   if (item->ad)
     {
        if (item->ad->draw_item_del)
          item->ad->draw_item_del(item->ad, item);
        if (storage && item->ad->storage_item_del)
          item->ad->storage_item_del(item->ad, item);
     }

   if (item->data)
     FREE(item->data);

   // When deleting combined style item, image file at the original location
   // (i.e file at the image path given while creating combined item) should not get deleted.
   if (item->file && item->gitem_style != GRID_ITEM_STYLE_COMBINED)
     {
        ecore_file_remove(item->file);
        FREE(item->file);
     }

   if (item->ad)
     {
        if (item->ad->selected_item == item)
          item->ad->selected_item = NULL;
     }
   FREE(item);
}

Cbhmd_Cnp_Item *item_add_by_Cbhmd_Cnp_Item(Cbhmd_App_Data *ad,
                                             Cbhmd_Cnp_Item *item, Eina_Bool storage, Eina_Bool show_msg)
{
   if (!ad || !item)
     {
        ERR("WRONG PARAMETER in %s", __func__);
        return NULL;
     }
   Cbhmd_Drawer_Data *dd = ad->drawer;
   if ((item_count_get(ad, ATOM_INDEX_COUNT_ALL) - dd->locked_item_count) == 0)
     {
        elm_object_signal_emit(dd->main_layout, "elm,state,enable,del", "elm");
        elm_object_part_content_unset(dd->main_layout, "historyitems");
        evas_object_hide(dd->noc_layout);
        elm_object_part_content_set(dd->main_layout, "historyitems", dd->gengrid);
     }

   if (!item)
     {
        ERR("WRONG PARAMETER in %s, ad: 0x%p, item: 0x%p", __func__, ad, item);
        return NULL;
     }
   item->ad = ad;

   Eina_Bool duplicated = EINA_FALSE;

   ad->item_list = eina_list_prepend(ad->item_list, item);
   if (ad && ad->draw_item_add)
     duplicated = ad->draw_item_add(ad, item);
   if (storage && ad && ad->storage_item_add)
     ad->storage_item_add(ad, item);

   while (ITEM_CNT_MAX < eina_list_count(ad->item_list))
     {
        Cbhmd_Cnp_Item *ditem = eina_list_nth(ad->item_list, ITEM_CNT_MAX);

        ad->item_list = eina_list_remove(ad->item_list, ditem);
        item_free(ditem, EINA_TRUE);
     }

#ifdef HAVE_X11
   slot_property_set(ad, -1);
   cbhmd_x_handler_slot_item_count_set(ad);
#endif
   if (show_msg)
     {
        if (duplicated)
          show_notification(item, S_EXIST);
        else show_notification(item, S_COPY);
     }

   return item;
}

static void downloaded_cb(void *data, const char *file_, int status)
{
   Cbhmd_Cnp_Item *item = data;

   if (status == 200)
     {
        item->img_from_web = EINA_TRUE;
        DBG("image download success\n");
     }
   else
     {
        item->img_from_web = EINA_FALSE;
        DBG("image download fail\n");
     }
}

static Eina_Bool get_network_state()
{
   int net_conf;
   int net_status;
   vconf_get_int(VCONFKEY_NETWORK_CONFIGURATION_CHANGE_IND, &net_conf);
   vconf_get_int(VCONFKEY_NETWORK_STATUS, &net_status);
   DBG("current network configuration (%d), Network status (%d)\n", net_conf, net_status);

   if (net_conf == 0 && net_status == VCONFKEY_NETWORK_OFF)
     {
        DBG("No wifi and No 3G\n");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static void image_name_get(char *filename, int size)
{
   time_t tim;
   struct tm now = {0};

   //file path get
   tim = time(NULL);
   localtime_r(&tim, &now);

   snprintf(filename, size, "fromweb-%d%02d%02d%02d%02d%02d%s",
            now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour,
            now.tm_min, now.tm_sec, ".jpg");
}

char* html_img_save_frm_local(char *copied_path)
{
   char *html_img_url = NULL;
   char html_img_name[PATH_MAX];
   int size_path = 0;

   size_path = snprintf(NULL, 0, "%s", copied_path) + 1;
   html_img_url = MALLOC(sizeof(char) * size_path);
   snprintf(html_img_url, size_path, "%s", copied_path);
   DBG("html_img_url = %s\n", html_img_url);

   image_name_get(html_img_name, sizeof(html_img_name));
   DBG("copied file name = %s\n", html_img_name);

   size_path = snprintf(NULL, 0, "%s/%s", COPIED_DATA_STORAGE_DIR,
                        html_img_name) + 1;
   copied_path = MALLOC(sizeof(char) * size_path);
   snprintf(copied_path, size_path, "%s/%s", COPIED_DATA_STORAGE_DIR,
            html_img_name);
   DBG("copied path = %s\n", copied_path);
   ecore_file_cp(html_img_url, copied_path);
   FREE(html_img_url);
   return copied_path;
}

char* html_img_save(char *copied_path, Cbhmd_Cnp_Item *item)
{
   char *html_img_url = NULL;
   char html_img_name[PATH_MAX];
   int size_path = 0;
   int ret;
   Eina_Bool is_network_enable;

   is_network_enable = get_network_state();
   DBG("network enable = %d\n", is_network_enable);
   if (is_network_enable)
     {
        size_path = snprintf(NULL, 0, "http:/" "%s", copied_path) + 1;
        html_img_url = MALLOC(sizeof(char) * size_path);
        snprintf(html_img_url, size_path, "http:/" "%s", copied_path);
        DBG("html_img_url = %s\n", html_img_url);

        image_name_get(html_img_name, sizeof(html_img_name));
        DBG("copied file name = %s\n", html_img_name);

        size_path = snprintf(NULL, 0, "%s/%s", COPIED_DATA_STORAGE_DIR,
                             html_img_name) + 1;
        copied_path = MALLOC(sizeof(char) * size_path);
        snprintf(copied_path, size_path, "%s/%s", COPIED_DATA_STORAGE_DIR,
                 html_img_name);
        DBG("copied path = %s\n", copied_path);
        ret = ecore_file_download_full(html_img_url, copied_path, downloaded_cb,
                                       NULL, item, NULL, NULL);
        if (ret)
          DBG("Download start");

        FREE(html_img_url);
        return copied_path;
     }

   return NULL;
}

#ifdef HAVE_X11
Cbhmd_Cnp_Item *item_add_by_data(Cbhmd_App_Data *ad, Ecore_X_Atom type, void *data, int len,
                                   Eina_Bool show_msg)
#else
Cbhmd_Cnp_Item *item_add_by_data(Cbhmd_App_Data *ad, int type, void *data,
                                   int len, Eina_Bool show_msg)
#endif
{
   char *entry_text = NULL;
   char *orig_path = NULL;
   char *copied_path = NULL;
   const char *file_name = NULL;
   int size_path = 0;

   if (!ad || !data)
     {
        ERR("WRONG PARAMETER in %s", __func__);
        return NULL;
     }
   Cbhmd_Cnp_Item *item;
   item = CALLOC(1, sizeof(Cbhmd_Cnp_Item));

   if (!item)
     return NULL;

#ifdef HAVE_X11
   item->type_index = atom_type_index_get(ad, type);
#else
   /* FIXME : Could handle various MIME types later */
   item->type_index = ATOM_INDEX_TEXT;
#endif

   if (item->type_index == ATOM_INDEX_TEXT)
     item->gitem_style = GRID_ITEM_STYLE_TEXT;
   else
     if (item->type_index == ATOM_INDEX_HTML)
       {
          copied_path = cbhmd_convert_get_image_path_string(ad, ATOM_INDEX_HTML,
                                                            data);
          DBG("found img path in html tags = %s\n", copied_path);

          if (copied_path && ad->drawer->http_path)
            copied_path = html_img_save(copied_path, item);
          else
            if (copied_path)
              copied_path = html_img_save_frm_local(copied_path);

          entry_text = cbhmd_convert_get_entry_string(ad, ATOM_INDEX_HTML, data);
          DBG("entry_text = %s\n copied_path = %s\n", entry_text, copied_path);
       }
     else
       if (item->type_index == ATOM_INDEX_EFL)
         {
            entry_text = cbhmd_convert_get_entry_string(ad, ATOM_INDEX_EFL,
                                                        data);
            copied_path = cbhmd_convert_get_image_path_string(ad,
                                                              ATOM_INDEX_EFL, data);
            if (copied_path)
              item->img_from_markup = EINA_TRUE;
         }
       else
         if (item->type_index == ATOM_INDEX_IMAGE)
           {
              orig_path = data;
              file_name = ecore_file_file_get(orig_path);
              size_path = snprintf(NULL, 0, "%s/%s", COPIED_DATA_STORAGE_DIR,
                                   file_name) + 1;
              copied_path = MALLOC(sizeof(char) * size_path);
              if (copied_path)
                {
                   snprintf(copied_path, size_path, "%s/%s",
                            COPIED_DATA_STORAGE_DIR, file_name);
                   data = copied_path;
                   len = SAFE_STRLEN(copied_path) + 1;
                }
              // Reallocate memory for item->file
              copied_path = MALLOC(sizeof(char) * len);
              if (copied_path)
                snprintf(copied_path, len, "%s", (char *)data);
           }

   if (copied_path)
     {
        item->file = copied_path;
        item->file_len = SAFE_STRLEN(copied_path) + 1;
     }

   if (entry_text && copied_path)
     {
        item->gitem_style = GRID_ITEM_STYLE_COMBINED;
        FREE(entry_text);
     }
   else
     if (entry_text)
       {
          item->gitem_style = GRID_ITEM_STYLE_TEXT;
          FREE(entry_text);
       }
     else
       if (copied_path)
         item->gitem_style = GRID_ITEM_STYLE_IMAGE;

   item->data = data;
   item->len = len;

   item = item_add_by_Cbhmd_Cnp_Item(ad, item, EINA_TRUE, show_msg);

   if ((item->type_index == ATOM_INDEX_IMAGE) && orig_path && copied_path)
     {
        if (!ecore_file_cp(orig_path, copied_path))
          DBG("ecore_file_cp fail!");
        FREE(orig_path);
     }

   return item;
}

Cbhmd_Cnp_Item *item_get_by_index(Cbhmd_App_Data *ad, int index)
{
   if (!ad || eina_list_count(ad->item_list) <= index || 0 > index)
     {
        ERR("WRONG PARAMETER in %s", __func__);
        return NULL;
     }
   Cbhmd_Cnp_Item *item;
   item = eina_list_nth(ad->item_list, index);
   return item;
}

Cbhmd_Cnp_Item *item_get_by_data(Cbhmd_App_Data *ad, void *data, int len)
{
   Cbhmd_Cnp_Item *item;
   Eina_List *l;

   if (!ad || !data)
     {
        ERR("WRONG PARAMETER in %s", __func__);
        return NULL;
     }

   EINA_LIST_FOREACH(ad->item_list, l, item)
     {
        if (item && (item->len == len) && (!SAFE_STRNCMP(item->data, data, len)))
          return item;
     }

   return NULL;
}

Cbhmd_Cnp_Item *item_get_last(Cbhmd_App_Data *ad)
{
   if (!ad)
     {
        ERR("WRONG PARAMETER in %s", __func__);
        return NULL;
     }
   return eina_list_data_get(ad->item_list);
}

void item_delete_by_Cbhmd_Cnp_Item(Cbhmd_App_Data *ad, Cbhmd_Cnp_Item *item)
{
   FN_CALL();

   if (!item)
     {
        ERR("WRONG PARAMETER in %s", __func__);
        return;
     }
   ad->item_list = eina_list_remove(ad->item_list, item);
   item_free(item, EINA_TRUE);
#ifdef HAVE_X11
   slot_property_set(ad, -1);
   cbhmd_x_handler_slot_item_count_set(ad);
#endif
   if (item_count_get(ad, ATOM_INDEX_COUNT_ALL) == 0)
     {
        cbhmd_drawer_delete_mode_set(ad, EINA_FALSE);
        cbhmd_drawer_hide(ad);
     }
}

void item_delete_by_data(Cbhmd_App_Data *ad, void *data, int len)
{
   FN_CALL();
   if (!ad || !data)
     {
        ERR("WRONG PARAMETER in %s", __func__);
        return;
     }
   Cbhmd_Cnp_Item *item;
   item = item_get_by_data(ad, data, len);
   item_delete_by_Cbhmd_Cnp_Item(ad, item);
}

void item_delete_by_index(Cbhmd_App_Data *ad, int index)
{
   FN_CALL();
   if (!ad || eina_list_count(ad->item_list) <= index || 0 > index)
     {
        ERR("WRONG PARAMETER in %s", __func__);
        return;
     }
   Cbhmd_Cnp_Item *item;
   item = item_get_by_index(ad, index);
   item_delete_by_Cbhmd_Cnp_Item(ad, item);
}

void item_clear_all(Cbhmd_App_Data *ad)
{
   FN_CALL();
   while (ad->item_list)
     {
        Cbhmd_Cnp_Item *item = eina_list_data_get(ad->item_list);
        ad->item_list = eina_list_remove(ad->item_list, item);
        if (item)
          item_free(item, EINA_FALSE);
     }
}

int item_count_get(Cbhmd_App_Data *ad, int atom_index)
{
   int icount = 0;
   Eina_List *l;
   Cbhmd_Cnp_Item *item;

   if (!ad || !ad->item_list)
     return 0;

   // Get the number of all items
   if (atom_index == ATOM_INDEX_COUNT_ALL)
     {
        icount = eina_list_count(ad->item_list);
     }
   // Get the number of text type items
   else
     if (atom_index == ATOM_INDEX_COUNT_TEXT)
       {
          EINA_LIST_FOREACH(ad->item_list, l, item)
            {
               if (item
                   && (item->type_index == ATOM_INDEX_TEXT
                       || item->type_index == ATOM_INDEX_HTML
                       || item->type_index == ATOM_INDEX_EFL))
                 icount++;
            }
       }
   // Get the number of image type items
     else
       if (atom_index == ATOM_INDEX_COUNT_IMAGE)
         {
            EINA_LIST_FOREACH(ad->item_list, l, item)
              {
                 if (item && (item->type_index == ATOM_INDEX_IMAGE))
                   icount++;
              }
         }

   return icount;
}
