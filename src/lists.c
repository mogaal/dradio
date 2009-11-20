/****************************************************************************
 ** DRadio - a Danmarks Radio netradio player.
 **
 ** Copyright (C) 2009  Jess Thrysoee
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 *****************************************************************************/

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "lists.h"


/**
 * configuration.c 
 *
 *  (linked list) contents of ~/.config/dradio/menu.xml
 */

CONF_LIST* conf_list_init()
{
   CONF_LIST *new_list = (CONF_LIST*) malloc(sizeof(CONF_LIST));
   new_list->size = 0;
   new_list->head = NULL;
   return new_list;
}


/** add to list */

void conf_list_add(CONF_LIST *list, const char *label, const char *src, enum SRC_TYPE srctype)
{
   CONF_ITEM *new_item = (CONF_ITEM*) malloc(sizeof(CONF_ITEM));

   new_item->label = label && *label ? strdup(label) : NULL;
   new_item->src =  src && *src ? strdup(src) : NULL;
   new_item->srctype = srctype;

   new_item->next = list->head;
   list->head = new_item;
   list->size++;
}


/** free list */

void conf_list_free(CONF_LIST *list)
{
   CONF_ITEM *next, *cur;

   if (!list)
      return;

   cur = list->head;

   while (cur)
   {
      free(cur->label);
      free(cur->src);
      next = cur->next;
      free(cur);
      cur = next;
   }
   free(list);
}


/**
 *   podcast.c
 *
 * (linked list) results from parsing a single podcast RSS 2.0 url.
 */

RSS_LIST* rss_list_init()
{
   RSS_LIST *new_list = (RSS_LIST*) malloc(sizeof(RSS_LIST));
   new_list->size = 0;
   new_list->head = NULL;
   new_list->tail = NULL;
   return new_list;
}


/** add to list */

void rss_list_add(RSS_LIST *list, const char *url, struct tm pubdate)
{
   RSS_ITEM *new_item = (RSS_ITEM*) malloc(sizeof(RSS_ITEM));

   new_item->url =  url && *url ? strdup(url) : NULL;
   new_item->pubdate = pubdate;

   new_item->next = list->head;

   if (list->head)
      list->head->prev = new_item;

   list->head = new_item;

   if (!list->tail)
      list->tail = new_item;

   list->size++;
}


/** free list */

void rss_list_free(RSS_LIST *list)
{
   RSS_ITEM *next, *cur;

   if (!list)
      return;

   cur = list->head;

   while (cur)
   {
      free(cur->url);
      next = cur->next;
      free(cur);
      cur = next;
   }
   free(list);
}


/**
 *   podcast.c
 *
 * (linked list) collection of previously fetched podcasts (RSS_LIST's) 
 */

RSSCACHE_LIST* rsscache_list_init()
{
   RSSCACHE_LIST *new_list = (RSSCACHE_LIST*) malloc(sizeof(RSSCACHE_LIST));
   new_list->size = 0;
   new_list->head = NULL;
   return new_list;
}


/** add to list */

void rsscache_list_add(RSSCACHE_LIST *list, const char *xmlurl, RSS_LIST *rsslist)
{
   RSSCACHE_ITEM *new_item = (RSSCACHE_ITEM*) malloc(sizeof(RSSCACHE_ITEM));

   new_item->xmlurl = xmlurl && *xmlurl ? strdup(xmlurl) : NULL;
   new_item->rsslist = rsslist;

   new_item->next = list->head;
   list->head = new_item;
   list->size++;
}


/** free list */

void rsscache_list_free(RSSCACHE_LIST *list)
{
   RSSCACHE_ITEM *next, *cur;

   if (!list)
      return;

   cur = list->head;

   while (cur)
   {
      free(cur->xmlurl);
      rss_list_free(cur->rsslist);
      next = cur->next;
      free(cur);
      cur = next;
   }
   free(list);
}

