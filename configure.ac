#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# Compile pedantic and with debug symbols:
#    CFLAGS="-ggdb -Wall -Wextra -pedantic -ansi -O0" ./configure
#
# Verbose output can be enabled with
#    "./configure --disable-silent-rules" or "make V=1"
#

AC_PREREQ(2.61)
AC_INIT(dradio, 3.8)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC

# features of Posix that are extensions to C (define _GNU_SOURCE)
AC_USE_SYSTEM_EXTENSIONS

# Checks for libraries.
AC_SEARCH_LIBS(initscr, ncursesw,,
             [AC_MSG_ERROR([libcursesw is required!])])
AC_SEARCH_LIBS(new_menu, menuw,,
             [AC_MSG_ERROR([libmenuw is required!])])
AC_SEARCH_LIBS(XML_ParserCreate, expat,,
             [AC_MSG_ERROR([libexpat is required!])])
AC_SEARCH_LIBS(curl_easy_init, curl,,
             [AC_MSG_ERROR([libcurl is required!])])
         
# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([iconv.h\
                  langinfo.h\
                  locale.h\
                  signal.h\
                  stdio.h\
                  stdlib.h\
                  string.h\
                  sys/stat.h\
                  sys/types.h\
                  fcntl.h\
                  expat.h\
                  ncursesw/menu.h],,
                  [AC_MSG_ERROR([required header not found])] )

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([mkdir nl_langinfo setlocale strdup strrchr strncpy])

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 src/Makefile])
AC_OUTPUT
