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

#ifndef __CLIPBOARD_HISTORY_MANAGER_LOG_H__
#define __CLIPBOARD_HISTORY_MANAGER_LOG_H__

#define CBHM_LOG_RED "\033[0;31m"
#define CBHM_LOG_GREEN "\033[0;32m"
#define CBHM_LOG_BROWN "\033[0;33m"
#define CBHM_LOG_BLUE "\033[0;34m"
#define CBHM_LOG_END "\033[0;m"

#undef _DBG
#undef _INFO
#undef _WARN
#undef _ERR

#undef DBG
#undef INFO
#undef WARN
#undef ERR

#define TIZEN_DEBUG_ENABLE
#define LOG_TAG "CBHM"
#include <dlog.h>

#ifdef CBHM_DAEMON

#define _DBG(fmt, arg...) SLOGD(CBHM_LOG_GREEN "<Daemon>" CBHM_LOG_END fmt, ##arg)
#define _INFO(fmt, arg...) SLOGI(CBHM_LOG_GREEN "<Daemon>" CBHM_LOG_END fmt, ##arg)
#define _WARN(fmt, arg...) SLOGW(CBHM_LOG_GREEN "<Daemon>" CBHM_LOG_END fmt, ##arg)
#define _ERR(fmt, arg...) SLOGE(CBHM_LOG_GREEN "<Daemon>" CBHM_LOG_END fmt, ##arg)

#else /* CBHM_DAEMON */

#define _DBG(fmt, arg...) SLOGD(fmt, ##arg)
#define _INFO(fmt, arg...) SLOGI(fmt, ##arg)
#define _WARN(fmt, arg...) SLOGW(fmt, ##arg)
#define _ERR(fmt, arg...) SLOGE(fmt, ##arg)

#endif /* CBHM_DAEMON */

#define CBHM_DEBUGGING

#ifdef CBHM_DEBUGGING

#define FN_CALL() _INFO(">>>>>>>> called")
#define FN_END() _INFO("<<<<<<<< ended")
#define DBG(fmt, arg...) _DBG(fmt, ##arg)
#define WARN(fmt, arg...) _WARN(fmt  ##arg)
#define ERR(fmt, arg...) _ERR(fmt, ##arg)
#define INFO(fmt, arg...) _INFO(fmt, ##arg)
#define SECURE_DBG(fmt, arg...) SECURE_SLOGD(fmt, ##arg)
#define SECURE_ERR(fmt, arg...) SECURE_SLOGE(fmt, ##arg)

#else /* CBHM_DEBUGGING */

#define FN_CALL()
#define FN_END()
#define DBG(fmt, arg...)
#define WARN(fmt, arg...)
#define ERR(fmt, arg...) _ERR(fmt, ##arg)
#define INFO(fmt, arg...)
#define SECURE_DBG(fmt, arg...)
#define SECURE_ERR(fmt, arg...) SECURE_SLOGE(fmt, ##arg)

#endif /* CBHM_DEBUGGING */

#endif /* __CLIPBOARD_HISTORY_MANAGER_LOG_H__ */
