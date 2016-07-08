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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_HANDLER_WAYLAND__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_HANDLER_WAYLAND__

#include <Ecore.h>
#include <Ecore_Wayland.h>

#include "cbhmd.h"
#include "cbhmd_appdata.h"

typedef struct _WlHandlerData {
   Ecore_Event_Handler *wl_offer_handler;
	Ecore_Event_Handler *wl_send_handler;
	Ecore_Event_Handler *wl_receive_handler;
} WlHandlerData;

WlHandlerData *init_wlhandler(AppData *data);
void depose_wlhandler(WlHandlerData *wld);

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_HANDLER_WAYLAND__ */
