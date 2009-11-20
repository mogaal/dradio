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
#include "dradio.h"
#include "lists.h"

extern MPLAYER *mplayer;

/* ncurses stuff */


/** create main window */

void main_win_create(MAIN_WIN *w)
{
   int x, y, menu_height, win_height;

   resize();

   menu_height = LINES - PAD - w->logoheight - PAD - PAD;                         /* total height minus 3*PADing and logo */
   menu_height = menu_height < 3 ? 3 : menu_height;                               /* ... but no less than 3 */
   menu_height = menu_height > w->menu->nitems ? w->menu->nitems : menu_height;   /* ... and no more than nitems */

   win_height = PAD + w->logoheight + PAD + menu_height + PAD;

   if ((x = (COLS  - WIN_WIDTH)/2) < 0)
      x = 0;
   if ((y = (LINES - win_height)/2) < 0)
      y = 0;

   touchwin(stdscr);
   wnoutrefresh(stdscr);

   w->win = newwin(win_height, WIN_WIDTH, y, x);
   box(w->win, 0, 0);

   nodelay(w->win, TRUE); /* wgetch non blocking */

   if (w->logoheight < 0)
   {
      mvwprintw(w->win, 0, 2, " DRadio ");   /* small label on border instead of logo */
   }

   mvwprintw(w->win, win_height - 1, WIN_WIDTH - 11, " h: help ");

   wnoutrefresh(w->win);

   if (w->logoheight > 0)
      logo_win_create(w, 2, 3);

   menu_win_create(w, menu_height, w->logoheight + 4);

   stat_win_create(w, w->logoheight + 4, w->menu->namelen);
   if (w->cur_item)
   {
      stat_win_update(w->stat_win, w->cur_item_name);
      set_current_item(w->menu, w->cur_item);
      cur_item_name_update(w);
   }

   w->visible = 1;
}


/** resize main window */

void main_win_resize(MAIN_WIN *w)
{
   main_win_delete(w);
   main_win_create(w);
   cur_item_name_update(w);
}


/** delete main window */

void main_win_delete(MAIN_WIN *w)
{
   unpost_menu(w->menu);
   delwin(w->menu_win);
   if (w->logo_win)
      delwin(w->logo_win);
   delwin(w->stat_win);
   delwin(w->win);

   w->visible = 0;
   w->win = NULL;
   w->logo_win = NULL;
   w->menu_win = NULL;
   w->stat_win = NULL;
}


/** create stat window */

void stat_win_create(MAIN_WIN *w, int starty, size_t namelen)
{
   int startx;
   double column_pad;

   size_t width = namelen + 6;   /* width is namelen + ">>  <<" length (6) */

   column_pad = (WIN_WIDTH - 2*namelen)/3.0;

   /* win is 2 colums (menu and stat) and 3 equal whitespace pads */
   startx = 2*column_pad + namelen - 3;

   w->stat_win = derwin(w->win, 5, width, starty, startx);
   stat_win_update(w->stat_win, " ");
}


/** update line 1 in stat window */

void stat_win_update(WINDOW *stat_win, const char *new_msg)
{
   const char *msg = new_msg;
   int startx;

   if (!stat_win)
      return;

   if (mplayer->ispaused)
      msg = "paused";

   startx = center_startx(stat_win, strlen(msg) + 6);

   wmove(stat_win, 0, 0);
   wclrtoeol(stat_win);
   wmove(stat_win, 0, startx);

   /* '>>' */
   waddch(stat_win, ACS_RARROW | A_BOLD);
   waddch(stat_win, ACS_RARROW | A_BOLD);

   wprintw(stat_win, " %s ", msg);

   /* '<<' */
   waddch(stat_win, ACS_LARROW | A_BOLD);
   waddch(stat_win, ACS_LARROW | A_BOLD);
}


/** update line y in stat window */

void stat_win_update_y(WINDOW *stat_win, int starty, const char *format, ...)
{
   int startx;
   char buf[BUFSIZ];
   va_list ap;

   if (!stat_win)
      return;

   wmove(stat_win, starty, 0);
   wclrtoeol(stat_win);
   if (format)
   {
      va_start(ap, format);
      vsnprintf(buf, sizeof(buf), format,  ap);
      va_end(ap);

      startx = center_startx(stat_win, strlen(buf));

      mvwprintw(stat_win, starty, startx, buf);
   }
}


