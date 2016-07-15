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

#include "cbhmd.h"
#include "cbhmd_storage.h"

#include <Ecore_File.h>

#include "cbhmd_item.h"
#include "cbhmd_utils.h"

#define STORAGE_FILEPATH tzplatform_mkpath(TZ_USER_DATA, "cbhm/.cbhm_data")
#define STORAGE_KEY_INDEX_FORMAT "<index%02d>"
#define STORAGE_KEY_ITEM_FORMAT "<item%02d%s>"
#define STORAGE_INDEX_ITEM_NONE 0.0

static void
_dump_items(Cbhmd_Storage_Data *sd)
{
#if 0
   FN_CALL();
   int i;

   RET_IF(NULL == sd);

   for (i = 0; i < ITEM_CNT_MAX; i++)
     {
        Cbhmd_Cnp_Item *item = _storage_item_load(sd, i);
        if (item)
          DBG("item #%d type_index: 0x%x, gitem_style: 0x%x, data: %s\n, len: %d\n, file: %s\n, file_len: %d\n",
              i, item->type_index, item->gitem_style, (char *)item->data, item->len, (char *)item->file, item->file_len);
        free(item);
     }
#endif
}

static int
_storage_min_index_get(indexType *indexTable, int len)
{
   int i = 0;
   int minIndex;
   indexType min;
   min = indexTable[i];
   minIndex = i;

   for (i = 1; i < len; i++)
     {
        if ((min > indexTable[i]))
          {
             min = indexTable[i];
             minIndex = i;
          }
     }
   return minIndex;
}

static int
_storage_max_index_get(indexType *indexTable, int len)
{
   int i = 0;
   indexType max = indexTable[i];
   int maxIndex = i;
   for (i = 1; i < len; i++)
     {
        if (max < indexTable[i])
          {
             max = indexTable[i];
             maxIndex = i;
          }
     }
   return maxIndex;
}

static Eina_Bool
_storage_item_eet_write(Eet_File *ef, int index, Cbhmd_Cnp_Item *item)
{
   if (!ef)
     {
        ERR("eet_file is NULL");
        return EINA_FALSE;
     }
   if (!item)
     {
        ERR("item is NULL");
        return EINA_FALSE;
     }

   Eina_Bool ret = EINA_FALSE;
   char datakey[20];
   char write_data[10];

   if (item->len > 0)
     {
        snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index,
                 "data");
        ret = eet_write(ef, datakey, item->data, item->len, 1);
     }

   if (item->file_len > 0)
     {
        snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index,
                 "file");
        ret &= eet_write(ef, datakey, item->file, item->file_len, 1);
     }

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index,
            "type_index");
   snprintf(write_data, sizeof(write_data), "%d", item->type_index);
   ret &= eet_write(ef, datakey, write_data, strlen(write_data) + 1, 1);

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index,
            "gitem_style");
   snprintf(write_data, sizeof(write_data), "%d", item->gitem_style);
   ret &= eet_write(ef, datakey, write_data, strlen(write_data) + 1, 1);

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index, "locked");
   snprintf(write_data, sizeof(write_data), "%d", item->locked);
   ret &= eet_write(ef, datakey, write_data, strlen(write_data) + 1, 1);

   eet_sync(ef);
   DBG("write result: %d, item index: %d", ret, index);

   return ret != 0;
}

static Eina_Bool
_storage_item_eet_delete(Eet_File *ef, int index)
{
   if (!ef)
     {
        ERR("eet_file is NULL");
        return EINA_FALSE;
     }

   Eina_Bool ret;
   char datakey[20];

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index, "data");
   ret = eet_delete(ef, datakey);

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index,
            "type_index");
   ret &= eet_delete(ef, datakey);

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index,
            "gitem_style");
   ret &= eet_delete(ef, datakey);

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index, "locked");
   ret &= eet_delete(ef, datakey);

   eet_sync(ef);
   DBG("delete result: %d, item index: %d", ret, index);

   return ret != 0;
}

