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
#include <iconv.h>
#include <locale.h>
#include <langinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "lists.h"
#include "dradio.h"

static int create_config_dir(const char *HOME);
static int get_config_dradio_menu_path(char *path, size_t n);
static int conf_write_file(const char *path);
static int write_config_inputconf();
static int conf_write_inputconf_file(const char *path);
static int file_exists(const char *path);
static int dir_exists(const char *path);
static int dir_create(const char* path);

#define XML_ELEM_ITEM  "item"
#define XML_ATTR_LABEL "label"
#define XML_ATTR_SRC  "src"
#define XML_ATTR_PLAYLIST  "playlist"
#define XML_ATTR_TYPE  "type"   /* direct, playlist, rss */

#define MAX_LABEL_LENGTH 30   /*  WIN_WIDTH/2 - 2*PAD - 2*ACS_ARROW - 2*whitespace */


static char *menuxml[] = {
   "<?xml version='1.0' encoding='ISO-8859-1'?>\n",
   "\n",
   "<!-- dradio menu list. See 'man 5 dradio' for more information on the format of this file.\n",
   " and 'man dradio-config' for help on keeping this list up-to-date. -->\n",
   "\n",
   "<menu>\n",
   "  <!-- Example of audio stream -->\n",
   "  <item label='P1' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p1_128.asx' type='playlist'/>\n",
   "  <item label='P2' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p2_128.asx' type='playlist'/>\n",
   "  <item label='P3' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p3_128.asx' type='playlist'/>\n",
   "  <item label='P4 Bornholm' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p4_bornholm_128.asx' type='playlist'/>\n",
   "  <item label='P4 Danmark' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p4_danmark_128.asx' type='playlist'/>\n",
   "  <item label='P4 Fyn' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p4_fyn_128.asx' type='playlist'/>\n",
   "  <item label='P4 København' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p4_koebenhavn_128.asx' type='playlist'/>\n",
   "  <item label='P4 Midt &amp; Vest' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p4_midtvest_128.asx' type='playlist'/>\n",
   "  <item label='P4 Nordjylland' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p4_nordjylland_128.asx' type='playlist'/>\n",
   "  <item label='P4 Sjælland' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p4_Sjaelland_128.asx' type='playlist'/>\n",
   "  <item label='P4 Syd' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p4_syd_128.asx' type='playlist'/>\n",
   "  <item label='P4 Trekanten' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p4_trekanten_128.asx' type='playlist'/>\n",
   "  <item label='P4 Østjyllands Radio' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p4_oestjylland_128.asx' type='playlist'/>\n",
   "  <item label='DR Allegro' src='http://www.dr.dk/netradio/Metafiler/asx/dr_allegro_128.asx' type='playlist'/>\n",
   "  <item label='DR Barometer' src='http://www.dr.dk/netradio/Metafiler/asx/dr_barometer_128.asx' type='playlist'/>\n",
   "  <item label='DR Boogieradio' src='http://www.dr.dk/netradio/Metafiler/asx/dr_boogie_128.asx' type='playlist'/>\n",
   "  <item label='DR Country' src='http://www.dr.dk/netradio/Metafiler/asx/dr_country_128.asx' type='playlist'/>\n",
   "  <item label='DR Dansktop' src='http://www.dr.dk/netradio/Metafiler/asx/dr_dansktop_128.asx' type='playlist'/>\n",
   "  <item label='DR Electronica' src='http://www.dr.dk/netradio/Metafiler/asx/dr_electronica_128.asx' type='playlist'/>\n",
   "  <item label='DR Evergreen' src='http://www.dr.dk/netradio/Metafiler/asx/dr_evergreen_128.asx' type='playlist'/>\n",
   "  <item label='DR Folk' src='http://www.dr.dk/netradio/Metafiler/asx/dr_folk_128.asx' type='playlist'/>\n",
   "  <item label='DR Hiphop' src='http://www.dr.dk/netradio/Metafiler/asx/dr_hiphop_128.asx' type='playlist'/>\n",
   "  <item label='DR Hit' src='http://www.dr.dk/netradio/Metafiler/asx/dr_hit_128.asx' type='playlist'/>\n",
   "  <item label='DR Jazz' src='http://www.dr.dk/netradio/Metafiler/asx/dr_jazz_128.asx' type='playlist'/>\n",
   "  <item label='DR Klassisk' src='http://www.dr.dk/netradio/Metafiler/asx/dr_klassisk_128.asx' type='playlist'/>\n",
   "  <item label='DR MGP' src='http://www.dr.dk/netradio/Metafiler/asx/dr_mgp_128.asx' type='playlist'/>\n",
   "  <item label='DR Modern Rock' src='http://www.dr.dk/netradio/Metafiler/asx/dr_modern_rock_128.asx' type='playlist'/>\n",
   "  <item label='DR Oline' src='http://www.dr.dk/netradio/Metafiler/asx/dr_oline_128.asx' type='playlist'/>\n",
   "  <item label='DR P5000' src='http://www.dr.dk/netradio/Metafiler/asx/dr_p5000_128.asx' type='playlist'/>\n",
   "  <item label='DR Pop DK' src='http://www.dr.dk/netradio/Metafiler/asx/dr_pop_dk_128.asx' type='playlist'/>\n",
   "  <item label='DR R&amp;B' src='http://www.dr.dk/netradio/Metafiler/asx/dr_rogb_128.asx' type='playlist'/>\n",
   "  <item label='DR Rock' src='http://www.dr.dk/netradio/Metafiler/asx/dr_rock_128.asx' type='playlist'/>\n",
   "  <item label='DR Soft' src='http://www.dr.dk/netradio/Metafiler/asx/dr_soft_128.asx' type='playlist'/>\n",
   "  <item label='DR Spillemand' src='http://www.dr.dk/netradio/Metafiler/asx/dr_spillemand_128.asx' type='playlist'/>\n",
   "  <item label='DR World' src='http://www.dr.dk/netradio/Metafiler/asx/dr_world_128.asx' type='playlist'/>\n",
   "\n",
   "  <!-- Example of video stream -->\n",
   "  <item label='DR1 TV' src='mms://video.dr.dk/dr1' type='direct' />\n",
   "  <item label='DR2 TV' src='mms://video.dr.dk/dr2' type='direct' />\n",
   "  <item label='DR Update' src='mms://video.dr.dk/DRUpdate' type='direct' />\n",
   "\n",
   "  <!-- Example of audio podcast -->\n",
   "  <item label='Harddisken' src='http://podcast.dr.dk/p1/rssfeed/harddisken.xml' type='rss'/>\n",
   "\n",
   "  <!-- Example of video podcast -->\n",
   "  <item label='So ein Ding' src='http://vpodcast.dr.dk/feeds/soeindingrss.xml' type='rss'/>\n",
   "</menu>\n",
   ""
};

