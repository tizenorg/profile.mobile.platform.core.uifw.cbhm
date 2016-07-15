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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_CONVERTER_X_H__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_CONVERTER_X_H__

#include "cbhmd.h"

#ifdef HAVE_X11
int atom_type_index_get(cbhmd_app_data_s *ad, Ecore_X_Atom atom);
#else
int atom_type_index_get(cbhmd_app_data_s *ad, int atom);
#endif

char* cbhmd_convert_image_path_string_get(cbhmd_app_data_s *ad, int type_index,
		const char *str);
#ifdef HAVE_X11
Eina_Bool generic_converter(cbhmd_app_data_s *ad, Ecore_X_Atom reqAtom, cbhmd_cnp_item_s *item, void **data_ret, int *size_ret, Ecore_X_Atom *ttype, int *tsize);
#else
Eina_Bool generic_converter(cbhmd_app_data_s *ad, int reqAtom, cbhmd_cnp_item_s *item, void **data_ret, int *size_ret, int *ttype, int *tsize);
#endif

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_CONVERTER_X_H__ */
