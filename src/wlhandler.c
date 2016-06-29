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

#include "wlhandler.h"

static Eina_Bool _wl_selection_send(void *udata, int type EINA_UNUSED,
		void *event)
{
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _wl_selection_receive(void *udata, int type EINA_UNUSED,
		void *event)
{
   return ECORE_CALLBACK_PASS_ON;
}

WlHandlerData *init_wlhandler(AppData *ad)
{
	WlHandlerData *wld = CALLOC(1, sizeof(WlHandlerData));
	if (!wld)
		return NULL;

	wld->wl_send_handler = ecore_event_handler_add(
			ECORE_WL_EVENT_DATA_SOURCE_SEND, _wl_selection_send, NULL);
	wld->wl_receive_handler = ecore_event_handler_add(
			ECORE_WL_EVENT_SELECTION_DATA_READY, _wl_selection_receive, NULL);

   return wld;
}
