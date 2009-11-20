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

#ifndef LISTS_H
#define LISTS_H

#include <time.h>

enum SRC_TYPE {   /* the XML_ATTR_TYPE types */
   direct,
   playlist,
   rss
};

/*
 *   configuration.c
 *
 * (linked list) contents of ~/.config/dradio/menu.xml
 */
struct CONF_ITEM 
{
   char *label;
   char *src;
   enum SRC_TYPE srctype;

   struct CONF_ITEM *next;

};
typedef struct CONF_ITEM CONF_ITEM;

struct CONF_LIST
{
   int size;
   CONF_ITEM *head;

};
typedef struct CONF_LIST CONF_LIST;

CONF_LIST* conf_list_init();
void       conf_list_free(CONF_LIST *list);
void       conf_list_add(CONF_LIST *list, const char *label, const char *src, enum SRC_TYPE srctype);


/*
 *   podcast.c
 *
 * (linked list) results from parsing a single podcast RSS 2.0 url.
 */
struct RSS_ITEM 
{
   char *url;
   struct tm pubdate;

   struct RSS_ITEM *next;
   struct RSS_ITEM *prev;

};
typedef struct RSS_ITEM RSS_ITEM;

struct RSS_LIST
{
   int size;
   RSS_ITEM *head;
   RSS_ITEM *tail;

};
typedef struct RSS_LIST RSS_LIST;

RSS_LIST* rss_list_init();
void      rss_list_free(RSS_LIST *list);
void      rss_list_add(RSS_LIST *list, const char *url, struct tm pubdate);


/*
 *   podcast.c
 *
 * (linked list) collection of previously fetched podcasts (RSS_LIST's) 
 */
struct RSSCACHE_ITEM 
{
   char *xmlurl;
   RSS_LIST *rsslist;

   struct RSSCACHE_ITEM *next;

};
typedef struct RSSCACHE_ITEM RSSCACHE_ITEM;

struct RSSCACHE_LIST
{
   int size;
   RSSCACHE_ITEM *head;

};
typedef struct RSSCACHE_LIST RSSCACHE_LIST;

RSSCACHE_LIST* rsscache_list_init();
void           rsscache_list_free(RSSCACHE_LIST *list);
void           rsscache_list_add(RSSCACHE_LIST *list, const char *xmlurl, RSS_LIST *rsslist);


#endif

