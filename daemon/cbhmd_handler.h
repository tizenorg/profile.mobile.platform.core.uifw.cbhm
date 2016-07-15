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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_HANDLER__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_HANDLER__

#ifdef HAVE_X11
#include "cbhmd_handler_x.h"
#endif
#ifdef HAVE_WAYLAND
#include "cbhmd_wl_handler.h"
#endif

#include "cbhmd.h"

typedef char* (*text_converter_func)(cbhmd_app_data_s *ad, int type_index, const char *str);

struct _cbhmd_handler_target {
#ifdef HAVE_X11
	Ecore_X_Atom *atom;
#else
	unsigned int *atom;
#endif
	char **name;
	int atom_cnt;
	text_converter_func convert_to_entry;
	text_converter_func convert_to_target[ATOM_INDEX_MAX];
};

int cbhmd_handler_init(cbhmd_app_data_s *ad);

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_HANDLER__ */
