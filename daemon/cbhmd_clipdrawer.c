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

//#include <utilX.h>
#include "cbhmd_clipdrawer.h"

#include "cbhmd_appdata.h"
#include "cbhmd_converter_x.h"
#include "cbhmd_handler.h"
#include "cbhmd_item_manager.h"
#include "cbhmd_utils.h"

#define EDJ_PATH RES_DIR"/edje"
#define APP_EDJ_FILE EDJ_PATH"/cbhmdrawer.edj"
#define GRP_MAIN "cbhmdrawer"

#define ANIM_DURATION 30 // 1 seconds
#define ANIM_FLOPS (0.5/30)
#define DEFAULT_WIDTH 720
#define COMBINED_ITEM_IMAGE_WIDTH 102
#define COMBINED_ITEM_IMAGE_HEIGHT 153
#define BUFF (int)1024 //limited gengrid text buffer

#define EDJE_CLOSE_PART_PREFIX "background/title/close/bg"
#define EDJE_DELETE_MODE_PREFIX "background/title/delete/image"
#define EDJE_DELETE_ALL_BTN_PART_PREFIX "background/title/delete_all/image"

#define TIME_DELAY_LOWER_VIEW 0.1 //Time to delay lower view

static Evas_Object *create_win(ClipdrawerData *cd, const char *name);
static Evas_Object *_grid_image_content_get(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_grid_combined_content_get(void *data, Evas_Object *obj, const char *part);
static char *_grid_text_get(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_grid_content_get(void *data, Evas_Object *obj, const char *part);
static void _grid_realized(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool clipdrawer_add_item(AppData *ad, CNP_ITEM *item);
static Eina_Bool clipdrawer_del_item(AppData *ad, CNP_ITEM *item);
static void _ok_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void _cancel_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void _create_cbhm_popup(AppData *ad);
static void clipdrawer_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _grid_item_button_clicked(void *data, Evas_Object *obj, void *event_info);
#ifdef HAVE_X11
static void setting_win(Ecore_X_Display *x_disp, Ecore_X_Window x_root_win, Ecore_X_Window x_main_win);
static Ecore_X_Window isf_ise_window_get();
static void set_transient_for(Ecore_X_Window x_main_win, Ecore_X_Window x_active_win);
static void unset_transient_for(Ecore_X_Window x_main_win);
static void set_focus_for_app_window(Ecore_X_Window x_main_win, Eina_Bool enable);
#endif
#ifdef HAVE_WAYLAND
//static void setting_win(void *x_disp, unsigned int x_root_win, unsigned int x_main_win);
static Ecore_Wl_Window* wl_isf_ise_window_get();
static void set_transient_for(Ecore_Wl_Window *wl_main_win, Ecore_Wl_Window *wl_active_win);
static void unset_transient_for(Ecore_Wl_Window *wl_main_win);
static void set_focus_for_app_window(Ecore_Wl_Window *wl_main_win, Eina_Bool enable);
#endif
#ifdef HAVE_WAYLAND
void setting_win(const char *wl_disp, Ecore_Wl_Window *wl_main_win);
#endif

static Evas_Event_Flags flick_end(void *data , void *event_info)
{
	AppData *ad = data;
	Elm_Gesture_Line_Info *event = event_info;
	int x_diff, y_diff;

	x_diff = event->momentum.x2 - event->momentum.x1;
	y_diff = event->momentum.y2 - event->momentum.y1;

	if (y_diff > 0 && y_diff > abs(x_diff))
		clipdrawer_lower_view(ad);

	return EVAS_EVENT_FLAG_NONE;
}

static void _change_gengrid_paste_textonly_mode(ClipdrawerData *cd)
{
	CNP_ITEM *item = NULL;

	Elm_Object_Item *gitem = elm_gengrid_first_item_get(cd->gengrid);
	char *entry_text = NULL;

	while (gitem)
	{
		item = elm_object_item_data_get(gitem);
		if(!item)
			return;
		if (item->type_index == ATOM_INDEX_IMAGE)
		{
			if (cd->paste_text_only)
				elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
			else
				elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
		}

		if (item->gitem_style == GRID_ITEM_STYLE_COMBINED)
		{
			entry_text = string_for_entry_get(item->ad, item->type_index, item->data);
			entry_text = evas_textblock_text_markup_to_utf8(NULL, entry_text);

			if (cd->paste_text_only && entry_text && !strlen(entry_text))
				elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
			else
				elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
			if (entry_text) free(entry_text);
		}

		gitem = elm_gengrid_item_next_get(gitem);
	}
}

void clipdrawer_paste_textonly_set(AppData *ad, Eina_Bool textonly)
{
	FN_CALL();
	ClipdrawerData *cd = ad->clipdrawer;
	if (cd->paste_text_only != textonly)
		cd->paste_text_only = textonly;
	DBG("paste textonly mode = %d", textonly);

	_change_gengrid_paste_textonly_mode(cd);
}

Eina_Bool clipdrawer_paste_textonly_get(AppData *ad)
{
	ClipdrawerData *cd = ad->clipdrawer;
	return cd->paste_text_only;
}

static Evas_Object *_load_edj(Evas_Object* win, const char *file,
		const char *group)
{
	Evas_Object *layout = elm_layout_add(win);
	if (!layout) {
		ERR("elm_layout_add() Fail");
		return NULL;
	}

	if (!elm_layout_file_set(layout, file, group)) {
		ERR("elm_layout_file_set() Fail(%s)", file);
		evas_object_del(layout);
		return NULL;
	}

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, layout);

	evas_object_show(layout);
	return layout;
}

static void _gengrid_select_cb(void *data, Evas_Object *obj, void *event)
{
	CNP_ITEM *item = data;
	AppData *ad = item->ad;
	ClipdrawerData *cd = ad->clipdrawer;

	elm_gengrid_item_selected_set(event, EINA_FALSE);
	if (cd->delbtn_clicked)
	{
		item_delete_by_CNP_ITEM(ad, item);
		cd->delbtn_clicked = EINA_FALSE;
	}
	else
	{
		if (delete_mode)
			return;

		if (cd->popup_activate)
			return;

		if (item->type_index != ATOM_INDEX_IMAGE || !cd->paste_text_only)
		{
			ad->clip_selected_item = item;
#ifdef HAVE_X11
			if (is_cbhm_selection_owner(ad, ECORE_X_SELECTION_SECONDARY))
				cd->item_clicked = EINA_TRUE;
			else
				set_selection_owner(ad, ECORE_X_SELECTION_SECONDARY, NULL);
#endif
#ifdef HAVE_WAYLAND
			send_item_clicked_signal(ad->iface);
			ad->send_item_clicked = EINA_TRUE;
#endif
		}
	}
}

static Eina_Bool keydown_cb(void *data, int type, void *event)
{
	if (!data || !event) return ECORE_CALLBACK_DONE;
	AppData *ad = data;
	ClipdrawerData *cd = ad->clipdrawer;
	Ecore_Event_Key *ev = event;

	if (!SAFE_STRCMP(ev->keyname, "XF86Back"))
	{
		if (cd->popup_activate)
		{
			cd->popup_activate = EINA_FALSE;
			evas_object_del(cd->popup_conform);
			evas_object_del(cd->popup_win);
		}
		else if (delete_mode)
			_delete_mode_set(ad, EINA_FALSE);
		else
			clipdrawer_lower_view(ad);
	}
	else if(!SAFE_STRCMP(ev->keyname, "XF86Home"))
	{
		clipdrawer_lower_view(ad);
		return ECORE_CALLBACK_PASS_ON;
	}

	return ECORE_CALLBACK_DONE;
}

// Access type callback for title close button
static char *
_title_close_btn_access_type_cb(void *data,
										  Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	return strdup("Button");
}

// Access information callback for title close button
static char *
_title_close_btn_access_info_cb(void *data,
										  Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	return strdup(S_CLOSE);
}

// Access type callback for title delete button
static char *
_title_delete_btn_access_type_cb(void *data,
											Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	return strdup("Button");
}


// Access information callback for title delete button
static char *
_title_delete_btn_access_info_cb(void *data,
											Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	if (delete_mode)
	  return strdup(S_DONE);
	else
	  return strdup(S_DELETE);
}

// Access type callback for title delete all button
static char *
_title_delete_all_btn_access_type_cb(void *data,
												 Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	return strdup("Button");
}

// Access information callback for title delete all button
static char *
_title_delete_all_btn_access_info_cb(void *data,
												 Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	return strdup(S_DELETE_ALL);
}

// Access activate callback for title close button
static void
_title_close_btn_access_activate_cb(void *data,
												Evas_Object *obj,
												Elm_Object_Item *item)
{
	AppData *ad = (AppData *)data;
	if (!ad) return;

	clipdrawer_lower_view(ad);
}

// Access activate callback for title delete button
static void
_title_delete_btn_access_activate_cb(void *data,
												 Evas_Object *obj,
												 Elm_Object_Item *item)
{
	AppData *ad = (AppData *)data;
	if (!ad) return;

	_delete_mode_set(ad, !delete_mode);
}

// Access activate callback for title delete all button
static void
_title_delete_all_btn_access_activate_cb(void *data,
													  Evas_Object *obj,
													  Elm_Object_Item *item)
{
	AppData *ad = (AppData *)data;
	if (!ad) return;
	ClipdrawerData *cd = ad->clipdrawer;

	if ((item_count_get(ad, ATOM_INDEX_COUNT_ALL) - cd->locked_item_count) != 0)
		_create_cbhm_popup(ad);
}

ClipdrawerData* init_clipdrawer(AppData *ad)
{
	FN_CALL();
	const char *data;
	Evas_Object *part_obj, *access_obj;

	ClipdrawerData *cd = calloc(1, sizeof(ClipdrawerData));
	/* create and setting window */
	if (!cd) {
		ERR("calloc() Fail");
		return NULL;
	}

	if (!(cd->main_win = create_win(cd, APPNAME))) {
		ERR("create_win() Fail");
		free(cd);
		return NULL;
	}
#ifdef HAVE_X11
	if (!(cd->x_main_win = elm_win_xwindow_get(cd->main_win))) {
		free(cd);
		return NULL;
	}

	setting_win(ad->x_disp, ad->x_root_win, cd->x_main_win);
#endif
#ifdef HAVE_WAYLAND
	if (!(cd->wl_main_win = elm_win_wl_window_get(cd->main_win))) {
		ERR("elm_win_wl_window_get() Fail");
		free(cd);
		return NULL;
	}

	setting_win(ad->wl_disp, cd->wl_main_win);
#endif

	/* edj setting */
	if (!(cd->main_layout = _load_edj(cd->main_win, APP_EDJ_FILE, GRP_MAIN)))
	{
		ERR("_load_edj() Fail");
		evas_object_del(cd->main_win);
		free(cd);
		return NULL;
	}

	//double scale = elm_config_scale_get();
	Evas_Object* ly = elm_layout_edje_get(cd->main_layout);

	data = edje_object_data_get(ly, "clipboard_height");
	cd->height = data ? atoi(data) : 0;
	cd->height = ELM_SCALE_SIZE(cd->height);
	
	data = edje_object_data_get(ly, "clipboard_landscape_height");
	cd->landscape_height = data ? atoi(data) : 0;
	cd->landscape_height = ELM_SCALE_SIZE(cd->landscape_height);

	data = edje_object_data_get(ly, "grid_item_bg_w");
	cd->grid_item_bg_w = data ? atoi(data) : 0;
	cd->grid_item_bg_w = ELM_SCALE_SIZE(cd->grid_item_bg_w);

	data = edje_object_data_get(ly, "grid_item_bg_h");
	cd->grid_item_bg_h = data ? atoi(data) : 0;
	cd->grid_item_bg_h = ELM_SCALE_SIZE(cd->grid_item_bg_h);

	data = edje_object_data_get(ly, "grid_image_item_w");
	cd->grid_image_item_w = data ? atoi(data) : 0;
	cd->grid_image_item_w = ELM_SCALE_SIZE(cd->grid_image_item_w);

	data = edje_object_data_get(ly, "grid_image_item_h");
	cd->grid_image_item_h = data ? atoi(data) : 0;
	cd->grid_image_item_h = ELM_SCALE_SIZE(cd->grid_image_item_h);

	/* create and setting gengrid */
	elm_theme_extension_add(NULL, APP_EDJ_FILE);
	elm_object_signal_callback_add(cd->main_layout,
			"mouse,clicked,1", "*", clipdrawer_ly_clicked, ad);

	// Set accessibility function for title close button
	part_obj = (Evas_Object *)edje_object_part_object_get
		(elm_layout_edje_get(cd->main_layout), EDJE_CLOSE_PART_PREFIX);
	access_obj = elm_access_object_register(part_obj, cd->main_layout);

	elm_access_info_cb_set(access_obj, ELM_ACCESS_TYPE, _title_close_btn_access_type_cb, ad);
	elm_access_info_cb_set(access_obj, ELM_ACCESS_INFO, _title_close_btn_access_info_cb, ad);
	elm_access_activate_cb_set(access_obj, _title_close_btn_access_activate_cb, ad);

	// Set access function for title delete button
	part_obj = (Evas_Object *)edje_object_part_object_get
		(elm_layout_edje_get(cd->main_layout), EDJE_DELETE_MODE_PREFIX);
	access_obj = elm_access_object_register(part_obj, cd->main_layout);

	elm_access_info_cb_set(access_obj, ELM_ACCESS_TYPE, _title_delete_btn_access_type_cb, ad);
	elm_access_info_cb_set(access_obj, ELM_ACCESS_INFO, _title_delete_btn_access_info_cb, ad);
	elm_access_activate_cb_set(access_obj, _title_delete_btn_access_activate_cb, ad);

	// Set access function for title delete all button
	part_obj = (Evas_Object *)edje_object_part_object_get
		(elm_layout_edje_get(cd->main_layout), EDJE_DELETE_ALL_BTN_PART_PREFIX);
	access_obj = elm_access_object_register(part_obj, cd->main_layout);

	elm_access_info_cb_set(access_obj, ELM_ACCESS_TYPE, _title_delete_all_btn_access_type_cb, ad);
	elm_access_info_cb_set(access_obj, ELM_ACCESS_INFO, _title_delete_all_btn_access_info_cb, ad);
	elm_access_activate_cb_set(access_obj, _title_delete_all_btn_access_activate_cb, ad);

	cd->gengrid = elm_gengrid_add(cd->main_win);
	elm_object_style_set(cd->gengrid, "cbhm");
	elm_object_part_content_set(cd->main_layout, "historyitems", cd->gengrid);
	elm_gengrid_item_size_set(cd->gengrid, cd->grid_item_bg_w, cd->grid_item_bg_h);
	elm_gengrid_align_set(cd->gengrid, 0.0, 0.0);
	elm_gengrid_horizontal_set(cd->gengrid, EINA_TRUE);
//	elm_gengrid_bounce_set(cd->gengrid, EINA_TRUE, EINA_FALSE);
	elm_gengrid_multi_select_set(cd->gengrid, EINA_FALSE);
//	evas_object_smart_callback_add(cd->gengrid, "selected", _grid_click_paste, ad);
	evas_object_size_hint_weight_set(cd->gengrid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_gengrid_clear(cd->gengrid);

	evas_object_smart_callback_add(cd->gengrid, "realized", _grid_realized, NULL);

	cd->gic_image.item_style = "clipboard/image_style";
	cd->gic_image.func.text_get = NULL;
	cd->gic_image.func.content_get = _grid_image_content_get;
	cd->gic_image.func.state_get = NULL;
	cd->gic_image.func.del = NULL;

	cd->gic_text.item_style = "clipboard/text_style";
	cd->gic_text.func.text_get = _grid_text_get;
	cd->gic_text.func.content_get = _grid_content_get;
	cd->gic_text.func.state_get = NULL;
	cd->gic_text.func.del = NULL;

	cd->gic_combined.item_style = "clipboard/combined_style";
	cd->gic_combined.func.text_get = _grid_text_get;
	cd->gic_combined.func.content_get = _grid_combined_content_get;
	cd->gic_combined.func.state_get = NULL;
	cd->gic_combined.func.del = NULL;

	evas_object_show(cd->gengrid);

	ad->draw_item_add = clipdrawer_add_item;
	ad->draw_item_del = clipdrawer_del_item;
//	ad->x_main_win = cd->x_main_win;

	cd->keydown_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, keydown_cb, ad);
	cd->evas = evas_object_evas_get(cd->main_win);

	delete_mode = EINA_FALSE;
	cd->popup_activate = EINA_FALSE;
	cd->item_clicked = EINA_FALSE;
	cd->delbtn_clicked = EINA_FALSE;

	cd->lower_view_timer = NULL;

	cd->event_rect = evas_object_rectangle_add(evas_object_evas_get(cd->main_win));
	evas_object_size_hint_weight_set(cd->event_rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_color_set(cd->event_rect, 0, 0, 0, 0);
	elm_win_resize_object_add(cd->main_win, cd->event_rect);
	evas_object_repeat_events_set(cd->event_rect, EINA_TRUE);
	evas_object_show(cd->event_rect);
	evas_object_raise(cd->event_rect);

	cd->gesture_layer = elm_gesture_layer_add(cd->main_win);
	elm_gesture_layer_hold_events_set(cd->gesture_layer, EINA_FALSE);
	elm_gesture_layer_attach(cd->gesture_layer, cd->event_rect);
	elm_gesture_layer_cb_set(cd->gesture_layer, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, flick_end, ad);

	cd->noc_layout = elm_layout_add(cd->main_win);
	elm_layout_theme_set(cd->noc_layout, "layout", "nocontents", "default");
	elm_object_part_text_set(cd->noc_layout, "elm.text", S_NO_ITEMS);

	return cd;
}

void depose_clipdrawer(ClipdrawerData *cd)
{
#ifdef HAVE_X11
	utilx_ungrab_key(ecore_x_display_get(), cd->x_main_win, "XF86Back");
	utilx_ungrab_key(ecore_x_display_get(), cd->x_main_win, "XF86Home");
#endif
	evas_object_del(cd->main_win);
	if (cd->anim_timer)
		ecore_timer_del(cd->anim_timer);
	if (cd->keydown_handler)
		ecore_event_handler_del(cd->keydown_handler);
	if (cd->event_rect)
		evas_object_del(cd->event_rect);
	if (cd->gesture_layer)
	{
		elm_gesture_layer_cb_set(cd->gesture_layer, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, NULL, NULL);
		evas_object_del(cd->gesture_layer);
	}
	free(cd);
}

static Eina_Bool clipdrawer_add_item(AppData *ad, CNP_ITEM *item)
{
	Eina_Bool duplicated = EINA_FALSE;
	ClipdrawerData *cd = ad->clipdrawer;
	CNP_ITEM *gitem_data = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;
	Eina_List *itemlist = eina_list_next(ad->item_list);

	if (!item) return EINA_FALSE;

	EINA_LIST_FOREACH_SAFE(itemlist, l, l_next, gitem_data)
	{
		if ((gitem_data->type_index == item->type_index) && (!SAFE_STRCMP(item->data, gitem_data->data)))
		{
			DBG("duplicated data = %s", (char *)item->data);
			duplicated = EINA_TRUE;
			item_delete_by_CNP_ITEM(ad, gitem_data);
			break;
		}
	}

	if(item->locked)
		cd->locked_item_count++;

	if (item->gitem_style == GRID_ITEM_STYLE_TEXT)
		item->gitem = elm_gengrid_item_prepend(cd->gengrid, &cd->gic_text, item, _gengrid_select_cb, item);
	else if (item->gitem_style == GRID_ITEM_STYLE_IMAGE)
		item->gitem = elm_gengrid_item_prepend(cd->gengrid, &cd->gic_image, item, _gengrid_select_cb, item);
	else
		item->gitem = elm_gengrid_item_prepend(cd->gengrid, &cd->gic_combined, item, _gengrid_select_cb, item);

	return duplicated;
}

static Eina_Bool clipdrawer_del_item(AppData *ad, CNP_ITEM *item)
{
	if (item->gitem)
	{
		elm_object_item_del(item->gitem);
		item->gitem = NULL;
	}
	return EINA_TRUE;
}

static char *_grid_text_get(void *data, Evas_Object *obj, const char *part)
{
	CNP_ITEM *item = data;
	char text_to_show[BUFF];
	if (!item)
		return NULL;

	if (!SAFE_STRCMP(part, "elm.text")) /* text */
	{
		char *entry_text = string_for_entry_get(item->ad, item->type_index, item->data);

		/* limiting the grid text to show to avoid UI blockage in case of large text */
		if (entry_text)
		{
			SAFE_STRNCPY(text_to_show, entry_text, (BUFF - 1));
			SAFE_STRNCAT(text_to_show, "\0", 1);
		}

		if (entry_text)
			return strdup(text_to_show);
		else
			return SAFE_STRDUP(item->data);
	}
	return NULL;
}

static Evas_Object *_grid_content_get(void *data, Evas_Object *obj, const char *part)
{
	CNP_ITEM *item = data;

	if (!SAFE_STRCMP(part, "delbtn/img")) /* text */
	{
		Evas_Object *btn = elm_button_add(obj);
		elm_object_style_set(btn, "delete_icon");
		evas_object_propagate_events_set(btn,EINA_FALSE);
		evas_object_smart_callback_add(btn, "clicked", _grid_item_button_clicked, item);
		return btn;
	}
	else
		return NULL;
}


static Evas_Object *_grid_image_content_get(void *data, Evas_Object *obj, const char *part)
{
	CNP_ITEM *item = data;
	AppData *ad;
	ClipdrawerData *cd;
	Elm_Object_Item *gitem;
	Evas_Object *sicon;

	if (!item)
		return NULL;

	ad = item->ad;
	cd = ad->clipdrawer;
	gitem = item->gitem;

	if (!SAFE_STRCMP(part, "elm.swallow.content")) /* uri */
	{
		int w, h, iw, ih;
		int grid_image_real_w = cd->grid_image_item_w;
		int grid_image_real_h = cd->grid_image_item_h;
		//double scale;

		sicon = evas_object_image_filled_add(evas_object_evas_get(obj));
		evas_object_image_load_size_set(sicon, grid_image_real_w, grid_image_real_h);
		evas_object_image_file_set(sicon, item->file, NULL);
		evas_object_image_preload(sicon, EINA_FALSE);
		evas_object_image_size_get(sicon, &w, &h);

		//scale = elm_config_scale_get();

		if (w <= 0 || h <= 0)
			return NULL;

		if (w > grid_image_real_w || h > grid_image_real_h)
		{
			if (w > h)
			{
				iw = ELM_SCALE_SIZE(240);
				ih = ELM_SCALE_SIZE(159);
			}
			else if (w < h)
			{
				iw = ELM_SCALE_SIZE(104);
				ih = ELM_SCALE_SIZE(159);
			}
			else
			{
				iw = ELM_SCALE_SIZE(159);
				ih = ELM_SCALE_SIZE(159);
			}
		}
		else
		{
			iw = w;
			ih = h;
		}

		evas_object_resize(sicon, iw, ih);
		evas_object_size_hint_min_set(sicon, iw, ih);

		if (cd->paste_text_only)
			elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
		else
			elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
	}
	else if (!SAFE_STRCMP(part, "delbtn/img")) /* text */
	{
		Evas_Object *btn = elm_button_add(obj);
		elm_object_style_set(btn, "delete_icon");
		evas_object_propagate_events_set(btn,EINA_FALSE);
		evas_object_smart_callback_add(btn, "clicked", _grid_item_button_clicked, item);
		return btn;
	}
	else
		return NULL;

	return sicon;
}

static Evas_Object *_grid_combined_content_get(void *data, Evas_Object *obj, const char *part)
{
	CNP_ITEM *item = data;
	AppData *ad;
	ClipdrawerData *cd;
	Elm_Object_Item *gitem;
	Evas_Object *sicon;
	char *entry_text = NULL;

	if (!item)
		return NULL;

	ad = item->ad;
	cd = ad->clipdrawer;
	gitem = item->gitem;

	if (!SAFE_STRCMP(part, "elm.swallow.content")) /* uri */
	{
		int w, h, iw, ih;
		int grid_image_real_w = cd->grid_image_item_w;
		int grid_image_real_h = cd->grid_image_item_h;

		sicon = evas_object_image_filled_add(evas_object_evas_get(obj));
		evas_object_image_load_size_set(sicon, grid_image_real_w, grid_image_real_h);
		evas_object_image_file_set(sicon, item->file, NULL);
		evas_object_image_preload(sicon, EINA_FALSE);
		evas_object_image_size_get(sicon, &w, &h);

		if (w <= 0 || h <= 0)
			return NULL;

		if (w > COMBINED_ITEM_IMAGE_WIDTH || h > COMBINED_ITEM_IMAGE_HEIGHT )
		{
			if (h >= w)
			{
				iw = (float)grid_image_real_h / h * w;
				if (iw > COMBINED_ITEM_IMAGE_WIDTH)
				{
					ih = (float)COMBINED_ITEM_IMAGE_WIDTH / w * h;
					iw = COMBINED_ITEM_IMAGE_WIDTH;
				}
				else
				{
					ih = COMBINED_ITEM_IMAGE_HEIGHT;
				}
			}
			else
			{
				ih = (float)COMBINED_ITEM_IMAGE_WIDTH / w * h;
				iw = COMBINED_ITEM_IMAGE_WIDTH;
			}
		}
		else
		{
			iw = w;
			ih = h;
		}

		evas_object_resize(sicon, iw, ih);
		evas_object_size_hint_min_set(sicon, iw, ih);
		entry_text = string_for_entry_get(item->ad, item->type_index, item->data);

		if (entry_text)
		{
			entry_text = evas_textblock_text_markup_to_utf8(NULL, entry_text);
			if (entry_text)
				SAFE_STRNCAT(entry_text, "\0", 1);
		}

		if (cd->paste_text_only && entry_text && !strlen(entry_text))
			elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
		else
			elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
		if (entry_text)
		  free(entry_text);
	}
	else if (!SAFE_STRCMP(part, "delbtn/img")) /* text */
	{
		Evas_Object *btn = elm_button_add(obj);
		elm_object_style_set(btn, "delete_icon");
		evas_object_propagate_events_set(btn,EINA_FALSE);
		evas_object_smart_callback_add(btn, "clicked", _grid_item_button_clicked, item);
		return btn;
	}
	else
		return NULL;

	return sicon;
}

static void _grid_realized(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *gitem = (Elm_Object_Item *)event_info;
//	CNP_ITEM *item;

	if (gitem) {
//		item = elm_object_item_data_get(gitem);

		if (delete_mode)
			elm_object_item_signal_emit(gitem, "elm,state,show,delbtn", "elm");
		else
			elm_object_item_signal_emit(gitem, "elm,state,hide,delbtn", "elm");
	}
}

static void _create_cbhm_popup(AppData *ad)
{
	if (ad == NULL)
		return;

	ClipdrawerData *cd = ad->clipdrawer;
	Evas_Object *btn1;
	Evas_Object *btn2;
	int w = 0, h = 0;
	int rotations[4] = {0, 90, 180, 270};

	if (cd->popup_activate == EINA_TRUE)
		return;

	cd->popup_activate = EINA_TRUE;

	cd->popup_win = elm_win_add(NULL, "delete popup", ELM_WIN_MENU);

#ifdef HAVE_X11
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
#endif
	evas_object_resize(cd->popup_win, w, h);
	elm_win_alpha_set(cd->popup_win, EINA_TRUE);

#ifdef HAVE_X11
	ecore_x_icccm_name_class_set(elm_win_xwindow_get(cd->popup_win),"APP_POPUP", "APP_POPUP");
#endif
#ifdef HAVE_X11
	set_focus_for_app_window((unsigned int)elm_win_xwindow_get(cd->popup_win) , EINA_FALSE);
	set_transient_for((int)elm_win_xwindow_get(cd->popup_win), cd->x_main_win);
#endif
#ifdef HAVE_WAYLAND
	set_focus_for_app_window(elm_win_wl_window_get(cd->popup_win) , EINA_FALSE);
	set_transient_for(elm_win_wl_window_get(cd->popup_win), cd->wl_main_win);
#endif
	elm_win_wm_rotation_available_rotations_set(cd->popup_win, rotations, 4);
	evas_object_show(cd->popup_win);

	cd->popup_conform = elm_conformant_add(cd->popup_win);
	evas_object_size_hint_weight_set(cd->popup_conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(cd->popup_win, cd->popup_conform);
	evas_object_show(cd->popup_conform);

	elm_win_conformant_set(cd->popup_win, EINA_TRUE);

	cd->cbhm_popup = elm_popup_add(cd->popup_win);
	
	elm_object_part_text_set(cd->cbhm_popup, "title,text", S_DELETE_Q);
	elm_object_text_set(cd->cbhm_popup, S_DELETE_ALL_Q);

	btn1 = elm_button_add(cd->cbhm_popup);
	elm_object_style_set(btn1, "popup");
	elm_object_text_set(btn1, S_CANCEL);
	elm_object_part_content_set(cd->cbhm_popup, "button1", btn1);
	evas_object_smart_callback_add(btn1, "clicked", _cancel_btn_cb, ad);

	btn2 = elm_button_add(cd->cbhm_popup);
	elm_object_style_set(btn2, "popup");
	elm_object_text_set(btn2, S_DELETE);
	elm_object_part_content_set(cd->cbhm_popup, "button2", btn2);
	evas_object_smart_callback_add(btn2, "clicked", _ok_btn_cb, ad);

	evas_object_show(cd->cbhm_popup);
}

static void clipdrawer_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	AppData *ad = data;

	if (ad->clipdrawer->anim_status != STATUS_NONE)
		return;

	if (!SAFE_STRNCMP(source, EDJE_CLOSE_PART_PREFIX, SAFE_STRLEN(EDJE_CLOSE_PART_PREFIX)))
		clipdrawer_lower_view(ad);
	else if (!SAFE_STRNCMP(source, EDJE_DELETE_MODE_PREFIX, SAFE_STRLEN(EDJE_DELETE_MODE_PREFIX)))
		_delete_mode_set(ad, !delete_mode);
	else if (!SAFE_STRNCMP(source, EDJE_DELETE_ALL_BTN_PART_PREFIX, SAFE_STRLEN(EDJE_DELETE_ALL_BTN_PART_PREFIX)))
		 _create_cbhm_popup(ad);
	else
		return;
}

#ifdef HAVE_X11
static void set_focus_for_app_window(Ecore_X_Window x_main_win, Eina_Bool enable)
{
	    FN_CALL();
	    Eina_Bool accepts_focus;
	    Ecore_X_Window_State_Hint initial_state;
	    Ecore_X_Pixmap icon_pixmap;
	    Ecore_X_Pixmap icon_mask;
	    Ecore_X_Window icon_window;
	    Ecore_X_Window window_group;
	    Eina_Bool is_urgent;

	    ecore_x_icccm_hints_get (x_main_win,
        &accepts_focus, &initial_state, &icon_pixmap, &icon_mask, &icon_window, &window_group, &is_urgent);
	    ecore_x_icccm_hints_set (x_main_win,
        enable, initial_state, icon_pixmap, icon_mask, icon_window, window_group, is_urgent);
	    DBG("set focus mode = %d", enable);
}
#endif
#ifdef HAVE_WAYLAND
static void set_focus_for_app_window(Ecore_Wl_Window *wl_main_win, Eina_Bool enable)
{
}
#endif


static void _ok_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	AppData *ad = data;
	ClipdrawerData *cd = ad->clipdrawer;
	Elm_Object_Item *gitem = elm_gengrid_first_item_get(cd->gengrid);

	while (gitem)
	{
		CNP_ITEM *gitem_data = elm_object_item_data_get(gitem);
		gitem = elm_gengrid_item_next_get(gitem);

		if(!gitem_data->locked)
			item_delete_by_CNP_ITEM(ad, gitem_data);
	}

	if (item_count_get(ad, ATOM_INDEX_COUNT_ALL) == 0)
		clipdrawer_lower_view(ad);
	else
	{
		cd->popup_activate = EINA_FALSE;
		evas_object_del(cd->popup_conform);
		evas_object_del(cd->popup_win);
	}
}

static void _cancel_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	AppData *ad = data;
	ClipdrawerData *cd = ad->clipdrawer;

	cd->popup_activate = EINA_FALSE;
	evas_object_del(cd->popup_conform);
	evas_object_del(cd->popup_win);
}

static void _grid_item_button_clicked(void *data, Evas_Object *obj, void *event_info)
{
	CNP_ITEM *item = data;
	AppData *ad = item->ad;
	ClipdrawerData *cd = ad->clipdrawer;

	if (cd->anim_status != STATUS_NONE)
		return;

	item_delete_by_CNP_ITEM(ad, item);
}


#ifdef HAVE_X11
static Ecore_X_Window isf_ise_window_get()
{
	Ecore_X_Atom   x_atom_isf_control = ecore_x_atom_get("_ISF_CONTROL_WINDOW");
	Ecore_X_Atom   x_atom_isf_ise     = ecore_x_atom_get("_ISF_ISE_WINDOW");
	Ecore_X_Window x_isf_control_win = 0;
	Ecore_X_Window x_isf_ise_win     = 0;
	unsigned char *buf = NULL;
	int            num = 0;
	int            ret;

	ret = ecore_x_window_prop_property_get(0, x_atom_isf_control, ECORE_X_ATOM_WINDOW, 0, &buf, &num);
	if (ret && num)
		memcpy(&x_isf_control_win, buf, sizeof(Ecore_X_Window));
	if (buf)
		free(buf);
	if (!x_isf_control_win)
		return 0;

	ret = ecore_x_window_prop_property_get(x_isf_control_win, x_atom_isf_ise, ECORE_X_ATOM_WINDOW, 0, &buf, &num);
	if (ret && num)
		memcpy(&x_isf_ise_win, buf, sizeof(Ecore_X_Window));
	if (buf)
		free(buf);

	return x_isf_ise_win;
}

void set_transient_for(Ecore_X_Window x_main_win, Ecore_X_Window x_active_win)
{
	ecore_x_icccm_transient_for_set(x_main_win, x_active_win);
	ecore_x_event_mask_set(x_active_win,
								  ECORE_X_EVENT_MASK_WINDOW_PROPERTY |
								  ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
}

void unset_transient_for(Ecore_X_Window x_main_win)
{
	Ecore_X_Window x_transient_win = ecore_x_icccm_transient_for_get(x_main_win);

	if (x_transient_win) {
		ecore_x_event_mask_unset(x_transient_win,
										 ECORE_X_EVENT_MASK_WINDOW_PROPERTY |
										 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
		ecore_x_icccm_transient_for_unset(x_main_win);
	}
}

void setting_win(Ecore_X_Display *x_disp, Ecore_X_Window x_root_win, Ecore_X_Window x_main_win)
{
	FN_CALL();

	Ecore_X_Atom ATOM_WINDOW_EFFECT_ENABLE = 0;
	unsigned int effect_state = 0; // 0 : disabled effect // 1: enable effect

	// disable window effect
	ATOM_WINDOW_EFFECT_ENABLE = ecore_x_atom_get("_NET_CM_WINDOW_EFFECT_ENABLE");
	if (ATOM_WINDOW_EFFECT_ENABLE)
	{
		ecore_x_window_prop_card32_set(x_main_win, ATOM_WINDOW_EFFECT_ENABLE, &effect_state, 1);
	}
	else
	{
		// error case
		ERR("Could not get _NET_CM_WINDOW_EFFECT_ENABLE ATOM");
	}

	ecore_x_icccm_name_class_set(x_main_win, "NORMAL_WINDOW", "NORMAL_WINDOW");

	set_focus_for_app_window(x_main_win, EINA_FALSE);
	ecore_x_window_prop_property_set(
			x_root_win, ecore_x_atom_get("CBHM_ELM_WIN"),
			ECORE_X_ATOM_WINDOW, 32, &x_main_win, 1);
	ecore_x_flush();
}
#endif
#ifdef HAVE_WAYLAND
static Ecore_Wl_Window* wl_isf_ise_window_get()
{
	return 0;
}

void set_transient_for(Ecore_Wl_Window *wl_main_win, Ecore_Wl_Window *wl_active_win)
{
}

void unset_transient_for(Ecore_Wl_Window *wl_main_win)
{
}

void setting_win(const char *wl_disp, Ecore_Wl_Window *wl_main_win)
{
	set_focus_for_app_window(wl_main_win, EINA_FALSE);
	ecore_wl_flush();
}
#endif

Evas_Object *create_win(ClipdrawerData *cd, const char *name)
{
	Evas_Object *win = elm_win_add(NULL, name, ELM_WIN_UTILITY);
	if (!win) {
		ERR("elm_win_add() Fail");
		return NULL;
	}
	elm_win_title_set(win, name);
	elm_win_borderless_set(win, EINA_TRUE);
#ifdef HAVE_X11
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &cd->root_w, &cd->root_h);
#endif
#ifdef HAVE_WAYLAND
	ecore_wl_screen_size_get(&cd->root_w, &cd->root_h);
#endif
	DBG("root_w: %d, root_h: %d", cd->root_w, cd->root_h);
	//evas_object_resize(win, cd->root_w, cd->root_h);

	//elm_config_scale_set((double)cd->root_w/DEFAULT_WIDTH);
	return win;
}

static void set_sliding_win_geometry(AppData *ad)
{
	FN_CALL();
	ClipdrawerData *cd = ad->clipdrawer;
	Evas_Coord x, y, w, h;
	int angle = elm_win_rotation_get(cd->main_win);

#ifdef HAVE_X11
	if (!ad->x_active_win) {
		ERR("x_active_win is NULL");
		return;
	}
#endif
#ifdef HAVE_WAYLAND
	if (!ad->wl_active_win) {
//		ERR("wl_active_win is NULL");
		return;
	}
#endif

	if (angle == 90 || angle == 270) {
		h = cd->landscape_height;
		x = 0;
		y = cd->root_w - h;
		w = cd->root_h;
	} else {
		h = cd->height;
		x = 0;
		y = cd->root_h - h;
		w = cd->root_w;
	}

	if (!h)
		w = 0;

	DBG("[CBHM] change degree geometry... (%d, %d, %d x %d)", x, y, w, h);

#ifdef HAVE_X11
	ecore_x_e_illume_clipboard_geometry_set(ad->x_active_win, x, y, w, h);
#endif
#ifdef HAVE_WAYLAND
	ecore_wl_window_clipboard_geometry_set(ad->wl_active_win, x, y, w, h);
#endif
}

void set_rotation_to_clipdrawer(AppData *ad)
{
	FN_CALL();
	ClipdrawerData *cd = ad->clipdrawer;
	int angle = elm_win_rotation_get(cd->main_win);
	int x, y, w, h;

	if (angle == 180) // reverse
	{
		h = cd->height;
		x = 0;
		y = 0;
		w = cd->root_w;
	}
	else if (angle == 90) // right rotate
	{
		h = cd->landscape_height;
		x = cd->root_w - h;
		y = 0;
		w = cd->root_h;
	}
	else if (angle == 270) // left rotate
	{
		h = cd->landscape_height;
		x = 0;
		y = 0;
		w = cd->root_h;
	}
	else // angle == 0
	{
		h = cd->height;
		x = 0;
		y = cd->root_h - h;
		w = cd->root_w;
 	}

	evas_object_resize(cd->main_win, w, h);
	evas_object_move(cd->main_win, x, y);
	set_sliding_win_geometry(ad);
}
/*
static Eina_Bool _get_anim_pos(ClipdrawerData *cd, int *sp, int *ep)
{
	if (!sp || !ep)
		return EINA_FALSE;

	int angle = cd->o_degree;
	int anim_start, anim_end;

	if (angle == 180) // reverse
	{
		anim_start = -(cd->root_h - cd->height);
		anim_end = 0;
	}
	else if (angle == 90) // right rotate
	{
		anim_start = cd->root_w;
		anim_end = anim_start - cd->landscape_height;
	}
	else if (angle == 270) // left rotate
	{
		anim_start = -(cd->root_w - cd->landscape_height);
		anim_end = 0;
	}
	else // angle == 0
	{
		anim_start = cd->root_h;
		anim_end = anim_start - cd->height;
	}

	*sp = anim_start;
	*ep = anim_end;
	return EINA_TRUE;
}

static Eina_Bool _do_anim_delta_pos(ClipdrawerData *cd, int sp, int ep, int ac, int *dp)
{
	if (!dp)
		return EINA_FALSE;

	int angle = cd->o_degree;
	int delta;
	double posprop;
	posprop = 1.0*ac/ANIM_DURATION;

	if (angle == 180) // reverse
	{
		delta = (int)((ep-sp)*posprop);
		evas_object_move(cd->main_win, 0, sp+delta);
	}
	else if (angle == 90) // right rotate
	{
		delta = (int)((ep-sp)*posprop);
		evas_object_move(cd->main_win, sp+delta, 0);
	}
	else if (angle == 270) // left rotate
	{
		delta = (int)((ep-sp)*posprop);
		evas_object_move(cd->main_win, sp+delta, 0);
	}
	else // angle == 0
	{
		delta = (int)((sp-ep)*posprop);
		evas_object_move(cd->main_win, 0, sp-delta);
	}
	
	*dp = delta;

	return EINA_TRUE;
}

static void stop_animation(AppData *ad)
{
	FN_CALL();
	ClipdrawerData *cd = ad->clipdrawer;
	cd->anim_status = STATUS_NONE;
	if (cd->anim_timer)
	{
		ecore_timer_del(cd->anim_timer);
		cd->anim_timer = NULL;
	}

	set_sliding_win_geometry(ad);
}

static Eina_Bool anim_pos_calc_cb(void *data)
{
	AppData *ad = data;
	ClipdrawerData *cd = ad->clipdrawer;
	int anim_start, anim_end, delta;

	_get_anim_pos(cd, &anim_start, &anim_end);

	if (cd->anim_status == SHOW_ANIM)
	{
		if (cd->anim_count > ANIM_DURATION)
		{
			cd->anim_count = ANIM_DURATION;
			stop_animation(ad);
			return EINA_FALSE;
		}
		_do_anim_delta_pos(cd, anim_start, anim_end, cd->anim_count, &delta);
		if (cd->anim_count == 1)
			evas_object_show(cd->main_win);
		cd->anim_count++;
	}
	else if (cd->anim_status == HIDE_ANIM)
	{
		if (cd->anim_count < 0)
		{
			cd->anim_count = 0;
			elm_object_signal_emit(cd->main_layout, "elm,state,hide,historyitems", "elm");
			edje_object_message_signal_process(elm_layout_edje_get(cd->main_layout));
			evas_object_hide(cd->main_win);
			elm_win_lower(cd->main_win);
			unset_transient_for(cd->x_main_win);
			stop_animation(ad);
			_delete_mode_set(ad, EINA_FALSE);
			return EINA_FALSE;
		}
		_do_anim_delta_pos(cd, anim_start, anim_end, cd->anim_count, &delta);
		cd->anim_count--;
	}
	else
	{
		stop_animation(ad);
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

static Eina_Bool clipdrawer_anim_effect(AppData *ad, AnimStatus atype)
{
	FN_CALL();
	ClipdrawerData *cd = ad->clipdrawer;
	if (atype == cd->anim_status)
	{
		WRN("Warning: Animation effect is already in progress.");
		return EINA_FALSE;
	}

	cd->anim_status = atype;

	if (cd->anim_timer)
		ecore_timer_del(cd->anim_timer);
	cd->anim_timer = ecore_timer_add(ANIM_FLOPS, anim_pos_calc_cb, ad);

	return EINA_TRUE;
}
*/
static Eina_Bool timer_cb(void *data)
{
	ClipdrawerData *cd = data;
	/* If the gengrid has some images, evas would have some heavy works for image drawings.
		In this case, window would be delayed in showing up.
		In order to avoid this problem,
		we show the gengrid after window is shown.
	 */
	elm_object_signal_emit(cd->main_layout, "elm,state,show,historyitems", "elm");
	return ECORE_CALLBACK_CANCEL;
}

void rotate_cb(void *data, Evas_Object * obj, void *event)
{
	if (!data) return;

	AppData *ad= data;

	set_rotation_to_clipdrawer(ad);
}

void clipdrawer_activate_view(AppData* ad)
{
	FN_CALL();
	ClipdrawerData                *cd = ad->clipdrawer;
#ifdef HAVE_X11
	Ecore_X_Window                 x_transient_win = ad->x_active_win;
	Ecore_X_Window                 x_isf_ise_win = 0;
	Ecore_X_Virtual_Keyboard_State isf_ise_state;
#endif
#ifdef HAVE_WAYLAND
/* FIXME : error: storage size of 'wl_transient_win' isn't known */
//	Ecore_Wl_Window                 wl_transient_win = ad->wl_active_win;
#endif
#ifdef HAVE_WAYLAND
	Ecore_Wl_Window                 *wl_isf_ise_win;
	Ecore_Wl_Virtual_Keyboard_State isf_ise_state;
	Ecore_Wl_Input *input = NULL;
	const char *types[10] = {0, };
	int i = -1;
#endif
	int rotations[4] = { 0, 90, 180, 270 };

	if(cd->main_layout)
	{
		if ((item_count_get(ad, ATOM_INDEX_COUNT_ALL) - cd->locked_item_count) == 0)
			elm_object_signal_emit(cd->main_layout, "elm,state,disable,del", "elm");

		elm_object_part_text_set(cd->main_layout, "panel_title", S_CLIPBOARD);
		elm_object_part_text_set(cd->main_layout, "panel_function_delete_all", S_DELETE_ALL);
		elm_object_part_text_set(cd->main_layout, "panel_function_delete", S_DELETE);
	}

	if (cd->main_win)
	{
#ifdef HAVE_X11
		isf_ise_state = ecore_x_e_virtual_keyboard_state_get(ad->x_active_win);
		if (isf_ise_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON) {
			x_isf_ise_win = isf_ise_window_get();
			if (x_isf_ise_win) {
				x_transient_win = x_isf_ise_win;
				ecore_x_e_illume_clipboard_state_set(x_transient_win, ECORE_X_ILLUME_CLIPBOARD_STATE_ON);
			}
		}
		set_transient_for(cd->x_main_win, x_transient_win);
#endif
#ifdef HAVE_WAYLAND
		isf_ise_state = ecore_wl_window_keyboard_state_get(ad->wl_active_win);
		if (isf_ise_state == ECORE_WL_VIRTUAL_KEYBOARD_STATE_ON)
		{
			wl_isf_ise_win = wl_isf_ise_window_get();
			if (wl_isf_ise_win)
			{
				ecore_wl_window_clipboard_state_set(wl_isf_ise_win, EINA_TRUE);
			}
		}
		/* FIXME : error: storage size of 'wl_transient_win' isn't known */
//		set_transient_for(cd->wl_main_win, wl_transient_win);
#endif


		elm_win_wm_rotation_available_rotations_set(cd->main_win, rotations, 4);
		evas_object_smart_callback_add(cd->main_win, "wm,rotation,changed", rotate_cb, ad);
		_delete_mode_set(ad, EINA_FALSE);
		set_rotation_to_clipdrawer(ad);
		evas_object_show(cd->main_win);
		elm_win_activate(cd->main_win);
		INFO("clipboard window is shown");

#ifdef HAVE_X11
		ecore_x_e_illume_clipboard_state_set(ad->x_active_win, ECORE_X_ILLUME_CLIPBOARD_STATE_ON);
		utilx_grab_key(ad->x_disp, cd->x_main_win, "XF86Back", TOP_POSITION_GRAB);
		utilx_grab_key(ad->x_disp, cd->x_main_win, "XF86Home", SHARED_GRAB);
#endif
#ifdef HAVE_WAYLAND
		ecore_wl_window_clipboard_state_set(ad->wl_active_win, EINA_TRUE);
		//TODO: review the parameters
		ecore_wl_window_keygrab_set(ad->wl_active_win, "XF86BACK", 0, 0, 0, ECORE_WL_WINDOW_KEYGRAB_TOPMOST);
		ecore_wl_window_keygrab_set(ad->wl_active_win, "XF86HOME", 0, 0, 0, ECORE_WL_WINDOW_KEYGRAB_TOPMOST);

		input = ecore_wl_input_get();
		//FIXME: types support should re-written
		types[++i] = "application/x-elementary-markup";
		ecore_wl_dnd_selection_set(input, types);

#endif
		ecore_timer_add(0.125, timer_cb, cd);  //0.125 is experimentally decided.
	}

	if (item_count_get(ad, ATOM_INDEX_COUNT_ALL) == 0)
	{
		elm_object_part_content_unset(cd->main_layout, "historyitems");
		evas_object_hide(cd->gengrid);
		elm_object_part_content_set(cd->main_layout, "historyitems", cd->noc_layout);
	}
	else
	{
		elm_object_part_content_unset(cd->main_layout, "historyitems");
		evas_object_hide(cd->noc_layout);
		elm_object_part_content_set(cd->main_layout, "historyitems", cd->gengrid);
	}
}

static Eina_Bool clipdrawer_lower_view_timer_cb(void *data)
{
	FN_CALL();
	AppData *ad = (AppData *)data;
	ClipdrawerData *cd = ad->clipdrawer;

	cd->lower_view_timer = NULL;

	if (cd->main_win)
	{
		elm_object_signal_emit(cd->main_layout, "elm,state,hide,historyitems", "elm");
		edje_object_message_signal_process(elm_layout_edje_get(cd->main_layout));
		Elm_Object_Item *it = elm_gengrid_first_item_get (cd->gengrid);
		elm_gengrid_item_show(it, ELM_GENGRID_ITEM_SCROLLTO_NONE);
		evas_object_hide(cd->main_win);
		INFO("clipboard window is hidden");
		elm_win_lower(cd->main_win);
#ifdef HAVE_X11
		unset_transient_for(cd->x_main_win);
#endif
#ifdef HAVE_WAYLAND
		unset_transient_for(cd->wl_main_win);
#endif
		_delete_mode_set(ad, EINA_FALSE);
	}

	return ECORE_CALLBACK_CANCEL;
}

void clipdrawer_lower_view(AppData* ad)
{
	FN_CALL();
	ClipdrawerData *cd = ad->clipdrawer;
#ifdef HAVE_X11
	Ecore_X_Window                 x_isf_ise_win = 0;
	Ecore_X_Virtual_Keyboard_State isf_ise_state;
#endif

	if (cd->lower_view_timer) return;

	if (cd->main_win)
	{
#ifdef HAVE_X11
		ecore_x_e_illume_clipboard_state_set(ad->x_active_win, ECORE_X_ILLUME_CLIPBOARD_STATE_OFF);
		ecore_x_e_illume_clipboard_geometry_set(ad->x_active_win, 0, 0, 0, 0);

		isf_ise_state = ecore_x_e_virtual_keyboard_state_get(ad->x_active_win);
		if (isf_ise_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON) {
			x_isf_ise_win = isf_ise_window_get();

			if (x_isf_ise_win)
				ecore_x_e_illume_clipboard_state_set(x_isf_ise_win, ECORE_X_ILLUME_CLIPBOARD_STATE_OFF);
		}

		utilx_ungrab_key(ad->x_disp, cd->x_main_win, "XF86Back");
		utilx_ungrab_key(ad->x_disp, cd->x_main_win, "XF86Home");
#endif
#ifdef HAVE_WAYLAND
		ecore_wl_window_clipboard_state_set(ad->wl_active_win, EINA_FALSE);
		ecore_wl_window_clipboard_geometry_set(ad->wl_active_win, 0, 0, 0, 0);
		Ecore_Wl_Virtual_Keyboard_State isf_ise_state;
		isf_ise_state = ecore_wl_window_keyboard_state_get(ad->wl_active_win);
		if (isf_ise_state == ECORE_WL_VIRTUAL_KEYBOARD_STATE_ON)
		{
			Ecore_Wl_Window *wl_isf_ise_win;
			wl_isf_ise_win = wl_isf_ise_window_get();
			ecore_wl_window_clipboard_state_set(wl_isf_ise_win, EINA_FALSE);
		}
		//TODO: review parameters
		ecore_wl_window_keygrab_unset(ad->wl_active_win, "XF86BACK", 0, 0);
		ecore_wl_window_keygrab_unset(ad->wl_active_win, "XF86HOME", 0, 0);
#endif

		if(cd->popup_activate)
		{
			cd->popup_activate = EINA_FALSE;
			evas_object_del(cd->popup_conform);
			evas_object_del(cd->popup_win);
		}
	}

	/* FIXME : This callback is sometimes not called. Does we need to call func
	 * directly without using timer? */
	cd->lower_view_timer = ecore_timer_add(TIME_DELAY_LOWER_VIEW,
			clipdrawer_lower_view_timer_cb, ad);
}

void _delete_mode_set(AppData* ad, Eina_Bool del_mode)
{
	ClipdrawerData *cd = ad->clipdrawer;
	Elm_Object_Item *gitem = elm_gengrid_first_item_get(cd->gengrid);
	CNP_ITEM *item = NULL;

	if (gitem)
		delete_mode = del_mode;
	else
		delete_mode = EINA_FALSE;

	if(!cd->main_layout)
		return;
	if (delete_mode)
	{
		elm_object_part_text_set(cd->main_layout, "panel_function_delete", S_DONE);
		elm_object_signal_emit(cd->main_layout, "elm,state,hide,delete_all", "elm");
	}
	else
	{
		elm_object_part_text_set(cd->main_layout, "panel_function_delete", S_DELETE);

		if (item_count_get(ad, ATOM_INDEX_COUNT_ALL) == 0)
		{
		   elm_object_signal_emit(cd->main_layout, "elm,state,show,delete_all", "elm");
		   elm_object_signal_emit(cd->main_layout, "elm,state,disable,del", "elm");
		   elm_object_part_content_unset(cd->main_layout, "historyitems");
		   evas_object_hide(cd->gengrid);
		   elm_object_part_content_set(cd->main_layout, "historyitems", cd->noc_layout);
		}
		else
		   elm_object_signal_emit(cd->main_layout, "elm,state,show,delete_all", "elm");
	}

	while (gitem)
	{
		item = elm_object_item_data_get(gitem);
		if(!item)
			return;

		if (!item->locked)
		{
			if (delete_mode)
				elm_object_item_signal_emit(gitem, "elm,state,show,delbtn", "elm");
			else
				elm_object_item_signal_emit(gitem, "elm,state,hide,delbtn", "elm");
		}

		gitem = elm_gengrid_item_next_get(gitem);
	}
}
