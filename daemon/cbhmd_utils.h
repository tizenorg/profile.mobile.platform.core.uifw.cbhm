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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_UTILS__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_UTILS__

#include "cbhmd.h"

void* d_malloc(const char *func, int line, size_t size);
void* d_calloc(const char *func, int line, size_t n, size_t size);
void d_free(const char *func, int line, void *m);

#define MALLOC(size) d_malloc(__func__, __LINE__, size)
#define CALLOC(n, size) d_calloc(__func__, __LINE__, n, size)
#define FREE(p) d_free(__func__, __LINE__, p)

// Define memory-safe string functions
#define SAFE_STRCMP(s1, s2) ((s1 && s2) ? strcmp(s1, s2) : (s1 ? 1 : -1))
#define SAFE_STRNCMP(s1, s2, n) ((s1 && s2) ? strncmp(s1, s2, n) : (s1 ? 1 : -1))
#define SAFE_STRDUP(s) (s ? strdup(s) : NULL)
#define SAFE_STRNDUP(s, n) (s ? strndup(s, n) : NULL)
#define SAFE_STRCPY(dest, src) (src ? strcpy(dest, src) : NULL)
#define SAFE_STRNCPY(dest, src, n) (src ? strncpy(dest, src, n) : NULL)
#define SAFE_STRCAT(dest, src) ((dest && src) ? strcat(dest, src) : NULL)
#define SAFE_STRNCAT(dest, src, n) ((dest && src) ? strncat(dest, src, n) : NULL)
#define SAFE_STRCHR(s, c) (s ? strchr(s, c) : NULL)
#define SAFE_STRSTR(haystack, needle) (haystack ? strstr(haystack, needle) : NULL)
#define SAFE_STRLEN(s) (s ? strlen(s) : 0)

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_UTILS__ */
