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
#include <signal.h>
#include <stdlib.h>
#include "dradio.h"


volatile sig_atomic_t got_signal = 0;

static void init_block_mask(sigset_t *block_mask);


/** block all signals */

void signals_block_all(sigset_t *orig_mask)
{
   sigset_t block_mask;

   init_block_mask(&block_mask);
   sigprocmask(SIG_BLOCK, &block_mask, orig_mask);
}


/** block no signals */

void signals_block_none()
{
   sigset_t block_mask;

   sigemptyset(&block_mask);
   sigprocmask(SIG_UNBLOCK, NULL, &block_mask);
   sigprocmask(SIG_UNBLOCK, &block_mask, NULL);
}


/** setup signal handlers */

void signals_handler(int signum)
{
   got_signal = signum;
}


/** install handler for all signals */

void signals_sigaction_all()
{
   signals_sigaction(SIGHUP,  signals_handler);
   signals_sigaction(SIGINT,  signals_handler);
   signals_sigaction(SIGQUIT, signals_handler);
   signals_sigaction(SIGPIPE, signals_handler);
   signals_sigaction(SIGTERM, signals_handler);
   signals_sigaction(SIGTSTP, signals_handler);
   signals_sigaction(SIGCHLD, signals_handler);
   signals_sigaction(SIGWINCH, signals_handler);
}


/** wrap sigaction */

void signals_sigaction(int signum, void (*handler)(int) )
{
   struct sigaction sa;
   sigset_t block_mask;

   sa.sa_handler = handler;
   sa.sa_flags = 0;

   /* using just one signal handler for a group of signals, we need
    * to block the others in the group to avoid races */
   init_block_mask(&block_mask);
   sa.sa_mask = block_mask;

   if (sigaction(signum, &sa, NULL))
   {
      perror("sigaction");
      exit(EXIT_FAILURE);
   }
}


/** init the sigset_t */

static void init_block_mask(sigset_t *block_mask)
{
   sigemptyset(block_mask);
   sigaddset(block_mask, SIGHUP);
   sigaddset(block_mask, SIGINT);
   sigaddset(block_mask, SIGQUIT);
   sigaddset(block_mask, SIGPIPE);
   sigaddset(block_mask, SIGTERM);
   sigaddset(block_mask, SIGTSTP);
   sigaddset(block_mask, SIGCHLD);
   sigaddset(block_mask, SIGWINCH);
}

