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

#include "cbhmd_handler_wl.h"

#include "cbhmd_utils.h"

static Eina_Bool _wl_selection_offer(void *data EINA_UNUSED,
		int type EINA_UNUSED, void *event EINA_UNUSED)
{
	FN_CALL();
	Ecore_Wl_Input *input = NULL;

	input = ecore_wl_input_get();
	if (!input)
		return ECORE_CALLBACK_PASS_ON;

	if (!ecore_wl_dnd_selection_owner_has(input))
		return ECORE_CALLBACK_PASS_ON;

	ecore_wl_dnd_selection_get(input, "application/x-elementary-markup");

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _wl_selection_send(void *udata, int type EINA_UNUSED,
		void *event)
{
	FN_CALL();
	Ecore_Wl_Event_Data_Source_Send *ev = event;
	AppData *ad = udata;
	char *buf;
	int len_written = 0;
	CNP_ITEM *item = NULL;
	int ret, len_remained;

	if (!udata || !event)
		return ECORE_CALLBACK_PASS_ON;

	/* FIXME : Should handle various types of clipboard laster.
	 * For ex, primary, secondary, ... */
	item = item_get_last(ad);

	if (!item) {
		/* FIXME : Do we need to reply to wayland? */
		DBG("has no item");
		return ECORE_CALLBACK_DONE;
	}

	len_remained = item->len;
	buf = item->data;

	/* is it able to tolerate delay until proper data is selected?
	 * printf("wait...\n");
	 * scanf("%c\n", &ret);
	 * printf("continue...\n");
	 * */

	while (len_written < item->len) {
		ret = write(ev->fd, buf, len_remained);
		if (ret == -1)
			break;
		buf += ret;
		len_written += ret;
		len_remained -= ret;
	}

	close(ev->fd);
	return ECORE_CALLBACK_PASS_ON;
}


static Eina_Bool _wl_selection_receive(void *udata, int type EINA_UNUSED,
		void *event)
{
	FN_CALL();
	Ecore_Wl_Event_Selection_Data_Ready *ev = event;
	AppData *ad = udata;
	Ecore_Wl_Input *input = NULL;
	char *stripstr = NULL;

	input = ecore_wl_input_get();
	if (!input)
		return ECORE_CALLBACK_PASS_ON;

	stripstr = SAFE_STRNDUP((char *)ev->data, ev->len);
	DBG("get data: %s, len: %d", stripstr, SAFE_STRLEN(stripstr));

	/* FIXME : Could handle various MIME types later */
	item_add_by_data(ad, 0, stripstr, SAFE_STRLEN(stripstr) + 1, EINA_TRUE);

	const char *types[10] = {0, };
	int i = -1;
	types[++i] = "application/x-elementary-markup";
	ecore_wl_dnd_selection_set(input, types);
	return ECORE_CALLBACK_PASS_ON;
}

WlHandlerData *init_wlhandler(AppData *ad)
{
	FN_CALL();
	WlHandlerData *wld = CALLOC(1, sizeof(WlHandlerData));
	if (!wld)
		return NULL;

	/* to catch wl_data_device selection event */
	wld->wl_receive_handler = ecore_event_handler_add(ECORE_WL_EVENT_DND_OFFER,
			_wl_selection_offer, ad);

	/* to catch requests for sending data */
	wld->wl_send_handler = ecore_event_handler_add(
			ECORE_WL_EVENT_DATA_SOURCE_SEND, _wl_selection_send, ad);

	/* to receive offered data */
	wld->wl_receive_handler = ecore_event_handler_add(
			ECORE_WL_EVENT_SELECTION_DATA_READY, _wl_selection_receive, ad);

	return wld;
}
