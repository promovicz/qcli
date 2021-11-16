/*
 * qcli - Tiny command interface library
 *
 * Copyright (C) 2020-2021 Ingo Albrecht <copyright@promovicz.org>
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#ifndef QTTY_H
#define QTTY_H

#include <sys/types.h>
#include <stdio.h>
#include <termios.h>

struct _qtty;

typedef struct _qtty qtty_t;

typedef void (*qtty_handler_t) (qtty_t *t,
                                void *cookie,
                                const char *line);

#define TTY_WIDTH  80
#define TTY_HEIGHT 25

enum {
  TTY_STATE_PLAIN,
  TTY_STATE_ESCAPE,
  TTY_STATE_ESCAPE_BRACKET,
};

struct _qtty {
  struct termios t_save;

#ifdef USE_STDIO
  FILE *t_is;
  FILE *t_os;
#else
  int t_ifd;
  int t_ofd;
#endif

  int t_state;

  void *t_cookie;
  qtty_handler_t t_exec_handler;
  qtty_handler_t t_help_handler;

  off_t t_posn;
  off_t t_start;
  off_t t_end;

  char  t_line[TTY_WIDTH+1];
  char  t_prompt[TTY_WIDTH+1];
};

/* initialize a qtty instance */
#ifdef USE_STDIO
extern int qtty_init(qtty_t *t, FILE *is, FILE *os);
#else
extern int qtty_init(qtty_t *t, int ifd, int ofd);
#endif

extern void qtty_setup(qtty_t *t,
                       const char *prompt,
                       void *cookie,
                       qtty_handler_t exec,
                       qtty_handler_t help);

/* feed one input character */
extern void qtty_feed(qtty_t *t, int c);
/* reset the decoding state machine */
extern void qtty_reset(qtty_t *t);

extern int qtty_loop(qtty_t *t);

extern void qtty_message(qtty_t *t, const char *msg);

extern void qtty_clear(qtty_t *t);
extern void qtty_redraw(qtty_t *t);
extern void qtty_finish(qtty_t *t);

#endif /* !QTTY_H */
