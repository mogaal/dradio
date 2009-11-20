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
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "dradio.h"

enum PIPES { READ, WRITE };


/** fork, pipe, execlp an initially idle mplayer process */

MPLAYER *mplayer_init(int argc, char **argv)
{
   pid_t pid;
   MPLAYER *mplayer;

   /* When the parent send data to the child trough a pipe,
      the parent should close READ, and the child should close WRITE */
   int dradio_to_mplayer_stdin[2];
   int mplayer_stdout_to_dradio[2];
   int mplayer_stderr_to_dradio[2];

   if ((mplayer = (MPLAYER*) malloc(sizeof(MPLAYER))) == NULL)
      return NULL; 

   if (pipe(dradio_to_mplayer_stdin) != 0)
      return NULL;

   if (pipe(mplayer_stdout_to_dradio) != 0)
      return NULL;

   if (pipe(mplayer_stderr_to_dradio) != 0)
      return NULL;

   pid = fork();

   if (pid < 0)
   {
      return NULL;
   }
   else if (pid == 0)
   {
      /* child */
      close(dradio_to_mplayer_stdin[WRITE]);
      close(mplayer_stdout_to_dradio[READ]);

      dup2(dradio_to_mplayer_stdin[READ], STDIN_FILENO); /* pipe is now stdin */
      close(dradio_to_mplayer_stdin[READ]);

      dup2(mplayer_stdout_to_dradio[WRITE], STDOUT_FILENO); /* pipe is now stdout */
      close(mplayer_stdout_to_dradio[WRITE]);

      dup2(mplayer_stderr_to_dradio[WRITE], STDERR_FILENO); /* pipe is now stderr */
      close(mplayer_stderr_to_dradio[WRITE]);

      execvp("mplayer", mplayer_argv(argc, argv));
      perror("mplayer");
      _exit(EXIT_FAILURE);
   }

   close(dradio_to_mplayer_stdin[READ]);
   close(mplayer_stdout_to_dradio[WRITE]);
   close(mplayer_stderr_to_dradio[WRITE]);

   if ((mplayer->in = fdopen(dradio_to_mplayer_stdin[WRITE], "w")) == NULL)
      return NULL;

   if ((mplayer->out = fdopen(mplayer_stdout_to_dradio[READ], "r")) == NULL)
      return NULL;

   if ((mplayer->err = fdopen(mplayer_stderr_to_dradio[READ], "r")) == NULL)
      return NULL;

   mplayer->pid = pid;
   mplayer->ispaused = 0;

   return mplayer;
}


/** handle mplayer arguments */

char **mplayer_argv(int argc, char **argv)
{
   int i, resi = 0;
   char **res;
   char path[BUFSIZ], conf[BUFSIZ];

   char *defaults[] = 
   { "mplayer",
      "-slave",
      "-nolirc", /* prevent 'could not connect to socket' log msg */
      "-prefer-ipv4",
      "-idle",
      "-quiet"
         /* "-msglevel", "all=9"*/
   };
   int size = sizeof(defaults)/sizeof(defaults[0]);

   /* alloc mem for argv, defaults, and '-input conf=' */
   res = (char**)calloc((argc - 1) + size + 3, sizeof(char *));

   /* default args */
   for (resi = 0; resi < size; resi++)
      res[resi] = strdup(defaults[resi]);

   /* create 'conf=$HOME/.config/dradio/input.conf' argument */
   get_config_dradio_inputconf_path(path, sizeof(path));
   snprintf(conf, sizeof(conf), "conf=%s", path);

   res[resi++] = strdup("-input");
   res[resi++] = strdup(conf);

   /* commandline args */
   for (i = 1; i < argc; i++, resi++)
   {
      if (strcmp(argv[i], "--nologo") == 0)
         continue;
      if (strcmp(argv[i], "--notitle") == 0)
         continue;

      res[resi] = strdup(argv[i]);
   }

   /* terminate */
   res[resi] = (char*)NULL;

   return res;
}


/** send quit */

void mplayer_quit(MPLAYER *mplayer)
{
   int status;

   fprintf(mplayer->in, "quit\n");
   fflush(mplayer->in);

   /* wait for mplayer to quit */
   if (waitpid(mplayer->pid, &status, 0) < 0)
   {
      errmsg("waitpid: %s", strerror(errno));
   }

   fclose(mplayer->in);
   fclose(mplayer->out);

   free(mplayer);
}


/** send pause */

void mplayer_pause(MPLAYER *mplayer)
{
   fprintf(mplayer->in, "pause\n");
   fflush(mplayer->in);
   mplayer_ispaused(mplayer);
}


/** Is mplayer paused? */

void mplayer_ispaused(MPLAYER *mplayer)
{
   fprintf(mplayer->in, "pausing_keep_force get_property pause\n");
   fflush(mplayer->in);
}


/** send volume up */

void mplayer_volume_up(MPLAYER *mplayer)
{
   fprintf(mplayer->in, "volume +1\n");
   fflush(mplayer->in);
}


/** send volume down */

void mplayer_volume_down(MPLAYER *mplayer)
{
   fprintf(mplayer->in, "volume -1\n");
   fflush(mplayer->in);
}


/**
 * seek <value> [type]
 *     Seek to some place in the movie.
 *         0 is a relative seek of +/- <value> seconds (default).
 *         1 is a seek to <value> % in the movie.
 *         2 is a seek to an absolute position of <value> seconds.
 */

void mplayer_seek(MPLAYER *mplayer, int value)
{
   fprintf(mplayer->in, "seek %d 0\n", value);
   fflush(mplayer->in);
}


/** Print out the current position in the file in seconds, as float */

void mplayer_get_time_pos(MPLAYER *mplayer)
{
   fprintf(mplayer->in, "pausing_keep_force get_time_pos\n");
   fflush(mplayer->in);
}


/** Print out the length of the current file in seconds */

void mplayer_get_time_length(MPLAYER *mplayer)
{
   fprintf(mplayer->in, "pausing_keep_force get_time_length\n");
   fflush(mplayer->in);
}


/** send load playlist */

void mplayer_loadlist(MPLAYER *mplayer, const char *url)
{
   fprintf(mplayer->in, "loadlist %s\n", url);
   fflush(mplayer->in);
}


/** send load file or url */

void mplayer_loadfile(MPLAYER *mplayer, const char *url)
{
   fprintf(mplayer->in, "loadfile %s\n", url);
   fflush(mplayer->in);
}

