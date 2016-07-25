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

#include "cbhmd_utils.h"
#include "cbhmd_wl_handler.h"

#define CBHMD_WL_HANDLER_CLIPBOARD_BEGIN "CLIPBOARD_BEGIN"
#define CBHMD_WL_HANDLER_CLIPBOARD_END "CLIPBOARD_END"

static Eina_Bool
_wl_handler_selection_offer(void *data EINA_UNUSED,
                            int type EINA_UNUSED, void *event)
{
   FN_CALL();
   int i;
   Ecore_Wl_Input *input = NULL;
   Ecore_Wl_Event_Dnd_Selection *ev = event;

   RETV_IF(NULL == ev, ECORE_CALLBACK_PASS_ON);

   if (!ev->mime_types[0] || !ev->mime_types[1])
     {
        DBG("mime type is empty");
        return ECORE_CALLBACK_PASS_ON;
     }

   for (i = 0; i < 10; i++)
     {
        if (ev->mime_types[i]) DBG("mime type(%s)", ev->mime_types[i]);
     }

   /* FIXME : Current wayland doesn't support multiple clipboard types such as
    * Primary, Clipboard, Drag n drop. They just serve only Clipboard type.
    * thus, we are using work around code - check whether mime types are
    * including the string "clipboard" */
   if (SAFE_STRCMP(ev->mime_types[0], CBHMD_WL_HANDLER_CLIPBOARD_BEGIN))
     return ECORE_CALLBACK_PASS_ON;

   /* mime type is only including "clipboard" prefix and postfix */
   if (!SAFE_STRCMP(ev->mime_types[1], CBHMD_WL_HANDLER_CLIPBOARD_END))
     {
        ERR("no mime type for clipboard");
        return ECORE_CALLBACK_PASS_ON;
     }

   input = ecore_wl_input_get();
   if (!input)
     {
        ERR("ecore_wl_input_get() Fail");
        return ECORE_CALLBACK_PASS_ON;
     }

   if (!ecore_wl_dnd_selection_owner_has(input))
     {
        ERR("input doesn't have selection source");
        return ECORE_CALLBACK_PASS_ON;
     }

   DBG(
      "== [ TRANSACTION-REQ ] ====>>");
   if (!ecore_wl_dnd_selection_get(input, ev->mime_types[1]))
     {
        ERR("ecore_wl_dnd_selection_get() Fail");
        return ECORE_CALLBACK_PASS_ON;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_wl_hanalder_selection_send(void *udata, int type EINA_UNUSED,
                            void *event)
{
   FN_CALL();
   char *buf;
   int len_written = 0;
   Cbhmd_Cnp_Item *item = NULL;
   int ret, len_remained;
   Cbhmd_Drawer_Data *dd;
   Cbhmd_App_Data *ad = udata;
   Ecore_Wl_Event_Data_Source_Send *ev = event;

   RETV_IF(NULL == ev, ECORE_CALLBACK_PASS_ON);
   RETV_IF(NULL == ad, ECORE_CALLBACK_PASS_ON);
   RETV_IF(NULL == ad->drawer, ECORE_CALLBACK_PASS_ON);

   dd = ad->drawer;

   /* FIXME : Should handle various types of clipboard later.
    * For ex, primary, secondary, ... */
   if (dd->send_item_clicked)
     {
        item = ad->selected_item;
        dd->send_item_clicked = EINA_FALSE;
     }
   else
     {
        item = cbhmd_item_get_last(ad);
     }

   if (!item)
     {
        /* FIXME : Do we need to reply to wayland? */
        ERR("drawer doesn't have item");
        return ECORE_CALLBACK_PASS_ON;
     }

   len_remained = item->len;
   buf = item->data;

   if (!buf)
     {
        ERR("buffer is NULL");
        return ECORE_CALLBACK_PASS_ON;
     }

   /* is it able to tolerate delay until proper data is selected?
    * printf("wait...\n");
    * scanf("%c\n", &ret);
    * printf("continue...\n");
    * */

   while (len_written < item->len)
     {
        ret = write(ev->fd, buf, len_remained);
        if (ret == -1) break;DBG(
           "== [ TRANSACTION-SEND ] ====>> data(%s), len(%d)", buf,
           SAFE_STRLEN(buf));

        buf += ret;
        len_written += ret;
        len_remained -= ret;
     }

   close(ev->fd);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_wl_handler_selection_receive(void *udata, int type EINA_UNUSED,
                              void *event)
{
   FN_CALL();
   int i = -1;
   Cbhmd_App_Data *ad = udata;
   char *stripstr = NULL;
   const char *types[10] = {0, };
   Ecore_Wl_Input *input = NULL;
   Ecore_Wl_Event_Selection_Data_Ready *ev = event;

   RETV_IF(NULL == ev, ECORE_CALLBACK_PASS_ON);
   RETV_IF(NULL == ad, ECORE_CALLBACK_PASS_ON);

   input = ecore_wl_input_get();
   if (!input)
     {
        ERR("ecore_wl_input_get() Fail");
        return ECORE_CALLBACK_PASS_ON;
     }

   stripstr = SAFE_STRNDUP((char * )ev->data, ev->len);
   DBG("== [ TRANSACTION-RECV ] ====>> data(%s), len(%d)", stripstr, SAFE_STRLEN(stripstr));

   /* FIXME : Could handle various MIME types later */
   cbhmd_item_add_by_data(ad, ATOM_INDEX_TEXT, stripstr, SAFE_STRLEN(stripstr) + 1, EINA_TRUE);

   types[++i] = "application/x-elementary-markup";
   if (!ecore_wl_dnd_selection_set(input, types))
     {
        ERR("ecore_wl_dnd_selection_set() Fail");
        return ECORE_CALLBACK_PASS_ON;
     }

   return ECORE_CALLBACK_PASS_ON;
}

int
cbhmd_wl_handler_init(Cbhmd_App_Data *ad)
{
   FN_CALL();

   RETV_IF(NULL == ad, CBHM_ERROR_INVALID_PARAMETER);

   Cbhmd_Wl_Handler_Data *wld = CALLOC(1, sizeof(Cbhmd_Wl_Handler_Data));
   if (!wld)
     {
        ERR("CALLOC() Fail");
        return CBHM_ERROR_OUT_OF_MEMORY;
     }

   /* to catch wl_data_device selection event */
   wld->wl_offer_handler = ecore_event_handler_add(ECORE_WL_EVENT_DND_OFFER,
                                                   _wl_handler_selection_offer, ad);

   /* to catch requests for sending data */
   wld->wl_send_handler = ecore_event_handler_add(
      ECORE_WL_EVENT_DATA_SOURCE_SEND, _wl_hanalder_selection_send, ad);

   /* to receive offered data */
   wld->wl_receive_handler = ecore_event_handler_add(
      ECORE_WL_EVENT_SELECTION_DATA_READY, _wl_handler_selection_receive, ad);

   ad->wl_handler = wld;

   return CBHM_ERROR_NONE;
}

void
cbhmd_wl_handler_deinit(Cbhmd_Wl_Handler_Data *wld)
{
   ecore_event_handler_del(wld->wl_offer_handler);
   ecore_event_handler_del(wld->wl_send_handler);
   ecore_event_handler_del(wld->wl_receive_handler);
   FREE(wld);
}
