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

#define CTRL(c) ((c) & 0x1f)


/** help window eventloop */

int help_win_handle_stdin(HELP_WIN *w, MAIN_WIN *main_win)
{
   int c;
   int y1 = w->y;
   int y2 = w->maxy;

   int page_height = LINES - 2;
   int max_scroll = y2 - page_height;

   /* under the ncurses implementation, handled signals never interrupt wgetch */
   while ((c = getch()) != ERR)
   {
      switch(c)
      {
       case 'j':          /* menu one up */
       case KEY_DOWN:
       case CTRL('n'):
         if (y1 < max_scroll)
         ++y1;
         else
            beep();
         break;
       case 'k':          /* menu one up */
       case KEY_UP:
       case CTRL('p'):
         if (y1 > 0)
         --y1;
         else
            beep();
         break;
       case CTRL('f'):    /* menu page down */
       case KEY_NEXT:
       case KEY_NPAGE:
         if (y1 < max_scroll)
         {
            y1 += page_height;
            if (y1 >= max_scroll)
               y1 = max_scroll;
         }
         else
            beep();
         break;
       case CTRL('b'):    /* menu page up */
       case KEY_PPAGE:
       case KEY_PREVIOUS:
         if (y1 > 0)
         {
            y1 -= page_height;
            if (y1 < 0)
               y1 = 0;
         } 
         else
            beep();
         break;
       case KEY_HOME:     /* menu first */
         y1 = 0;
         break;
       case KEY_END:      /* menu last */
         y1 = max_scroll;
         break;
       case CTRL('l'):    /* refresh screen */
       case KEY_RESIZE:   /* window resize */
         {
            int t;
            help_win_delete(w);
            help_win_create(w);
            /* if page_height is increased show more text */
            page_height = LINES - 2;
            t = page_height - (y2 - y1);
            if (t > 0)
               y1 -= t; 
            if (y1 < 0)
               y1 = 0;
         }
         break;
       case 'q':          /* 'q' quit help */
         help_win_delete(w);
         main_win_create(main_win);
         return 0;
         break;
       case ERR:
         break;
       default:
         beep();
         break;
      }

      help_win_update(w, y1);
   }

   return 0;
}


/** create help window */

void help_win_create(HELP_WIN *help_win)
{
   WINDOW *p;   /* pad alias */
   int x, y = 0;

   resize();

   if ((x = (COLS  - WIN_WIDTH)/2) < 0)
      x = 0;

   touchwin(stdscr);
   wnoutrefresh(stdscr);

   help_win->win = newwin(LINES, WIN_WIDTH, y, x);

   nodelay(help_win->win, TRUE); /* wgetch non blocking */

   /* create pad - 1000 lines should be enough */
   help_win->pad = newpad(1000, WIN_WIDTH - 4);
   p = help_win->pad;

   scrollok(p, TRUE); 

   waddstr(p, "\nKEYBOARD CONTROL\n");

   /* scroll line */
   waddstr(p, "\n   k/j or up/down\n");
   waddstr(p, "         Navigate menu 1 item up/down.\n");

   /* scroll page */
   waddstr(p, "\n   ctrl-b/ctrl-f or pgup/pgdown\n");
   waddstr(p, "         Navigate menu 1 page up/down.\n");

   /* volume */
   waddstr(p, "\n   / and *\n");
   waddstr(p, "         Decrease/increase volume.\n");

   /* navigate podcasts */
   waddstr(p, "\n   < and >\n");
   waddstr(p, "         Previous/next podcast.\n");

   /* seek 1 min */
   waddstr(p, "\n   left and right\n");
   waddstr(p, "         Seek backward/forwards 1 minute in podcasts.\n");

   /* seek 10 min */
   waddstr(p, "\n   shift-left and shift-right\n");
   waddstr(p, "         Seek backward/forward 10 minutes in podcasts.\n");

   /* toggle logo */
   waddstr(p, "\n   t\n");
   waddstr(p, "         Toggle show logo.\n");


   /* pause */
   waddstr(p, "\n   p\n");
   waddstr(p, "         Toggle pause.\n");

   /* q */
   waddstr(p, "\n   q\n");
   waddstr(p, "         Quit.\n");

   waddstr(p, "\n   When playing streaming TV or video podcasts, see mplayer(1) for keyboard controls.\n");


   waddstr(p, "\nSEE ALSO\n");

   waddstr(p, "\n   man pages: dradio(1), dradio(5), dradio-config(1), mplayer(1).\n");

   help_win->x = x;
   help_win->y = 0;
   help_win->maxy = getcury(p);

   help_win->visible = 1;
}


/** delete help window */

void help_win_delete(HELP_WIN *w)
{
   delwin(w->pad);
   delwin(w->win);

   w->pad = NULL;
   w->win = NULL;
}


/** update help window */

void help_win_update(HELP_WIN *w, int y)
{
   werase(w->win);

   box(w->win, 0, 0);
   mvwprintw(w->win, 0, 2, " DRadio - Help ");   /* label on border */
   mvwprintw(w->win, LINES - 1, WIN_WIDTH -16, " q: quit help ");

   wnoutrefresh(w->win);
   w->y = y;
   pnoutrefresh(w->pad, w->y, 0, 1, w->x + 2, LINES - 2, w->x + WIN_WIDTH - 4);
   doupdate();
}

