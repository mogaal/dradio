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

#include <stdio.h>
#include <stdarg.h>

#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>  /* also defines CTRL(c) */

#include "lists.h"
#include "dradio.h"

extern sig_atomic_t got_signal;

MPLAYER *mplayer;


/** main */

int main(int argc, char **argv)
{
   int i, maxfd;

   MAIN_WIN main_win;
   HELP_WIN help_win;

   RSSCACHE_LIST *rsscache;

   FILE *mplayer_outlog, *mplayer_errlog;

   int exit_eventloop = 0;

   sigset_t orig_mask;
   fd_set watchset;
   fd_set inset;
   struct timespec tv, *tv_p = NULL;

   int exit_status = EXIT_SUCCESS;

   memset(&main_win, 0, sizeof(MAIN_WIN));
   memset(&help_win, 0, sizeof(HELP_WIN));

   main_win.update_xterm_title = 1;

   signals_sigaction_all();
   /* mask out signals that are not to be received except within the pselect() call */
   signals_block_all(&orig_mask);

   logo_win_toggle(&main_win);

   /* command line arguments */
   for (i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
         usage();
      if (strcmp(argv[i], "--version") == 0)
         version();
      if (strcmp(argv[i], "--nologo") == 0)
         logo_win_toggle(&main_win);
      if (strcmp(argv[i], "--notitle") == 0)
         main_win.update_xterm_title = 0;
   }

   setlocale(LC_ALL, "");

   if (create_config_dradio_dir() == -1)
      exit(EXIT_FAILURE);
   if ((mplayer_outlog = open_outlog()) == NULL)
      exit(EXIT_FAILURE);
   if ((mplayer_errlog = open_errlog()) == NULL)
      exit(EXIT_FAILURE);

   if ((main_win.menu = menu_create()) == NULL)   /* read conf and create menu items */
      exit(EXIT_FAILURE);

   /* pass all args to mplayer */
   if ((mplayer = mplayer_init(argc, argv)) == NULL)
   {
      perror("mplayer");
      exit(EXIT_FAILURE);
   }

   update_xterm_title(&main_win, NULL);

   rsscache = rsscache_list_init();

   initscr();   /* start curses mode */
   nonl();    /* tell curses not to do NL->CR/NL on output */
   cbreak();   /* disable line buffering */
   noecho();   /* do not echo while getch() */
   keypad(stdscr, TRUE);   /* enable arrow keys, etc. */
   curs_set(0);   /* cursor invisible */
   nodelay(stdscr, TRUE); /* wgetch non blocking */
   wnoutrefresh(stdscr);

   main_win_create(&main_win);


   /* watch stdin (fd 0) for user input */
   FD_ZERO(&watchset);
   FD_SET(STDIN_FILENO, &watchset);

   /* watch mplayer stdout for status messages */
   FD_SET(fileno(mplayer->out), &watchset);
   maxfd = STDIN_FILENO > fileno(mplayer->out) ? STDIN_FILENO + 1 : fileno(mplayer->out) + 1;

   /* watch mplayer stdout for status messages */
   FD_SET(fileno(mplayer->err), &watchset);
   maxfd = maxfd > fileno(mplayer->err) ? maxfd : fileno(mplayer->err) + 1;


   /* event loop */

   while (!exit_eventloop)
   {
      int pselect_retval;

      /* use copy - selects mutates the fd_set */
      inset = watchset;

      pselect_retval = pselect(maxfd, &inset, NULL, NULL, tv_p, &orig_mask);

      /* handle pselect error */
      if (pselect_retval == -1 && errno != EINTR)
      {
         errmsg("pselect: %s", strerror(errno));
         exit_status = errno;
         exit_eventloop = 1;
         break;
      }

      /* handle signal */
      if (got_signal)
      {
         if (got_signal == SIGWINCH)
         {
            logmsg("dradio: interrupted by signal %d (%s)", got_signal, strsignal(got_signal));
            got_signal = 0;  /* clear got_signal */
            ungetch(KEY_RESIZE);  /* handle resize in handle_stdin() */
         }
         else
         {
            logmsg("dradio: interrupted by signal %d (%s)", got_signal, strsignal(got_signal));

            if (got_signal == SIGCHLD)
            {
               errmsg("dradio: mplayer not found or it exited unexpectedly, "
                      "please consult ~/.config/dradio/*.log for more information.");
            }

            exit_status = got_signal;
            exit_eventloop = 1;
            break;
         }
      }

      /* handle pselect timeout */
      if (pselect_retval == 0)
      {
         if (main_win.visible)
         {
            if (!mplayer->ispaused && main_win.cur_rss_item != NULL)
            {
               mplayer_get_time_length(mplayer);
               mplayer_get_time_pos(mplayer);
            }

            /* query pause status */
            mplayer_ispaused(mplayer);
         }

         tv_p = NULL;
      }


      /* user input */
      /*if (FD_ISSET(STDIN_FILENO, &inset))*/
      {
         int res;

         if (main_win.visible)
            res = main_win_handle_stdin(&main_win, &help_win, rsscache);
         else
            res = help_win_handle_stdin(&help_win, &main_win);

         if (res != 0)
         {
            if (res < 0)
               exit_status = EXIT_FAILURE;

            exit_eventloop = 1;
            break;
         }
      }

      /* ready mplayer stdout, unless pselect was interrupted */
      if (FD_ISSET(fileno(mplayer->out), &inset) && pselect_retval != -1)
      {
         if (mplayer_handle_stdout(&main_win, mplayer_outlog) < 0)
         {
            /* something is amiss, do not read again */
            FD_CLR(fileno(mplayer->out), &watchset);
         }
      }

      /* ready mplayer stderr, unless pselect was interrupted */
      if (FD_ISSET(fileno(mplayer->err), &inset) && pselect_retval != -1)
      {
         if (mplayer_handle_stderr(main_win.stat_win, mplayer_errlog) < 0)
         {
            /* something is amiss, do not read again */
            FD_CLR(fileno(mplayer->err), &watchset);
         }
      }

      if (main_win.visible)
      {
         if (!mplayer->ispaused && main_win.cur_rss_item != NULL)
         {
            /* set .1 sec new timeout */
            tv.tv_sec = 0;
            tv.tv_nsec = 100000000;
            tv_p = &tv;
         }
         else
         {
            /* set new .5 sec timeout primarily to update pause status */
            tv.tv_sec = 0;
            tv.tv_nsec = 500000000;
            tv_p = &tv;
         }

         wnoutrefresh(main_win.menu_win);
         wnoutrefresh(main_win.stat_win);
      }

      doupdate();
   }

   /* out of event loop - now exit */

   /* free memory */
   main_win_delete(&main_win);
   menu_delete(main_win.menu);

   help_win_delete(&help_win);
   endwin();

   rsscache_list_free(rsscache);

   mplayer_quit(mplayer);

   if (exit_status)
      errmsg(NULL);

   fclose(mplayer_outlog);
   fclose(mplayer_errlog);

   update_xterm_title(&main_win, NULL);

   return exit_status;
}


