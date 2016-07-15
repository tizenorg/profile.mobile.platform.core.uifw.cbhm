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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON__

#include <Elementary.h>
#include <tzplatform_config.h>

#ifdef HAVE_X11
#include <Ecore_X.h>
#endif
#ifdef HAVE_WAYLAND
#include <Ecore_Wayland.h>
#endif

#include "cbhm_log.h"
#include "cbhm_error.h"

#if !defined(PACKAGE)
#define PACKAGE "cbhm"
#endif

#if !defined(APPNAME)
#define APPNAME "CBHM"
#endif

#define CBHM_MAGIC 0xad960009

#define ITEM_CNT_MAX 20
#define DATA_PATH tzplatform_getenv(TZ_USER_DATA)
#define CBHM_DATA_PATH tzplatform_mkpath(TZ_USER_DATA, "cbhm")
#define COPIED_DATA_STORAGE_DIR tzplatform_mkpath(TZ_USER_DATA, "cbhm/.cbhm_files")

enum
{
   ATOM_INDEX_TARGET = 0,
   ATOM_INDEX_TEXT = 1,
   ATOM_INDEX_HTML = 2,
   ATOM_INDEX_EFL = 3,
   ATOM_INDEX_IMAGE = 4,
   ATOM_INDEX_POLARIS = 5,
   ATOM_INDEX_MAX = 6
};

enum
{
   ATOM_INDEX_COUNT_ALL = 0,
   ATOM_INDEX_COUNT_TEXT = 1,
   ATOM_INDEX_COUNT_IMAGE = 2,
   ATOM_INDEX_COUNT_MAX = 3
};

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON__ */