static Eina_Bool
_storage_item_update(Cbhmd_App_Data *ad, Cbhmd_Cnp_Item *item)
{
   FN_CALL();
   Cbhmd_Storage_Data *sd = ad->storage;
   Cbhmd_Cnp_Item *temp;
   Eina_Bool ret = EINA_FALSE;
   int index;
   char datakey[20];
   char write_data[10];

   if (!item)
     {
        ERR("item is NULL");
        return EINA_FALSE;
     }

   for (index = 0; index < ITEM_CNT_MAX; index++)
     {
        temp = sd->itemTable[index];

        if (temp && (item->type_index == temp->type_index)
            && (!SAFE_STRCMP(item->data, temp->data))) break;
     }
   //delete before value.
   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index, "locked");
   ret &= eet_delete(sd->ef, datakey);
   eet_sync(sd->ef);

   //rewrite locked value.
   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index, "locked");
   snprintf(write_data, sizeof(write_data), "%d", item->locked);
   ret &= eet_write(sd->ef, datakey, write_data, strlen(write_data) + 1, 1);
   eet_sync(sd->ef);

   return ret != 0;

}

static Cbhmd_Cnp_Item *
_storage_item_load(Cbhmd_Storage_Data *sd, int index)
{
   if (!sd->ef)
     {
        ERR("eet_file is NULL");
        return NULL;
     }
   if (index >= ITEM_CNT_MAX) return NULL;

   char datakey[20];
   char *read_data;
   int read_size;
   Cbhmd_Cnp_Item *item = CALLOC(1, sizeof(Cbhmd_Cnp_Item));

   if (!item)
     {
        ERR("item CALLOC failed");
        return NULL;
     }

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index, "data");
   read_data = eet_read(sd->ef, datakey, &read_size);
   if (read_data)
     {
        if (read_size > 0)
          {
             item->len = read_size;
             item->data = CALLOC(1, read_size * sizeof(char));
             if (item->data)
               {
                  memcpy(item->data, read_data, read_size);
               }
             else
               {
                  free(read_data);
                  FREE(item);
                  return NULL;
               }
          }
        free(read_data);
     }

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index, "file");
   read_data = eet_read(sd->ef, datakey, &read_size);
   if (read_data)
     {
        if (read_size > 0)
          {
             item->file_len = read_size;
             item->file = CALLOC(1, read_size * sizeof(char));
             if (item->file) memcpy(item->file, read_data, read_size);
          }
        free(read_data);
     }

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index,
            "type_index");
   read_data = eet_read(sd->ef, datakey, &read_size);
   if (read_data)
     {
        if (read_size > 0)
          {
             item->type_index = atoi(read_data);
          }
        free(read_data);
     }

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index,
            "gitem_style");
   read_data = eet_read(sd->ef, datakey, &read_size);
   if (read_data)
     {
        if (read_size > 0)
          {
             item->gitem_style = atoi(read_data);
          }
        free(read_data);
     }

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_ITEM_FORMAT, index, "locked");
   read_data = eet_read(sd->ef, datakey, &read_size);
   if (read_data)
     {
        if (read_size > 0)
          {
             item->locked = atoi(read_data);
          }
        free(read_data);
     }

   return item;
}

static Eina_Bool
_storage_index_write(Cbhmd_Storage_Data *sd, int index)
{
   FN_CALL();
   Eina_Bool ret;
   char datakey[20];
   char write_data[50];

   if (!sd->ef)
     {
        ERR("eet_file is NULL");
        return EINA_FALSE;
     }

   snprintf(datakey, sizeof(datakey), STORAGE_KEY_INDEX_FORMAT, index);
   snprintf(write_data, sizeof(write_data), "%lf", sd->indexTable[index]);
   ret = eet_write(sd->ef, datakey, write_data, strlen(write_data) + 1, 1);

   eet_sync(sd->ef);

   return ret != 0;
}

static Eina_Bool
_storage_item_add(Cbhmd_App_Data *ad, Cbhmd_Cnp_Item *item)
{
   FN_CALL();
   Cbhmd_Storage_Data *sd = ad->storage;
   Cbhmd_Cnp_Item *temp;
   Eina_Bool ret = EINA_TRUE;
   int index;

   if (!item)
     {
        ERR("item is NULL");
        return EINA_FALSE;
     }

   for (index = 0; index < ITEM_CNT_MAX; index++)
     {
        temp = sd->itemTable[index];

        if (temp && (item->type_index == temp->type_index)
            && (!SAFE_STRCMP(item->data, temp->data))) break;
     }

   // Item does not exist in clipboard
   if (index == ITEM_CNT_MAX)
     {
        index = _storage_min_index_get(sd->indexTable, ITEM_CNT_MAX);
        ret = _storage_item_eet_write(sd->ef, index, item);
     }

   sd->indexTable[index] = ecore_time_unix_get();
   sd->itemTable[index] = item;
   ret &= _storage_index_write(sd, index);
   _dump_items(sd);
   return ret;
}