/** handle user input (stdin) on pselect interrupt. Return -1 -> ERROR, 0 -> OK, 1 -> 'q' */

int main_win_handle_stdin(MAIN_WIN *w, HELP_WIN *help_win, RSSCACHE_LIST * rsscache)
{
   int c;
   CONF_ITEM* cur_conf_item;

   MENU *menu = w->menu;  /* alias */

   w->cur_item_name[0] = '\0';

   /* under the ncurses implementation, handled signals never interrupt getch */
   while ((c = getch()) != ERR)
   {
      switch(c)
      {
       case 'j':
       case KEY_DOWN:     /* menu one down */
       case CTRL('n'):
         menu_driver(menu, REQ_DOWN_ITEM);
         break;
       case 'k':
       case KEY_UP:       /* menu one up */
       case CTRL('p'):
         menu_driver(menu, REQ_UP_ITEM);
         break;
       case CTRL('f'):    /* menu page down */
       case KEY_NEXT:
       case KEY_NPAGE:
         if (menu_driver(menu, REQ_SCR_DPAGE) == E_REQUEST_DENIED)
       menu_driver(menu, REQ_LAST_ITEM);
         break;
       case CTRL('b'):    /* menu page up */
       case KEY_PPAGE:
       case KEY_PREVIOUS:
         if (menu_driver(menu, REQ_SCR_UPAGE) == E_REQUEST_DENIED)
       menu_driver(menu, REQ_FIRST_ITEM);
         break;
       case KEY_HOME:     /* menu first */
         menu_driver(menu, REQ_FIRST_ITEM);
         break;
       case KEY_END:      /* menu last */
         menu_driver(menu, REQ_LAST_ITEM);
         break;
       case KEY_ENTER:    /* menu select */
       case 10:
       case 13:
       case ' ':
         w->cur_rss_item = NULL;
         w->cur_item = current_item(menu);
         cur_conf_item = (CONF_ITEM *)item_userptr(w->cur_item);

         stat_win_update_y(w->stat_win, 4, NULL); /* clear possible errmsg */

         cur_item_name_update(w);

         if (cur_conf_item->srctype == rss)
         {
            RSS_LIST* list;
            stat_win_update_y(w->stat_win, 3, "Getting RSS...");
            wrefresh(w->stat_win);
            list = rss_list_lookup(rsscache, (char *)cur_conf_item->src);
            if (!list)
               return -1;

            w->cur_rss_item = list->tail;
            cur_rss_update(w);
         }

         stat_win_update_y(w->stat_win, 3, "Loading...");
         wrefresh(w->stat_win);

         if (cur_conf_item->srctype == playlist)
            mplayer_loadlist(mplayer, (char *)cur_conf_item->src);
         else if (cur_conf_item->srctype == rss)
            mplayer_loadfile(mplayer, w->cur_rss_item->url);
         else
            mplayer_loadfile(mplayer, (char *)cur_conf_item->src);

         update_xterm_title(w, w->cur_item);
         break;
       case '<':          /* (podcast) previous in rss */
         if (w->cur_rss_item && w->cur_rss_item->prev)
         {
            w->cur_rss_item = w->cur_rss_item->prev;
            cur_item_name_update(w);
            mplayer_loadfile(mplayer, w->cur_rss_item->url);
         }
         break;
       case '>':          /* (podcast) next in rss */
         if (w->cur_rss_item && w->cur_rss_item->next)
         {
            w->cur_rss_item = w->cur_rss_item->next;
            cur_item_name_update(w);
            mplayer_loadfile(mplayer, w->cur_rss_item->url);
         }
         break;
       case KEY_RIGHT:    /* (podcast) seek 1 min forwards */
         if (w->cur_rss_item)
            mplayer_seek(mplayer, 60);
         break;
       case KEY_LEFT:     /* (podcast) seek 1 min backwards */
         if (w->cur_rss_item)
            mplayer_seek(mplayer, -60);
         break;
       case KEY_SRIGHT:   /* (podcast) seek 10 min forwards */
         if (w->cur_rss_item)
            mplayer_seek(mplayer, 600);
         break;
       case KEY_SLEFT:    /* (podcast) seek 10 min backwards */
         if (w->cur_rss_item)
            mplayer_seek(mplayer, -600);
         break;
       case 'p':          /* pause */
         mplayer_pause(mplayer);
         break;
       case '*':          /* volume up */
         cur_item_name_update(w);
         mplayer_volume_up(mplayer);
         break;
       case '/':          /* volume down */
         cur_item_name_update(w);
         mplayer_volume_down(mplayer);
         break;
       case 't':          /* toggle show logo */
         logo_win_toggle(w);
         main_win_resize(w);
         break;
       case CTRL('l'):    /* refresh screen */
       case KEY_RESIZE:   /* window resize */
         resize();
         main_win_resize(w);
         break;
       case 'h':          /* 'h' help screen */
         main_win_delete(w);
         help_win_create(help_win);
         help_win_update(help_win, 0);
         return 0;
         break;
       case 'q':          /* 'q' quit dradio */
         return 1;
         break;
       default:
         beep();
         break;
      }
   }

   return 0;
}


