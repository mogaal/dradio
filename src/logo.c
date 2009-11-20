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

#include "dradio.h"


static char *logo[] = {
   "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD       DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD  ",
   " DDDDDDDDDDDDD     DDDDDDDDDDDDDD      DDDDDDDDDDDDD       DDDDDDDDDDDDDDD",
   " DDDDDDDDDDDD         DDDDDDDDDDDD      DDDDDDDDDDD          DDDDDDDDDDDDD",
   "  DDDDDDDDDDD          DDDDDDDDDDDD     DDDDDDDDDDD           DDDDDDDDDDDD",
   "  DDDDDDDDDDD           DDDDDDDDDDD     DDDDDDDDDDD           DDDDDDDDDDDD",
   "  DDDDDDDDDDD           DDDDDDDDDDD     DDDDDDDDDDD           DDDDDDDDDD  ",
   "  DDDDDDDDDDD           DDDDDDDDDDD     DDDDDDDDDDD          DDDDDDDDD    ",
   "  DDDDDDDDDDD           DDDDDDDDDDD     DDDDDDDDDDD    DDDDDDDDDDDDD      ",
   "  DDDDDDDDDDD          DDDDDDDDDDDD     DDDDDDDDDDD     DDDDDDDDDDDDD     ",
   "  DDDDDDDDDDD         DDDDDDDDDDDDD     DDDDDDDDDDD      DDDDDDDDDDDDD    ",
   " DDDDDDDDDDDD        DDDDDDDDDDDDD      DDDDDDDDDDD       DDDDDDDDDDDDDD  ",
   " DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD       DDDDDDDDDDDD        DDDDDDDDDDDDDD ",
   "DDDDDDDDDDDDDDDDDDDDDDDDDDDD          DDDDDDDDDDDDD          DDDDDDDDDDDDD",
   ""
};


/** create logo window */

void logo_win_create(MAIN_WIN *w, int starty, int startx)
{
   int i = 0;
   char *cp;
   w->logo_win = derwin(w->win, 13, 74, starty, startx);

   while (*logo[i]) 
   {
      int j = 0;
      cp = logo[i];
      while (*cp != '\0') 
      {
         if (*cp != ' ')
            mvwaddch(w->logo_win, i, j++, ACS_DIAMOND);
         else
            mvwaddch(w->logo_win, i, j++, ' ');
         ++cp;
      }
      ++i;
   }

   wnoutrefresh(w->logo_win);   /* flush to internal buffer  */
}


/** toggle show logo */

void logo_win_toggle(MAIN_WIN *w)
{
   if (w->logoheight == 13)
      w->logoheight = -2;   /* negative to compensate for padding to menu */
   else
      w->logoheight = 13;
}