static Eina_Bool
_storage_item_del(Cbhmd_App_Data *ad, Cbhmd_Cnp_Item *item)
{
   FN_CALL();
   Cbhmd_Storage_Data *sd = ad->storage;
   Eina_Bool ret = EINA_FALSE;
   int index;

   for (index = 0; index < ITEM_CNT_MAX; index++)
     {
        if (sd->itemTable[index] == item) break;
     }

   if (index < ITEM_CNT_MAX)
     {
        ret = _storage_item_eet_delete(sd->ef, index);
        sd->indexTable[index] = STORAGE_INDEX_ITEM_NONE;
        sd->itemTable[index] = NULL;
        ret &= _storage_index_write(sd, index);
     }
   return ret;
}

Cbhmd_Storage_Data *
cbhmd_storage_init(Cbhmd_App_Data *ad)
{
   FN_CALL();
   Cbhmd_Storage_Data *sd = CALLOC(1, sizeof(Cbhmd_Storage_Data));
   if (!sd) return EINA_FALSE;

   eet_init();
   ecore_file_init();

   sd->ef = eet_open(STORAGE_FILEPATH, EET_FILE_MODE_READ_WRITE);

   if (sd->ef)
     {
        char datakey[20];
        int i, j;
        int index;
        int index_order[ITEM_CNT_MAX];
        int read_size;
        indexType *read_data;
        indexType temp[ITEM_CNT_MAX];

        // Initialize index data in file
        for (i = 0; i < ITEM_CNT_MAX; i++)
          {
             sd->itemTable[i] = NULL;
             sd->indexTable[i] = STORAGE_INDEX_ITEM_NONE;
          }

        // Load index data from file
        for (i = 0; i < ITEM_CNT_MAX; i++)
          {
             snprintf(datakey, sizeof(datakey), STORAGE_KEY_INDEX_FORMAT, i);
             read_data = eet_read(sd->ef, datakey, &read_size);
             if (read_data && read_size > 0)
               temp[i] = atol((char *)read_data);
             else temp[i] = STORAGE_INDEX_ITEM_NONE;
             free(read_data);
          }

        // Load item data from file
        for (i = 0, index = 0; i < ITEM_CNT_MAX; i++)
          {
             int maxIndex = _storage_max_index_get(temp, ITEM_CNT_MAX);
             if (temp[maxIndex] == STORAGE_INDEX_ITEM_NONE)
               break;
             else
               {
                  sd->itemTable[maxIndex] = _storage_item_load(sd, maxIndex);
                  sd->indexTable[maxIndex] = temp[maxIndex];
                  temp[maxIndex] = STORAGE_INDEX_ITEM_NONE;
                  index_order[index] = maxIndex;
                  index++;
               }
          }

        // Add loaded item to clipboard
        if (index > 0)
          {
             for (i = index - 1; i >= 0; i--)
               {
                  j = index_order[i];
                  if (sd->itemTable[j])
                    cbhmd_item_add_by_cnp_item(ad, sd->itemTable[j], EINA_FALSE,
                                               EINA_FALSE);
               }
          }
        else
          {
             DBG("load storage index failed");
          }
     }
   else DBG("storage ef is NULL");

   _dump_items(sd);

   ad->Storage_Item_Add_Cb = _storage_item_add;
   ad->Storage_Item_Del_Cb = _storage_item_del;
   ad->Storage_Item_Load_Cb = _storage_item_load;
   ad->Storage_Item_Update_Cb = _storage_item_update;

   return sd;
}

void
cbhmd_storage_deinit(Cbhmd_Storage_Data *sd)
{
   FN_CALL();
   _dump_items(sd);
   if (sd->ef) eet_close(sd->ef);
   sd->ef = NULL;
   eet_shutdown();
   ecore_file_shutdown();
}
