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

#include "cbhmd_eldbus.h"

#include <Elementary.h>

#include "cbhm_log.h"
#include "cbhmd_item.h"
#include "cbhmd_app_data.h"
#include "cbhmd_drawer.h"

#define CBHM_DBUS_OBJPATH "/org/tizen/cbhm/dbus"
#ifndef CBHM_DBUS_INTERFACE
#define CBHM_DBUS_INTERFACE "org.tizen.cbhm.dbus"
#endif /* CBHM_DBUS_INTERFACE */

enum
{
   CBHMD_ITEM_CLICKED_SIGNAL = 0
};

static Eldbus_Message *_cbhmd_eldbus_show(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_cbhmd_eldbus_hide(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_cbhmd_eldbus_count(const Eldbus_Service_Interface *iface EINA_UNUSED, const Eldbus_Message *msg);

static const Eldbus_Method _cbhmd_eldbus_methods[] =
{
     {
        "CbhmShow",
        ELDBUS_ARGS({"s", "string"}),
        ELDBUS_ARGS({NULL, NULL}),
        _cbhmd_eldbus_show
     },
     {
        "CbhmHide",
        ELDBUS_ARGS({NULL, NULL}),
        ELDBUS_ARGS({NULL, NULL}),
        _cbhmd_eldbus_hide
     },
     {
        "CbhmGetCount",
        ELDBUS_ARGS({NULL, NULL}),
        ELDBUS_ARGS({"i", "int32"}),
        _cbhmd_eldbus_count
     },
     {}
};

static const Eldbus_Signal _cbhmd_eldbus_signals[] =
{
   [CBHMD_ITEM_CLICKED_SIGNAL] =
     {
        "ItemClicked",
        ELDBUS_ARGS({"s", "message"}),
        0
     },
     {}
};

static const
Eldbus_Service_Interface_Desc iface_desc = {
     CBHM_DBUS_INTERFACE,
     _cbhmd_eldbus_methods,
     _cbhmd_eldbus_signals
};

static Eldbus_Connection *cbhm_conn;

static void
_cbhmd_on_name_request(void *data, const Eldbus_Message *msg,
                       Eldbus_Pending *pending EINA_UNUSED)
{
   FN_CALL();
   unsigned int reply;

   if (eldbus_message_error_get(msg, NULL, NULL))
     {
        printf("error on _cbhmd_on_name_request\n");
        return;
     }

   if (!eldbus_message_arguments_get(msg, "u", &reply))
     {
        printf("error geting arguments on _cbhmd_on_name_request\n");
        return;
     }

   if (reply != ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER)
     {
        printf("error name already in use\n");
        return;
     }
}

static Eldbus_Message *
_cbhmd_eldbus_show(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   FN_CALL();
   Cbhmd_App_Data *ad;
   const char *show_type = NULL;

   ad = eldbus_service_object_data_get(iface, CBHM_DBUS_OBJPATH);

   if (!eldbus_message_arguments_get(msg, "s", &show_type))
     {
        ERR("Cannot get arguments");
        return eldbus_message_error_new(msg, "org.freedesktop.DBus.Error.Failed",
                                        "Cannot get arguments");
     }

   if (!show_type)
     {
        ERR("Cannot get show type");
        return eldbus_message_error_new(msg, "org.freedesktop.DBus.Error.Failed",
                                        "Cannot get show type");
     }

   if (!strcmp("1", show_type))
     cbhmd_drawer_text_only_mode_set(ad, EINA_FALSE);
   else cbhmd_drawer_text_only_mode_set(ad, EINA_TRUE);

   cbhmd_drawer_show(ad);

   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_cbhmd_eldbus_hide(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg)
{
   FN_CALL();
   Cbhmd_App_Data *ad;

   ad = eldbus_service_object_data_get(iface, CBHM_DBUS_OBJPATH);

   cbhmd_drawer_hide(ad);

   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_cbhmd_eldbus_count(const Eldbus_Service_Interface *iface EINA_UNUSED,
                    const Eldbus_Message *msg)
{
   FN_CALL();
   Cbhmd_App_Data *ad;
   int item_count = -1;
   Eldbus_Message *reply;

   ad = eldbus_service_object_data_get(iface, CBHM_DBUS_OBJPATH);

   /* FIXME : should support get count for TEXT and ALL. That is,
    * it needs some parameter */
   item_count = cbhmd_item_count_get(ad, ATOM_INDEX_COUNT_ALL);
   DBG("cbhm has %d items", item_count);
   if (item_count == -1)
     {
        ERR("Cannot get count");
        return eldbus_message_error_new(msg, "org.freedesktop.DBus.Error.Failed",
                                        "Cannot get count");
     }

   reply = eldbus_message_method_return_new(msg);
   if (!reply)
     {
        ERR("eldbus_message_method_return_new Failed");
        return eldbus_message_error_new(msg, "org.freedesktop.DBus.Error.Failed",
                                        "eldbus_message_method_return_new Failed");
     }

   if (!eldbus_message_arguments_append(reply, "i", item_count))
     {
        eldbus_message_unref(reply);
        ERR("eldbus_message_arguments_append Failed");
        return eldbus_message_error_new(msg, "org.freedesktop.DBus.Error.Failed",
                                        "eldbus_message_arguments_append Failed");
     }

   return reply;
}

void
cbhmd_eldbus_send_item_clicked_signal(void *data)
{
   Eldbus_Service_Interface *iface = data;
   eldbus_service_signal_emit(iface, CBHMD_ITEM_CLICKED_SIGNAL, "");
}

int
cbhmd_eldbus_init(Cbhmd_App_Data *ad)
{
   FN_CALL();
   Eldbus_Service_Interface *iface = NULL;

   if (!elm_need_eldbus())
     {
        ERR("elm_need_eldbus() Fail");
        return CBHM_ERROR_NOT_SUPPORTED;
     }

   if (!(cbhm_conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION)))
     {
        ERR("eldbus_connection_get() Fail");
        return CBHM_ERROR_CONNECTION_REFUSED;
     }

   if (!(iface = eldbus_service_interface_register(cbhm_conn,
                                                   CBHM_DBUS_OBJPATH, &iface_desc)))
     {
        ERR("eldbus_service_interface_register() Fail");
        return CBHM_ERROR_CONNECTION_REFUSED;
     }

   ad->iface = iface;

   eldbus_service_object_data_set(iface, CBHM_DBUS_OBJPATH, ad);
   eldbus_name_request(cbhm_conn, CBHM_DBUS_INTERFACE,
                       ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE, _cbhmd_on_name_request, iface);
   return CBHM_ERROR_NONE;
}

void
cbhmd_eldbus_deinit()
{
   if (cbhm_conn) eldbus_connection_unref(cbhm_conn);

   eldbus_shutdown();
}
