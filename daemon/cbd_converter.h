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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_CONVERTER_H__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_CONVERTER_H__

#ifdef HAVE_X11
#include "cbd_converter_x.h"
#endif
#ifdef HAVE_WAYLAND
#include "cbd_converter_wl.h"
#endif

char* string_for_entry_get(AppData *ad, int type_index, const char *str);
char* string_for_image_path_get(AppData *ad, int type_index, const char *str);

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_CONVERTER_H__ */