/** create menu window */

void menu_win_create(MAIN_WIN *w, int menu_height, int starty)
{
   int startx;

   set_menu_mark(w->menu, "");

   /* win is 2 colums (menu and stat) and 3 equal whitespace pads */
   startx = (WIN_WIDTH - 2*w->menu->namelen)/3;

   /* create the window to be associated with the menu */
   w->menu_win = derwin(w->win, menu_height, w->menu->namelen, starty, startx);
   keypad(w->menu_win, TRUE);

   /* set main window and format */
   set_menu_win(w->menu, w->menu_win);
   set_menu_format(w->menu, menu_height, 1);

   post_menu(w->menu);
}


/** create menu */

MENU *menu_create()
{
   MENU *menu;
   ITEM **menu_items;
   CONF_ITEM *conf_item;
   CONF_LIST *conf_list;
   int n_choices, i;

   conf_list = create_conf_list();   /* from menu.xml */
   if (conf_list == NULL)
      return NULL;

   n_choices = conf_list->size;

   menu_items = (ITEM **)calloc(n_choices + 1, sizeof(ITEM *));

   conf_item = conf_list->head;
   i = n_choices - 1;
   while (conf_item)
   {
      menu_items[i] = new_item(conf_item->label, NULL);
      menu_items[i]->userptr = conf_item;   /* the xml conf */
      conf_item = conf_item->next;
      i--;
   }
   menu_items[n_choices] = (ITEM*)NULL;

   menu = new_menu((ITEM **)menu_items);
   menu->userptr = conf_list;

   return menu;
}


/** delete menu */

void menu_delete(MENU *menu)
{
   ITEM **item;
   ITEM **items;
   CONF_LIST *conf_list;

   items = menu->items;
   conf_list = (CONF_LIST*)menu->userptr;

   unpost_menu(menu);
   free_menu(menu);
   menu = NULL;

   for (item = items; *item; item++)
      free_item(*item);
   free(items);

   conf_list_free(conf_list);
}


/** find start x for centering string */

int center_startx(WINDOW *win, int strlen)
{
   int y, x;
   getmaxyx(win, y, x);

   if (x < strlen)
      return 0;
   else
      return (x-strlen)/2;
}


/** update cur menu item */

void cur_item_name_update(MAIN_WIN *w)
{
   if (w->cur_item)
   {
      char *buf = w->cur_item_name;
      size_t n = sizeof(w->cur_item_name);

      strncpy(buf, item_name(w->cur_item), n - 1);
      buf[n-1] = '\0';

      stat_win_update(w->stat_win, buf);
      cur_rss_update(w);
   }
}


/** update rss name */

void cur_rss_update(MAIN_WIN *w)
{
   int y = 1;

   if (w->cur_rss_item)
   {
      strftime(w->cur_rss_name, sizeof(w->cur_rss_name), "%d/%m-%Y", &w->cur_rss_item->pubdate);
      stat_win_update_y(w->stat_win, y, "%s", w->cur_rss_name);
      cur_ans_time_position_update(w->stat_win);
   }
   else
   {
      stat_win_update_y(w->stat_win, y, NULL);
   }
}


/** update rss seek position and length */

void cur_ans_time_position_update(WINDOW *stat_win)
{
   int y = 3;

   if (mplayer->ans_time_position[0] != '\0' && mplayer->ans_length[0] != '\0')
   {
      stat_win_update_y(stat_win, y, "%s of %s", mplayer->ans_time_position, mplayer->ans_length);
      stat_win_update_y(stat_win, y+1, NULL);
   }
   else
   {
      stat_win_update_y(stat_win, y, NULL);
   }
}

/** update xterm title with name of current stream/podcast */

void update_xterm_title(MAIN_WIN *w, ITEM *item)
{
   if (w->update_xterm_title)
   {
      if (item)
         printf("%c]0;%s - %s%c", '\033', item_name(item), "DRadio", '\007');
      else
         printf("%c]0;%s%c", '\033', "DRadio", '\007');
   }
}

