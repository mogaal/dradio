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

#include <expat.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <errno.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include "lists.h"
#include "dradio.h"


#define XML_ELEM_ITEM  "item"
#define XML_ELEM_PUBDATE  "pubDate"
#define XML_ELEM_ENCLOSURE  "enclosure"
#define XML_ATTR_URL  "url"


/** libcurl download */

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
   return fwrite(ptr, size, nmemb, stream);
}


/** download file, return 0->OK, -1->ERROR */

static int download_file(FILE *datafile, const char *url)
{
   CURL *curl;
   CURLcode res;
   long http_status_code;

   static char errorbuf[CURL_ERROR_SIZE];


   curl = curl_easy_init();

   if (curl == NULL)
   {
      errmsg("Failed to create CURL handle");
      return -1;
   }

   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorbuf);

   curl_easy_setopt(curl, CURLOPT_URL, url);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, datafile);

   res = curl_easy_perform(curl);

   if (res != CURLE_OK)
   {
      errmsg("Failed to get URL '%s' [%s]\n", url, curl_easy_strerror(res));
      return -1;
   }

   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);
   if (http_status_code/100 != 2)
   {
      errmsg("Failed to get URL '%s' [HTTP status code %ld]\n", url, http_status_code);
      return -1;
   }

   /* always cleanup */
   curl_easy_cleanup(curl);

   return 0;
}


/** expat userdata */

struct RSSPLAYLIST_DATA
{
   char *url;
   int accumulate_pubdate;
   int in_item_element;
   char *pubdate;

   RSS_LIST *list;

};
typedef struct RSSPLAYLIST_DATA RSSPLAYLIST_DATA;


/** expat callbacks */

static void XMLCALL start_element_handler(void *userdata, const char *el, const char **attr)
{
   int i;
   RSSPLAYLIST_DATA *data = (RSSPLAYLIST_DATA *)userdata;

   /* item */
   if (strcmp(el, XML_ELEM_ITEM) == 0)
      data->in_item_element = 1;

   if (data->in_item_element)
   {
      /* pubDate */
      if (strcmp(el, XML_ELEM_PUBDATE) == 0)
         data->accumulate_pubdate = 1;

      /* enclosure */
      if (strcmp(el, XML_ELEM_ENCLOSURE) == 0)
      {
         for (i = 0; attr[i]; i += 2)
         {
            if (strcmp(attr[i], XML_ATTR_URL) == 0)
               data->url = strdup((char*)attr[i + 1]);
         }
      }
   }
}


/** end element handler */

static void XMLCALL end_element_handler(void *userdata, const char *el)
{
   static const char *RFC822x = "%d %b %Y %H:%M:%S %z";
   static const char *RFC822 = "%a, %d %b %Y %H:%M:%S %z";

   RSSPLAYLIST_DATA *data = (RSSPLAYLIST_DATA *)userdata;

   if (data->in_item_element)
   {
      /* pubDate */
      if (strcmp(el, XML_ELEM_PUBDATE) == 0)
         data->accumulate_pubdate = 0;
   }

   /* item */
   if (strcmp(el, XML_ELEM_ITEM) == 0)
   {
      char *res = NULL;
      struct tm tm;
      bzero(&tm, sizeof(tm));

      if (data->pubdate)
      {
         res = strptime(data->pubdate, RFC822x, &tm);
         if (!res)
            res = strptime(data->pubdate, RFC822, &tm);

         if (data->url)
            rss_list_add(data->list, data->url, tm);
      }

      free(data->url);
      data->url = NULL;

      free(data->pubdate);
      data->pubdate = NULL;

      data->in_item_element = 0;
   }
}


/** character data handler  */

static void XMLCALL character_data_handler(void *userdata, const XML_Char *content, int len)
{
   char *s;
   int oldlen, newlen;
   RSSPLAYLIST_DATA *data = (RSSPLAYLIST_DATA *)userdata;

   if (data->accumulate_pubdate)
   {
      s = data->pubdate;   /* make shorter alias */

      oldlen = s ? strlen(s) : 0;
      newlen = len + oldlen;

      s = realloc(s, newlen + 1);

      strncpy(s + oldlen , content, len);
      s[newlen] = '\0';

      data->pubdate = s;
   }
}


/** expat parse menu.xml. return RSS_LIST or NULL on error  */

static RSS_LIST *rss_list_parse(FILE *datafile)
{
   char buff[BUFSIZ];
   RSS_LIST *list;

   XML_Parser p;

   RSSPLAYLIST_DATA data;
   bzero(&data, sizeof(data));

   p = XML_ParserCreate(NULL);
   if (!p) {
      errmsg("Couldn't allocate memory for parser\n");
      return NULL;
   }

   list = rss_list_init();
   data.list = list;

   XML_SetUserData(p, &data);

   XML_SetElementHandler(p, start_element_handler, end_element_handler);
   XML_SetCharacterDataHandler(p, character_data_handler);

   for (;;) {
      int done;
      int len;

      len = (int)fread(buff, sizeof(char), sizeof(buff), datafile);
      if (ferror(datafile)) {
         errmsg("fread: %s", strerror(errno));
         return NULL;
      }
      done = feof(datafile);

      if (XML_Parse(p, buff, len, done) == XML_STATUS_ERROR) {
         errmsg("Parse error at line %lu:\n%s\n",
                XML_GetCurrentLineNumber(p),
                XML_ErrorString(XML_GetErrorCode(p)));
         return NULL;
      }

      if (done)
         break;
   }

   XML_ParserFree(p);

   return list;
}


/** return RSS_LIST or NULL on error */

static RSS_LIST* rss_list_create(const char *xmlurl)
{
   FILE *datafile;
   RSS_LIST *list = NULL;

   datafile = tmpfile();
   if (datafile == NULL)
   {
      errmsg("tmpfile: %s", strerror(errno));
      return NULL;
   }

   if (download_file(datafile, xmlurl) != -1)
   {
      rewind(datafile);
      list  = rss_list_parse(datafile);
   }

   fclose(datafile);
   return list;
}


/**
 * Get a podcast list. If not yet cached, download and parse the
 * RSS 2.0 file the 'xmlurl' points to.
 */

RSS_LIST* rss_list_lookup(RSSCACHE_LIST *list, const char *xmlurl)
{
   RSSCACHE_ITEM *item;
   RSS_LIST *rsslist;

   item = list->head;

   /* just return if already in cache */
   while (item)
   {
      if (strcmp(xmlurl, item->xmlurl) == 0)
         return item->rsslist;

      item = item->next;
   }

   /* not in cache, add it */
   rsslist = rss_list_create(xmlurl);
   rsscache_list_add(list, xmlurl, rsslist);

   return rsslist;
}