static char *inputconf[] = {
   "## For the TV popup window stop mplayer instead of quiting\n",
   "q stop\n",
   "ESC stop\n",
   "CLOSE_WIN stop\n",
   ""
};

struct expat_userdata 
{
   CONF_LIST *list;
   XML_Parser parser;
};
typedef struct expat_userdata expat_userdata;

static iconv_t iconv_cd;


/** handle encoding */

int convert(char *src, char *dest, size_t n)
{
   char *codeset = nl_langinfo(CODESET);

   if (strcmp("ISO-8859-1", codeset) == 0 || strcmp("ISO-8859-15", codeset) == 0)
   {
      size_t src_bytesleft;
      src_bytesleft = strlen(src);

      if (iconv(iconv_cd, &src, &src_bytesleft, &dest, &n) == (size_t) -1)
      {
         perror("iconv");
         return -1;
      }
      *dest = '\0';
   } 
   else if (strcmp("UTF-8", codeset) == 0)
   {
      strncpy(dest, src, n-1);
      dest[n-1] = '\0';
   }
   else 
   {
      if (strcmp(codeset, "ANSI_X3.4-1968") == 0)
         fprintf(stderr, "US-ASCII (%s): locale character encoding not supported.\n", codeset);
      else
         fprintf(stderr, "%s: locale character encoding not supported.\n", codeset);
      fprintf(stderr, "Supported character encodings are: UTF-8, ISO-8859-1 and ISO-8859-15.\n");

      return -1;
   }

   return 0;
}


/** expat callbacks */

