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

#ifndef DRADIO_H
#define DRADIO_H

#include <signal.h>
#include <ncursesw/menu.h>

#include "lists.h"

/* win structs */

struct MAIN_WIN
{
   int visible;
   WINDOW *win;
   WINDOW *logo_win;
   WINDOW *menu_win;
   WINDOW *stat_win;
   MENU *menu;
   ITEM *cur_item;
   char cur_item_name[BUFSIZ];
   RSS_ITEM *cur_rss_item;
   char cur_rss_name[BUFSIZ];
   int logoheight;
   int update_xterm_title;
};
typedef struct MAIN_WIN MAIN_WIN;

struct HELP_WIN
{
   int visible;
   int x, y, maxy;
   WINDOW *win;
   WINDOW *pad;
};
typedef struct HELP_WIN HELP_WIN;

/* dradio.c */

int main_win_handle_stdin(MAIN_WIN *w, HELP_WIN *help_win, RSSCACHE_LIST * rsscache);
int mplayer_handle_stdout(MAIN_WIN *w, FILE *mplayer_outlog);
int mplayer_handle_stderr(WINDOW *stat_win, FILE *mplayer_errlog);
void resize();
void seconds_to_string(double time, char *buf, size_t n);
void errmsg(const char *format, ...);
void logmsg(const char *format, ...);
void usage();
void version();

/* gui.c - ncurses GUI */

#define PAD 2
#define WIN_WIDTH 80

MENU *menu_create();
void menu_delete(MENU *menu);

void main_win_create(MAIN_WIN *w);
void main_win_resize(MAIN_WIN *w);
void main_win_delete(MAIN_WIN *w);
void menu_win_create(MAIN_WIN *w, int menu_height, int starty);
void logo_win_create(MAIN_WIN *w, int starty, int startx);
void stat_win_create(MAIN_WIN *w, int starty, size_t namelen);

void stat_win_update(WINDOW *stat_win, const char *msg);
void stat_win_update_y(WINDOW *stat_win, int starty, const char *format, ...);

int center_startx(WINDOW *win, int strlen);

void cur_item_name_update(MAIN_WIN *w);
void cur_rss_update(MAIN_WIN *w);
void cur_ans_time_position_update(WINDOW *stat_win);

int  help_win_handle_stdin(HELP_WIN *w, MAIN_WIN *main_win);
void help_win_create(HELP_WIN *w);
void help_win_delete(HELP_WIN *w);
void help_win_update(HELP_WIN *w, int y);

void logo_win_toggle(MAIN_WIN *w);
void update_xterm_title(MAIN_WIN *w, ITEM *item);


/* signals.c - signal handling */

void signals_sigaction(int signum, void (*handler)(int));
void signals_sigaction_all();
void signals_block_none();
void signals_block_all(sigset_t *orig_mask);
void signals_handler(int signum);


/* mplayer.c - mplayer */

struct MPLAYER
{
   pid_t pid;
   FILE *in;
   FILE *out;
   FILE *err;
   char ans_length[BUFSIZ];
   char ans_time_position[BUFSIZ];
   int ispaused;
};
typedef struct MPLAYER MPLAYER;

char **mplayer_argv(int argc, char **argv);
MPLAYER *mplayer_init(int argc, char **argv);
void mplayer_loadlist(MPLAYER *mplayer, const char *url);
void mplayer_loadfile(MPLAYER *mplayer, const char *url);
void mplayer_volume_down(MPLAYER *mplayer);
void mplayer_volume_up(MPLAYER *mplayer);
void mplayer_seek(MPLAYER *mplayer, int value);
void mplayer_pause(MPLAYER *mplayer);
void mplayer_quit(MPLAYER *mplayer);
void mplayer_get_time_pos(MPLAYER *mplayer);
void mplayer_get_time_length(MPLAYER *mplayer);
void mplayer_ispaused(MPLAYER *mplayer);

/* configuration.c - menu conf */

CONF_LIST *create_conf_list();
int get_config_dradio_inputconf_path(char *path, size_t n);
int get_config_dradio_outlog_path(char *path, size_t n);
int get_config_dradio_errlog_path(char *path, size_t n);

int create_config_dradio_dir();
FILE * open_outlog();
FILE * open_errlog();

/* podcast.c */

RSS_LIST* rss_list_lookup(RSSCACHE_LIST *list, const char *xmlurl);



#endif