/** handle mplayer stdout on pselect interrupt */

int mplayer_handle_stdout(MAIN_WIN *w, FILE *mplayer_outlog)
{
   int rc;
   char mplayer_outbuf[BUFSIZ];

   WINDOW *stat_win = w->stat_win;

   double ans_length = -1;
   double ans_time_position = -1;

   mplayer_outbuf[0] = 0;


   /* If a read() is interrupted by a signal before it reads any
    * data, it shall return -1 with errno set to [EINTR].
    *
    * If a read() is interrupted by a signal after it has
    * successfully read some data, it shall return the number of
    * bytes read.
    */
   rc = read(fileno(mplayer->out), mplayer_outbuf, sizeof(mplayer_outbuf) - 1);

   if (rc < 0)
   {
      if (errno == EINTR)
      {
         /* interrupted system call (e.g. xterm resized)  - just try again next time around */
      }
      else
      {
         logmsg("read: (%d) %s", errno, strerror(errno));
         return -1;
      }
   }
   else if (rc == 0)
   {
      /* pipe closed */
      return -1;
   }
   else
   {
      double f;
      char *s, ans_paused[4];

      mplayer_outbuf[rc] = '\0';

      s = mplayer_outbuf;
      while ( (s = strtok(s, "\n")) != NULL)
      {
         if (sscanf(s, "ANS_pause=%3s", ans_paused) == 1)
         {
            /* mplayer->ispaused should not be set anywhere else, otherwise
               the flag may get out of sync with mplayer */
            int old_ispaused = mplayer->ispaused;

            if (strcmp(ans_paused, "yes") == 0) 
               mplayer->ispaused = 1;
            else
               mplayer->ispaused = 0;

            if (old_ispaused != mplayer->ispaused)
               cur_item_name_update(w);
         }
         else if (sscanf(s, "ANS_LENGTH=%lf", &ans_length) == 1)
         {
            seconds_to_string(ans_length, mplayer->ans_length, sizeof(mplayer->ans_length));
         } 
         else if (sscanf(s, "ANS_TIME_POSITION=%lf", &ans_time_position) == 1)
         {
            seconds_to_string(ans_time_position, mplayer->ans_time_position, sizeof(mplayer->ans_time_position));
         }
         else if (sscanf(s, "\rCache fill:  %lf%%", &f) == 1)
         {
            stat_win_update_y(stat_win, 3, "Caching... %.2f%%", f);
            stat_win_update_y(stat_win, 4, NULL);
         }
         else if (strcmp(s, "Starting playback...") == 0)
         {
            stat_win_update_y(stat_win, 3, NULL);
            stat_win_update_y(stat_win, 4, NULL);
         }
         else
         {
            fprintf(mplayer_outlog, "%s\n", s);
         }

         if (ans_length > 0 || ans_time_position > 0)
         {
            cur_ans_time_position_update(stat_win);

            ans_length = ans_time_position = -1;
         }

         s = NULL;
      }
   }

   return 0;
}


