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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_CONVERT_H__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_CONVERT_H__

#include "cbhmd_app_data.h"

typedef struct
{
   char emoticon_name[31 + 1];
   char text[24 + 1];
} ENTRY_EMOTICON_S;

typedef enum
{
   ENTRY_EMOTICON_NONE = 0,
   ENTRY_EMOTICON_HAPPY,
   ENTRY_EMOTICON_SORRY,
   ENTRY_EMOTICON_WINK,
   ENTRY_EMOTICON_TONGUE_DANGLING,
   ENTRY_EMOTICON_SUPRISED,
   ENTRY_EMOTICON_KISS,
   ENTRY_EMOTICON_ANGRY_SHOUT,
   ENTRY_EMOTICON_SMILE,
   ENTRY_EMOTICON_OMG,
   ENTRY_EMOTICON_LITTLE_BIT_SORRY,

   ENTRY_EMOTICON_VERY_SORRY = 11,
   ENTRY_EMOTICON_GUILTY,
   ENTRY_EMOTICON_HAHA,
   ENTRY_EMOTICON_WORRIED,
   ENTRY_EMOTICON_LOVE,
   ENTRY_EMOTICON_EVIL,
   ENTRY_EMOTICON_HALF_SMILE,
   ENTRY_EMOTICON_MINIMAL_SMILE,
   ENTRY_EMOTICON_MAX
} ENTRY_EMOTICON_TYPE_E;

#ifdef HAVE_X11
int atom_type_index_get(Cbhmd_App_Data *ad, Ecore_X_Atom atom);
#else
int atom_type_index_get(Cbhmd_App_Data *ad, int atom);
#endif

#ifdef HAVE_X11
Eina_Bool generic_converter(Cbhmd_App_Data *ad, Ecore_X_Atom reqAtom, Cbhmd_Cnp_Item *item, void **data_ret, int *size_ret, Ecore_X_Atom *ttype, int *tsize);
#else
Eina_Bool generic_converter(Cbhmd_App_Data *ad, int reqAtom, Cbhmd_Cnp_Item *item, void **data_ret, int *size_ret, int *ttype, int *tsize);
#endif

char *cbhmd_convert_get_entry_string(Cbhmd_App_Data *ad, int type_index, const char *str);
char *cbhmd_convert_get_image_path_string(Cbhmd_App_Data *ad, int type_index, const char *str);

void cbhmd_convert_target_atoms_init(Cbhmd_App_Data *ad);
void cbhmd_convert_target_atoms_deinit(Cbhmd_App_Data *ad);

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_CONVERT_H__ */
