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
#include <appcore-efl.h>

#ifdef HAVE_X11
#include <Ecore_X.h>
#endif

#include <systemd/sd-daemon.h>

#ifdef X_BASED
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XIproto.h>
#endif

#define CBHM_DBUS_OBJPATH "/org/tizen/cbhm/dbus"
#ifndef CBHM_DBUS_INTERFACE
#define CBHM_DBUS_INTERFACE "org.tizen.cbhm.dbus"
#endif /* CBHM_DBUS_INTERFACE */

#include "cbhmd_utils.h"
#include "cbhmd_appdata.h"
#include "cbhmd_handler.h"
#include "cbhmd.h"

#define CLIPBOARD_MANAGER_WINDOW_TITLE_STRING "X11_CLIPBOARD_HISTORY_MANAGER"
#define ATOM_CLIPBOARD_MANAGER_NAME "CLIPBOARD_MANAGER"

static AppData *g_main_ad = NULL;
static Eldbus_Connection *cbhm_conn;

int _log_domain = -1;

#ifdef HAVE_WAYLAND
enum {
   CBHM_SIGNAL_HELLO = 0,
};

static Eldbus_Message* __cbhmd_show(const Eldbus_Service_Interface *iface,
		const Eldbus_Message *msg)
{
	FN_CALL();
	AppData *ad;
	const char *show_type = NULL;

	ad = eldbus_service_object_data_get(iface, CBHM_DBUS_OBJPATH);

	if (!eldbus_message_arguments_get(msg, "s", &show_type)) {
		ERR("Cannot get arguments");
		return eldbus_message_error_new(msg,
				"org.freedesktop.DBus.Error.Failed", "Cannot get arguments");
	}

	if (!show_type) {
		ERR("Cannot get show type");
		return eldbus_message_error_new(msg,
				"org.freedesktop.DBus.Error.Failed", "Cannot get show type");
	}

	if (!strcmp("1", show_type))
		clipdrawer_paste_textonly_set(ad, EINA_FALSE);
	else
		clipdrawer_paste_textonly_set(ad, EINA_TRUE);

	clipdrawer_activate_view(ad);

	return eldbus_message_method_return_new(msg);
}


static Eldbus_Message* __cbhmd_hide(const Eldbus_Service_Interface *iface,
		const Eldbus_Message *msg)
{
	FN_CALL();
	AppData *ad;

	ad = eldbus_service_object_data_get(iface, CBHM_DBUS_OBJPATH);

	clipdrawer_lower_view(ad);

	return eldbus_message_method_return_new(msg);
}

static Eldbus_Message* __cbhmd_count(
		const Eldbus_Service_Interface *iface EINA_UNUSED,
		const Eldbus_Message *msg)
{
	FN_CALL();
	AppData *ad;
	int item_count = -1;
	Eldbus_Message *reply;

	ad = eldbus_service_object_data_get(iface, CBHM_DBUS_OBJPATH);

   /* FIXME : should support get count for TEXT and ALL. That is,
    * it needs some parameter */
	item_count = item_count_get(ad, ATOM_INDEX_COUNT_ALL);
	DBG("cbhm has %d items", item_count);
	if (item_count == -1) {
		ERR("Cannot get count");
		return eldbus_message_error_new(msg,
				"org.freedesktop.DBus.Error.Failed", "Cannot get count");
	}

	reply = eldbus_message_method_return_new(msg);
	if (!reply) {
		ERR("eldbus_message_method_return_new Failed");
		return eldbus_message_error_new(msg,
				"org.freedesktop.DBus.Error.Failed",
				"eldbus_message_method_return_new Failed");
	}

	if (!eldbus_message_arguments_append(reply, "i", item_count)) {
		eldbus_message_unref(reply);
		ERR("eldbus_message_arguments_append Failed");
		return eldbus_message_error_new(msg,
				"org.freedesktop.DBus.Error.Failed",
				"eldbus_message_arguments_append Failed");
	}

	return reply;
}

static const Eldbus_Method methods[] = {
	{ "CbhmShow", ELDBUS_ARGS({"s", "string"}), ELDBUS_ARGS({NULL, NULL}),
		__cbhmd_show
	},
	{ "CbhmHide", ELDBUS_ARGS({NULL, NULL}), ELDBUS_ARGS({NULL, NULL}),
		__cbhmd_hide
	},
	{ "CbhmGetCount", ELDBUS_ARGS({NULL, NULL}), ELDBUS_ARGS({"i", "int32"}),
		__cbhmd_count
	},
	{ }
};

