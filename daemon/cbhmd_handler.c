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

#include "cbhm_log.h"
#include "cbhmd_appdata.h"
#include "cbhmd_handler.h"

int cbhmd_handler_init(Cbhmd_App_Data *ad)
{
   int ret;
#ifdef HAVE_X11
   ret = cbhmd_x_handler_init(ad);
   if (CBHM_ERROR_NONE != ret)
     {
        ERR("cbhmd_x_handler_init() Fail(%d", ret);
        return ret;
     }
#endif
#ifdef HAVE_WAYLAND
   ret = cbhmd_wl_handler_init(ad);
   if (CBHM_ERROR_NONE != ret)
     {
        ERR("cbhmd_wl_handler_init() Fail(%d", ret);
        return ret;
     }
#endif

   return CBHM_ERROR_NONE;
}