static void XMLCALL start_element_handler(void *data, const char *el, const char **attr)
{
   int i;
   enum SRC_TYPE srctype = playlist;
   char label[BUFSIZ], src[BUFSIZ];
   expat_userdata *userdata = (expat_userdata*) data;

   if (strcmp(el, XML_ELEM_ITEM) == 0)
   {
      label[0] = '\0';
      src[0] = '\0';

      for (i = 0; attr[i]; i += 2)
      {
         if (strcmp(attr[i], XML_ATTR_LABEL) == 0)
         {
            if (convert((char*)attr[i + 1], label, sizeof(label)) == -1)
            {
               XML_StopParser(userdata->parser, XML_FALSE);
               return;
            }
            continue;
         }
         if (strcmp(attr[i], XML_ATTR_SRC) == 0)
         {
            if (convert((char*)attr[i + 1], src, sizeof(src)) == -1)
            {
               XML_StopParser(userdata->parser, XML_FALSE);
               return;
            }
            continue;
         }
         if (strcmp(attr[i], XML_ATTR_PLAYLIST) == 0)
         {
            if (strcasecmp((char*)attr[i + 1], "true") == 0 || strcmp((char*)attr[i + 1], "1") == 0)
               srctype = playlist;
            else
               srctype = direct;
            continue;
         }
         if (strcmp(attr[i], XML_ATTR_TYPE) == 0)
         {
            if (strcasecmp((char*)attr[i + 1], "playlist") == 0)
               srctype = playlist;
            else if (strcasecmp((char*)attr[i + 1], "rss") == 0)
               srctype = rss;
            else
               srctype = direct;
            continue;
         }
      }
      if (label[0] && src[0])
      {
         label[MAX_LABEL_LENGTH] = 0;
         conf_list_add((CONF_LIST*) userdata->list, label, src, srctype);
      }
   }
}


/** end element handler  */

static void XMLCALL end_element_handler(void *data, const char *el)
{
   (void)data;
   (void)el;
}


/** expat parse menu.xml  */

CONF_LIST *create_conf_list()
{
   char buff[BUFSIZ], path[BUFSIZ];
   FILE *fp = NULL;
   CONF_LIST *list;
   XML_Parser p;
   expat_userdata userdata;
   int xml_error = XML_ERROR_NONE;

   if (get_config_dradio_menu_path(path, sizeof(path)) == -1)
      return NULL;

   if (write_config_inputconf() == -1)
      return NULL;

   if ((iconv_cd = iconv_open("ISO-8859-1", "UTF-8")) == (iconv_t) -1)
   {
      perror("iconv_open");
      return NULL;
   }

   p = XML_ParserCreate(NULL);
   if (!p) {
      fprintf(stderr, "Couldn't allocate memory for parser\n");
      return NULL;
   }

   list = conf_list_init();

   userdata.list = list;
   userdata.parser = p;
   XML_SetUserData(p, &userdata);

   XML_SetElementHandler(p, start_element_handler, end_element_handler);

   fp = fopen(path, "r");
   if (fp == NULL)
   {
      perror("fopen");
      return NULL;
   }

   for (;;) {
      int done;
      int len;

      len = (int)fread(buff, sizeof(char), sizeof(buff), fp);
      if (ferror(fp)) {
         perror("fread");
         return NULL;
      }
      done = feof(fp);

      if (XML_Parse(p, buff, len, done) == XML_STATUS_ERROR) {

         xml_error = XML_GetErrorCode(p);

         switch (xml_error)
         {
          case XML_ERROR_ABORTED:
            ; /* This is a XML_StopParser event, error message has already written */
            break;
          default:
            fprintf(stderr, "Parse error in '%s' at line %lu column %lu: %s\n",
                    path,
                    XML_GetCurrentLineNumber(p),
                    XML_GetCurrentColumnNumber(p),
                    XML_ErrorString(xml_error));
            break;
         }
         break;
      }

      if (done)
         break;
   }

   XML_ParserFree(p);
   fclose(fp);

   iconv_close(iconv_cd);

   if (xml_error != XML_ERROR_NONE)
   {
      conf_list_free(list);
      return NULL;
   }

   return list;
}


/**
 * Get '~/.config/dradio/menu.xml' path
 * 
 * If menu.xml does not exist, create it. 
 */