enum
{
	ITEM_CLICKED_SIGNAL = 0
};

static const Eldbus_Signal signals[] = {
	[ITEM_CLICKED_SIGNAL] = {"ItemClicked", ELDBUS_ARGS({ "s", "message" }), 0},
	{ }
};

void
send_item_clicked_signal(void *data)
{
	Eldbus_Service_Interface *iface = data;
	eldbus_service_signal_emit(iface, ITEM_CLICKED_SIGNAL, "");
}


static const Eldbus_Service_Interface_Desc iface_desc = {
		CBHM_DBUS_INTERFACE, methods, signals
};
#endif /* HAVE_WAYLAND */

static Eina_Bool setClipboardManager(AppData *ad)
{
	if (!ad) return EINA_FALSE;

	if (!ecore_file_exists(COPIED_DATA_STORAGE_DIR)) {
		if(!ecore_file_mkpath(COPIED_DATA_STORAGE_DIR))
			ERR("ecore_file_mkpath() fail");
	} else {
		ERR("Already exist !!!!");
	}

#ifdef HAVE_X11
	ad->x_disp = ecore_x_display_get();
	DBG("x_disp: 0x%p", ad->x_disp);
	if (ad->x_disp)
	{
		Ecore_X_Atom clipboard_manager_atom = XInternAtom(ad->x_disp, ATOM_CLIPBOARD_MANAGER_NAME, False);
		Ecore_X_Window clipboard_manager = XGetSelectionOwner(ad->x_disp, clipboard_manager_atom);
		DBG("clipboard_manager_window: 0x%x", clipboard_manager);
		if (!clipboard_manager)
		{
			ad->x_root_win = DefaultRootWindow(ad->x_disp);
			if (ad->x_root_win)
			{
				ad->x_event_win = ecore_x_window_new(ad->x_root_win, 0, 0, 19, 19);
				DBG("x_event_win: 0x%x", ad->x_event_win);
				if (ad->x_event_win)
				{
					XSetSelectionOwner(ad->x_disp, clipboard_manager_atom, ad->x_event_win, CurrentTime);
					Ecore_X_Window clipboard_manager = XGetSelectionOwner(ad->x_disp, clipboard_manager_atom);
					DBG("clipboard_manager: 0x%x", clipboard_manager);
					if (ad->x_event_win == clipboard_manager)
					{
						return EINA_TRUE;
					}
				}
			}
		}
	}
#endif
#ifdef HAVE_WAYLAND
	//FIXME: Implement it
	return EINA_TRUE;
#endif
	return EINA_FALSE;
}

