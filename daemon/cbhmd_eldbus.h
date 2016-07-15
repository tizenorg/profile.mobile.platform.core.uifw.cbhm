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

#ifndef __CLIPBOARD_HISTORY_MANAGER_DAEMON_ELDBUS__
#define __CLIPBOARD_HISTORY_MANAGER_DAEMON_ELDBUS__

void cbhmd_eldbus_send_item_clicked_signal(void *data);

int cbhmd_eldbus_init();
void cbhmd_eldbus_deinit();

#endif /* __CLIPBOARD_HISTORY_MANAGER_DAEMON_ELDBUS__ */
