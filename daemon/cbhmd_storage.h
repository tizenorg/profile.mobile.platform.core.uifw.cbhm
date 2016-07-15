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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_STORAGE__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_STORAGE__

#include <Eet.h>
#include <Eina.h>
#include <Ecore.h>

#include "cbhmd.h"
#include "cbhmd_item_manager.h"

typedef double indexType; /* Ecore_Time */

struct _Cbhmd_Storage_Data
{
   Eet_File *ef;
   indexType indexTable[ITEM_CNT_MAX];
   Cbhmd_Cnp_Item *itemTable[ITEM_CNT_MAX];
};

Cbhmd_Storage_Data *cbhmd_storage_init(Cbhmd_App_Data *ad);
void depose_storage(Cbhmd_Storage_Data *sd);

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_STORAGE__ */