static int get_config_dradio_menu_path(char *path, size_t n)
{
   char *HOME = getenv("HOME");
   if (!HOME)
   {
      fprintf(stderr, "$HOME not found\n");
      return -1;
   }

   snprintf(path, n, "%s/.config/dradio/menu.xml", HOME);

   if (!file_exists(path))
      if (conf_write_file(path) == -1)
         return -1;

   return 0;
}


/** write configuration file  */

static int conf_write_file(const char *path)
{ 
   int i;

   FILE *file = fopen(path, "w");
   if (!file)
   {
      perror("fopen");
      return -1;
   }

   for (i = 0; *menuxml[i]; ++i) 
      fwrite(menuxml[i], sizeof(char), strlen(menuxml[i]), file);

   fclose(file);

   return 0;
}


/** */
int get_config_dradio_inputconf_path(char *path, size_t n)
{
   char *HOME = getenv("HOME");
   if (!HOME)
   {
      fprintf(stderr, "$HOME not found\n");
      return -1;
   }

   snprintf(path, n, "%s/.config/dradio/input.conf", HOME);

   return 0;
}


/** write input.conf if it does not exist.  */

static int write_config_inputconf()
{
   char path[BUFSIZ];

   char *HOME = getenv("HOME");
   if (!HOME)
   {
      fprintf(stderr, "$HOME not found\n");
      return -1;
   }

   if (get_config_dradio_inputconf_path(path, sizeof(path)) == -1)
      return -1;

   if (!file_exists(path))
      if (conf_write_inputconf_file(path) == -1)
         return -1;

   return 0;
}


/** */

static int conf_write_inputconf_file(const char *path)
{ 
   int i;

   FILE *file = fopen(path, "w");
   if (!file)
   {
      perror("fopen");
      return -1;
   }

   for (i = 0; *inputconf[i]; ++i) 
      fwrite(inputconf[i], sizeof(char), strlen(inputconf[i]), file);

   fclose(file);

   return 0;
}


/** */

int get_config_dradio_outlog_path(char *path, size_t n)
{
   char *HOME = getenv("HOME");
   if (!HOME)
   {
      fprintf(stderr, "$HOME not found\n");
      return -1;
   }

   snprintf(path, n, "%s/.config/dradio/out.log", HOME);

   return 0;
}


/** */

int get_config_dradio_errlog_path(char *path, size_t n)
{
   char *HOME = getenv("HOME");
   if (!HOME)
   {
      fprintf(stderr, "$HOME not found\n");
      return -1;
   }

   snprintf(path, n, "%s/.config/dradio/err.log", HOME);

   return 0;
}


/** */

static int create_config_dir(const char *HOME)
{
   char path[BUFSIZ];

   snprintf(path, sizeof(path), "%s/.config", HOME);

   if (dir_create(path) == -1)
      return -1;

   return 0;
}


/** */

int create_config_dradio_dir()
{
   char *HOME;
   char path[BUFSIZ];

   HOME = getenv("HOME");
   if (!HOME)
   {
      fprintf(stderr, "$HOME not found\n");
      return -1;
   }

   if (create_config_dir(HOME) == -1)
      return -1;

   snprintf(path, sizeof(path), "%s/.config/dradio", HOME);

   if (dir_create(path) == -1)
      return -1;

   return 0;
}


/** */

static int dir_exists(const char *path)
{
   struct stat statbuf;
   return (stat(path, &statbuf) != -1 && S_ISDIR(statbuf.st_mode));
}


/** */

static int file_exists(const char *path)
{
   struct stat statbuf;
   return (stat(path, &statbuf) != -1 && S_ISREG(statbuf.st_mode));
}


/** */

static int dir_create(const char* path)
{
   if (!dir_exists(path))
      if (mkdir(path, S_IRWXU) == -1) 
      {
         perror("mkdir");
         return -1;
      }

   return 0;
}


/** stdout > out.log */

FILE * open_outlog()
{
   FILE *stream;
   char out_log[BUFSIZ];
   if (get_config_dradio_outlog_path(out_log, sizeof(out_log)) == -1)
      return NULL;

   stream = fopen(out_log, "a+");

   return stream;
}


/** stderr > err.log */

FILE * open_errlog()
{
   FILE *stream;
   char err_log[BUFSIZ];
   get_config_dradio_errlog_path(err_log, sizeof(err_log));

   stream = fopen(err_log, "a+");

   return stream;
}

