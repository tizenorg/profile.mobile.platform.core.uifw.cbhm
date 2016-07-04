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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_HANDLER_X__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_HANDLER_X__

#ifdef HAVE_X11
#include <Ecore_X.h>
#endif
#include <Ecore.h>
#ifdef MDM_ENABLE
#include <mdm.h>
#endif

struct _XHandlerData {
	Ecore_Event_Handler *xsel_clear_handler;
	Ecore_Event_Handler *xsel_request_handler;
	Ecore_Event_Handler *xsel_notify_handler;
	Ecore_Event_Handler *xclient_msg_handler;
	Ecore_Event_Handler *xfocus_out_handler;
	Ecore_Event_Handler *xproperty_notify_handler;
	Ecore_Event_Handler *xwindow_destroy_handler;

#ifdef HAVE_X11
	Ecore_X_Atom atomInc;

	Ecore_X_Atom atomCBHM_MSG;
	Ecore_X_Atom atomCBHM_ITEM;

	Ecore_X_Atom atomUTF8String;
	Ecore_X_Atom atomCBHMCount[ATOM_INDEX_COUNT_MAX];
	Ecore_X_Atom atomCBHM_SELECTED_ITEM;

	Ecore_X_Atom atomShotString;
#endif

	Ecore_Timer *selection_timer;
};

#include "cbhmd.h"

XHandlerData *init_xhandler(AppData *data);
void depose_xhandler(XHandlerData *xd);

#ifdef HAVE_X11
Eina_Bool is_cbhm_selection_owner(AppData *ad, Ecore_X_Selection selection);
Eina_Bool set_selection_owner(AppData *ad, Ecore_X_Selection selection, CNP_ITEM *item);
#endif
void slot_property_set(AppData *ad, int index);
void slot_item_count_set(AppData *ad);
void slot_selected_item_set(AppData *ad);
#ifdef HAVE_X11
Eina_Bool cbhm_send_event(AppData *ad, Ecore_X_Window xwin, char *msg);
#endif
Eina_Bool _mdm_get_allow_clipboard();	// magnolia only

#define SELECTION_CHECK_TIME 10.0

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_HANDLER_X__ */