/** handle mplayer stderr on pselect interrupt */

int mplayer_handle_stderr(WINDOW *stat_win, FILE *mplayer_errlog)
{
   int rc;
   char mplayer_errbuf[BUFSIZ];

   mplayer_errbuf[0] = 0;

   /* If a read() is interrupted by a signal before it reads any
    * data, it shall return -1 with errno set to [EINTR].
    *
    * If a read() is interrupted by a signal after it has
    * successfully read some data, it shall return the number of
    * bytes read.
    */
   rc = read(fileno(mplayer->err), mplayer_errbuf, sizeof(mplayer_errbuf) - 1);

   if (rc < 0)
   {
      if (errno == EINTR)
      {
         /* interrupted system call (e.g. xterm resized)  - just try again next time around */
      }
      else
      {
         logmsg("read: (%d) %s", errno, strerror(errno));
         return -1;
      }
   }
   else if (rc == 0)
   {
      /* pipe closed */
      return -1;
   }
   else
   {
      char *s;

      mplayer_errbuf[rc] = '\0';

      s = mplayer_errbuf;
      while ( (s = strtok(s, "\n")) != NULL)
      {
         if (strstr(s, "Server returned 404"))
         {
            stat_win_update_y(stat_win, 3, "Error!");
            stat_win_update_y(stat_win, 4, "View log for details.");
            wrefresh(stat_win);
         }

         fprintf(mplayer_errlog, "%s\n", s);
         fflush(mplayer_errlog);

         s = NULL;
      }
   }

   return 0;
}


