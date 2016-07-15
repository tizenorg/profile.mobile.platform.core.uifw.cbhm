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
#include "cbhmd_x_drawer.h"

#define CBHMD_X_DRAWER_WINDOW_TITLE_STRING "X11_CLIPBOARD_HISTORY_MANAGER"

void cbhmd_x_drawer_focus_set(Ecore_X_Window x_main_win, Eina_Bool enable)
{
   FN_CALL();
   Eina_Bool accepts_focus;
   Ecore_X_Window_State_Hint initial_state;
   Ecore_X_Pixmap icon_pixmap;
   Ecore_X_Pixmap icon_mask;
   Ecore_X_Window icon_window;
   Ecore_X_Window window_group;
   Eina_Bool is_urgent;

   ecore_x_icccm_hints_get(x_main_win, &accepts_focus, &initial_state,
                           &icon_pixmap, &icon_mask, &icon_window, &window_group, &is_urgent);
   ecore_x_icccm_hints_set(x_main_win, enable, initial_state, icon_pixmap,
                           icon_mask, icon_window, window_group, is_urgent);
   DBG("set focus mode = %d", enable);
}

int cbhmd_x_drawer_effect_and_focus_set(Cbhmd_App_Data *ad)
{
   FN_CALL();

   RETV_IF(NULL == ad, CBHM_ERROR_INVALID_PARAMETER);
   RETV_IF(NULL == ad->x_root_win, CBHM_ERROR_INVALID_PARAMETER);
   RETV_IF(NULL == ad->drawer, CBHM_ERROR_INVALID_PARAMETER);
   RETV_IF(NULL == ad->drawer->x_main_win, CBHM_ERROR_INVALID_PARAMETER);

   Cbhmd_Drawer_Data *dd = ad->drawer;
   Ecore_X_Window x_root_win = ad->x_root_win;
   Ecore_X_Window x_main_win = dd->x_main_win;

   Ecore_X_Atom ATOM_WINDOW_EFFECT_ENABLE = 0;
   unsigned int effect_state = 0; /* 0 : disabled effect, 1: enable effect */

   /* disable window effect */
   ATOM_WINDOW_EFFECT_ENABLE = ecore_x_atom_get("_NET_CM_WINDOW_EFFECT_ENABLE");
   if (ATOM_WINDOW_EFFECT_ENABLE)
     {
        ecore_x_window_prop_card32_set(x_main_win, ATOM_WINDOW_EFFECT_ENABLE,
                                       &effect_state, 1);
     }
   else
     {
        /* error case */
        ERR("Could not get _NET_CM_WINDOW_EFFECT_ENABLE ATOM");
     }

   ecore_x_icccm_name_class_set(x_main_win, "NORMAL_WINDOW", "NORMAL_WINDOW");

   cbhmd_drawer_focus_set(ad, EINA_FALSE);
   ecore_x_window_prop_property_set(x_root_win,
                                    ecore_x_atom_get("CBHM_ELM_WIN"), ECORE_X_ATOM_WINDOW, 32, &x_main_win,
                                    1);
   ecore_x_flush();

   return CBHM_ERROR_NONE;
}

void cbhmd_x_drawer_set_transient(Ecore_X_Window transient_win,
                                  Ecore_X_Window toplevel_win)
{
   RET_IF(NULL == transient_win);
   RET_IF(NULL == toplevel_win);

   ecore_x_icccm_transient_for_set(transient_win, toplevel_win);
   ecore_x_event_mask_set(toplevel_win,
                          ECORE_X_EVENT_MASK_WINDOW_PROPERTY
                          | ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
}

void cbhmd_x_drawer_unset_transient(Ecore_X_Window x_main_win)
{
   Ecore_X_Window x_transient_win = ecore_x_icccm_transient_for_get(x_main_win);

   if (x_transient_win)
     {
        ecore_x_event_mask_unset(x_transient_win,
                                 ECORE_X_EVENT_MASK_WINDOW_PROPERTY
                                 | ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
        ecore_x_icccm_transient_for_unset(x_main_win);
     }
}

void cbhmd_x_drawer_event_window_set_title(Ecore_X_Window x_event_win,
                                           Ecore_X_Window x_root_win)
{
   ecore_x_netwm_name_set(x_event_win, CBHMD_X_DRAWER_WINDOW_TITLE_STRING);
   ecore_x_event_mask_set(x_event_win, ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
   ecore_x_event_mask_set(x_root_win, ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
   ecore_x_window_prop_property_set(x_root_win, ecore_x_atom_get("CBHM_XWIN"),
                                    XA_WINDOW, 32, &x_event_win, 1);
   ecore_x_flush();
}

int cbhmd_x_drawer_event_window_create(Cbhmd_App_Data *ad)
{
   ad->x_disp = ecore_x_display_get();
   DBG("x_disp: 0x%p", ad->x_disp);
   if (ad->x_disp)
     {
        Ecore_X_Atom clipboard_manager_atom = XInternAtom(ad->x_disp,
                                                          CBHMD_ATOM_CLIPBOARD_MANAGER_NAME, False);
        Ecore_X_Window clipboard_manager = XGetSelectionOwner(ad->x_disp,
                                                              clipboard_manager_atom);
        DBG("clipboard_manager_window: 0x%x", clipboard_manager);
        if (!clipboard_manager)
          {
             ad->x_root_win = DefaultRootWindow(ad->x_disp);
             if (ad->x_root_win)
               {
                  ad->x_event_win = ecore_x_window_new(ad->x_root_win, 0, 0, 19, 19);
                  DBG("x_event_win: 0x%x", ad->x_event_win);
                  if (ad->x_event_win)
                    {
                       XSetSelectionOwner(ad->x_disp, clipboard_manager_atom,
                                          ad->x_event_win, CurrentTime);
                       Ecore_X_Window clipboard_manager = XGetSelectionOwner(ad->x_disp,
                                                                             clipboard_manager_atom);
                       DBG("clipboard_manager: 0x%x", clipboard_manager);
                       if (ad->x_event_win == clipboard_manager)
                         {
                            return CBHM_ERROR_NONE;
                         }
                    }
               }
          }
     }

   return CBHM_ERROR_NOT_SUPPORTED;
}

Ecore_X_Window cbhmd_x_drawer_isf_ise_window_get()
{
   Ecore_X_Atom x_atom_isf_control = ecore_x_atom_get("_ISF_CONTROL_WINDOW");
   Ecore_X_Atom x_atom_isf_ise = ecore_x_atom_get("_ISF_ISE_WINDOW");
   Ecore_X_Window x_isf_control_win = 0;
   Ecore_X_Window x_isf_ise_win = 0;
   unsigned char *buf = NULL;
   int num = 0;
   int ret;

   ret = ecore_x_window_prop_property_get(0, x_atom_isf_control,
                                          ECORE_X_ATOM_WINDOW, 0, &buf, &num);
   if (ret && num)
     memcpy(&x_isf_control_win, buf, sizeof(Ecore_X_Window));
   if (buf)
     free(buf);
   if (!x_isf_control_win)
     return 0;

   ret = ecore_x_window_prop_property_get(x_isf_control_win, x_atom_isf_ise,
                                          ECORE_X_ATOM_WINDOW, 0, &buf, &num);
   if (ret && num)
     memcpy(&x_isf_ise_win, buf, sizeof(Ecore_X_Window));
   if (buf)
     free(buf);

   return x_isf_ise_win;
}