#ifdef HAVE_X11
static void set_x_window(Ecore_X_Window x_event_win, Ecore_X_Window x_root_win)
{
	ecore_x_netwm_name_set(x_event_win, CLIPBOARD_MANAGER_WINDOW_TITLE_STRING);
	ecore_x_event_mask_set(x_event_win,
			ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
	ecore_x_event_mask_set(x_root_win,
			ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
	ecore_x_window_prop_property_set(
			x_root_win, ecore_x_atom_get("CBHM_XWIN"),
			XA_WINDOW, 32, &x_event_win, 1);
	ecore_x_flush();
}
#endif
#ifdef HAVE_WAYLAND
static void set_wl_window(Ecore_Wl_Window *wl_event_win)
{
	FN_CALL();
	/* FIXME : After decide whether use wl_event_win or not, use below func */
//	ecore_wl_window_title_set(wl_event_win,
//			CLIPBOARD_MANAGER_WINDOW_TITLE_STRING);
//	ecore_wl_flush();
}
#endif

static void __cbhmd_on_name_request(void *data, const Eldbus_Message *msg,
		Eldbus_Pending *pending EINA_UNUSED)
{
	FN_CALL();
	unsigned int reply;

	if (eldbus_message_error_get(msg, NULL, NULL)) {
		printf("error on __cbhmd_on_name_request\n");
		return;
	}

	if (!eldbus_message_arguments_get(msg, "u", &reply)) {
		printf("error geting arguments on __cbhmd_on_name_request\n");
		return;
	}

	if (reply != ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER) {
		printf("error name already in use\n");
		return;
	}
}

static int app_create(void *data)
{
	FN_CALL();
	AppData *ad = (AppData *)data;
	Eldbus_Service_Interface *iface = NULL;

	/* init connectivity */
	elm_need_eldbus();

	if (!(cbhm_conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION))) {
		ERR("eldbus_connection_get() Fail");
		return EXIT_FAILURE;
	}

	if (!(iface = eldbus_service_interface_register(cbhm_conn,
			CBHM_DBUS_OBJPATH, &iface_desc))) {
		ERR("eldbus_service_interface_register() Fail");
		return EXIT_FAILURE;
	}
	ad->iface = iface;

	eldbus_service_object_data_set(iface, CBHM_DBUS_OBJPATH, ad);

	eldbus_name_request(cbhm_conn, CBHM_DBUS_INTERFACE,
			ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE, __cbhmd_on_name_request, iface);

	/* init UI */
	elm_app_base_scale_set(2.6);
#ifdef HAVE_X11
	ecore_x_init(ad->x_disp);
#endif
#ifdef HAVE_WAYLAND
	if (!ecore_wl_init(ad->wl_disp)) {
		ERR("ecore_wl_init() Fail");
		return EXIT_FAILURE;
	}
#endif
	_log_domain = eina_log_domain_register("cbhm", EINA_COLOR_LIGHTBLUE);
	if (!_log_domain) {
		EINA_LOG_ERR("could not register cbhm log domain.");
		_log_domain = EINA_LOG_DOMAIN_GLOBAL;
	}

	if (!setClipboardManager(ad)) {
		DBG("setClipboardManager() Fail");
		return EXIT_FAILURE;
	}

#ifdef HAVE_X11
	set_x_window(ad->x_event_win, ad->x_root_win);
#endif
#ifdef HAVE_WAYLAND
	set_wl_window(ad->wl_event_win);
#endif

	if (!ecore_init())
		return EXIT_FAILURE;
	if (!ecore_evas_init())
		return EXIT_FAILURE;
	if (!edje_init())
		return EXIT_FAILURE;
	ad->magic = CBHM_MAGIC;
#ifdef HAVE_X11
	init_target_atoms(ad);
#endif
	if (!(ad->clipdrawer = init_clipdrawer(ad)))
		return EXIT_FAILURE;

	/* to be identified by E20 */
	elm_win_role_set(ad->clipdrawer->main_win, "cbhm");

#ifdef HAVE_X11
	if (!(ad->xhandler = init_xhandler(ad))) return EXIT_FAILURE;
#endif
#ifdef HAVE_WAYLAND
	if (!(ad->wlhandler = init_wlhandler(ad)))
		return EXIT_FAILURE;
#endif
	if (!(ad->storage = init_storage(ad)))
		return EXIT_FAILURE;
#ifdef HAVE_X11
	slot_item_count_set(ad);
#endif
#ifdef HAVE_X11
	set_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD, NULL);
#endif

	return 0;
}

static int app_terminate(void *data)
{
	AppData *ad = data;

	item_clear_all(ad);
	depose_clipdrawer(ad->clipdrawer);
#ifdef HAVE_X11
	depose_xhandler(ad->xhandler);
#endif
#ifdef HAVE_WAYLAND
	depose_wlhandler(ad->wlhandler);
#endif
	depose_storage(ad->storage);
#ifdef HAVE_X11
	depose_target_atoms(ad);
#endif
	FREE(ad);

	eina_log_domain_unregister(_log_domain);
	_log_domain = -1;

	if (cbhm_conn)
		eldbus_connection_unref(cbhm_conn);
	eldbus_shutdown();

	return 0;
}

static int app_pause(void *data)
{
	AppData *ad = data;
#ifdef HAVE_X11
	Ecore_X_Illume_Clipboard_State state = ecore_x_e_illume_clipboard_state_get(ad->x_active_win);
	if(state == ECORE_X_ILLUME_CLIPBOARD_STATE_ON)
#endif
	{
		clipdrawer_lower_view(ad);
	}
	return 0;
}

static int app_resume(void *data)
{
	return 0;
}

static int app_reset(bundle *b, void *data)
{
	return 0;
}

static int __lang_changed(void *event_info, void *data)
{
	return 0;
}

int main(int argc, char *argv[])
{
	AppData *ad;

	struct appcore_ops ops = {
		.create = app_create,
		.terminate = app_terminate,
		.pause = app_pause,
		.resume = app_resume,
		.reset = app_reset,
	};

	ad = calloc(1, sizeof(AppData));
	ops.data = ad;
	g_main_ad = ad;

	//appcore_set_i18n(PACKAGE, LOCALE_DIR);
	appcore_set_event_callback(APPCORE_EVENT_LANG_CHANGE, __lang_changed, NULL);

	// Notyfication to systemd
	sd_notify(1, "READY=1");

	return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}