/** resizeterm */

void resize()
{
   struct winsize ws;

   /* get new xterm size */
   if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
      errmsg("TIOCGWINSZ: %s", strerror(errno));

   resizeterm(ws.ws_row, ws.ws_col);
}


/** format seconds */

void seconds_to_string(double time, char *buf, size_t n)
{
   int hours, minutes, seconds, tenthsecond;

   hours = time / 3600;
   minutes = ((int)time % 3600) / 60;
   seconds = ((int)time % 60);
   tenthsecond = ((int)(time * 10)) % 10;

   if (hours > 0)
      snprintf(buf, n, "%d:%02d:%02d.%d", hours, minutes, seconds, tenthsecond);
   else if (minutes > 0)
      snprintf(buf, n, "%02d:%02d.%d", minutes, seconds, tenthsecond);
   else
      snprintf(buf, n, "%02d.%d", seconds, tenthsecond);
}


/** error message */

void errmsg(const char *format, ...)
{
   va_list ap;
   static char buf[BUFSIZ] = {0};

   if (format == NULL && *buf)
   {
      /* print last errmsg to stderr */
      fprintf(stderr, "%s\n", buf);
   }
   else
   {
      FILE * errlog;

      /* save */
      va_start(ap, format);
      vsnprintf(buf, sizeof(buf), format, ap);
      va_end(ap);

      /* print to errlog */
      errlog = open_errlog();
      fprintf(errlog, "%s\n", buf);
      fclose(errlog);
   }
}


/** log error message */

void logmsg(const char *format, ...)
{
   FILE * errlog;
   va_list ap;
   char buf[BUFSIZ] = {0};

   va_start(ap, format);
   vsnprintf(buf, sizeof(buf), format, ap);
   va_end(ap);

   /* print to errlog */
   errlog = open_errlog();
   fprintf(errlog, "%s\n", buf);
   fflush(errlog);
   fclose(errlog);
}


/** help messages */

void usage()
{
   printf("%s %s is a Danmarks Radio netradio, podcast, and TV player.\n", "dradio", VERSION);
   printf("\n");
   printf("Usage: %s [--nologo] [--notitle] [MPLAYER_OPTIONS]...\n", "dradio");
   printf("       %s --help, -h\n", "dradio");
   printf("       %s --version\n", "dradio");
   printf("\n");

   exit(EXIT_SUCCESS);
}


/** help messages */

void version()
{
   printf("%s %s\n", "dradio", VERSION);
   printf("\n");
   printf("Copyright (C) 2009 Jess Thrysoee.\n");
   printf("This is free software; see the source for copying conditions. There is NO\n");
   printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");

   exit(EXIT_SUCCESS);
}

