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

#ifndef __CLIPBOARD_HISTORY_MANAGER_ERROR_H__
#define __CLIPBOARD_HISTORY_MANAGER_ERROR_H__

#include <tizen.h>

typedef enum
{
   CBHM_ERROR_NONE = TIZEN_ERROR_NONE, /**< Successful */
   //   CBHM_ERROR_IO_ERROR = TIZEN_ERROR_IO_ERROR, /**< I/O error */
   CBHM_ERROR_OUT_OF_MEMORY = TIZEN_ERROR_OUT_OF_MEMORY, /**< Out of memory */
   //   CBHM_ERROR_PERMISSION_DENIED = TIZEN_ERROR_PERMISSION_DENIED, /**< Permission denied */
   CBHM_ERROR_NOT_SUPPORTED = TIZEN_ERROR_NOT_SUPPORTED, /**< Not supported */
   CBHM_ERROR_INVALID_PARAMETER = TIZEN_ERROR_INVALID_PARAMETER, /**< Invalid parameter */
   CBHM_ERROR_NO_DATA = TIZEN_ERROR_NO_DATA, /**< No data available */
   //   CBHM_ERROR_TIMEOUT = TIZEN_ERROR_CONNECTION_TIME_OUT, /**< Time out */
   CBHM_ERROR_CONNECTION_REFUSED = TIZEN_ERROR_CONNECTION_REFUSED, /**< Connection refused */
   CBHM_ERROR_ALREADY_IN_USE = TIZEN_ERROR_ADDRES_IN_USE, /**< Address already in use */
} Cbhm_Error;

#endif /* __CLIPBOARD_HISTORY_MANAGER_ERROR_H__ */
