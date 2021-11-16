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

#include "qtty.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* ASCII constants */
#define CTRL(x) (x&037)
#define ESCAPE (0x1b)
#define DELETE (0x7f)

static void
qtty_write(qtty_t *t, const char *buf, size_t len)
{
#ifdef USE_STDIO
  fwrite(buf, 1, len, t->t_os);
#else
  write(t->t_ofd, buf, len);
#endif
}

static void
qtty_writestr(qtty_t *t, const char *str)
{
  qtty_write(t, str, strlen(str));
}

static void
qtty_writechr(qtty_t *t, char c)
{
  qtty_write(t, &c, 1);
}

int
qtty_init(qtty_t *t,
#ifdef USE_STDIO
          FILE *is, FILE *os
#else
          int ifd, int ofd
#endif
          )
{
  memset(t, 0, sizeof(*t));
#ifdef USE_STDIO
  t->t_is = is;
  t->t_os = os;
#else
  t->t_ifd = ifd;
  t->t_ofd = ofd;
#endif
  return 0;
}

static void
qtty_enter(qtty_t *t)
{
  struct termios raw;
  if(isatty(1) == 1) {
    cfmakeraw(&raw);
    tcgetattr(1, &t->t_save);
    tcsetattr(1, TCSADRAIN, &raw);
  }
}

static void
qtty_leave(qtty_t *t)
{
  if(isatty(1) == 1) {
    tcsetattr(1, TCSADRAIN, &t->t_save);
  }
}

int
qtty_loop(qtty_t *t) {
#ifdef USE_STDIO
  int c;
#else
  int res;
  char buf[1];
#endif
  qtty_enter(t);
  qtty_redraw(t);
#ifdef USE_STDIO
  while((c = fgetc(t->t_is)) != EOF) {
    qtty_feed(t, c);
  }
#else
  while((res = read(t->t_ifd, &buf, 1)) != 0) {
      if(res == -1) {
        break;
      }
      qtty_feed(t, buf[0]);
  }
#endif
  qtty_leave(t);
  return 0;
}

void
qtty_setup(qtty_t *t,
           const char *prompt,
           void *cookie,
           qtty_handler_t exec,
           qtty_handler_t help)
{
  strncpy(t->t_prompt, prompt, sizeof(t->t_prompt));
  t->t_cookie = cookie;
  t->t_exec_handler = exec;
  t->t_help_handler = help;
  qtty_clear(t);
}

void
qtty_clear(qtty_t *t)
{
  memcpy(t->t_line, t->t_prompt, sizeof(t->t_line));

  t->t_start = strlen(t->t_prompt);
  t->t_end = t->t_start;
  t->t_posn = t->t_start;
}

void
qtty_finish(qtty_t *t)
{
  qtty_writestr(t, "\r\n");
}

void
qtty_message(qtty_t *t, const char *msg)
{
  qtty_writestr(t, "\r\x1b[K");
  qtty_writestr(t, msg);

  qtty_redraw(t);
}

void
qtty_redraw(qtty_t *t)
{
  qtty_writestr(t, "\r\x1b[K");
  qtty_writestr(t, t->t_line);

  int i = t->t_end;
  while(i > t->t_posn) {
    qtty_writechr(t, '\b');
    i--;
  }
}

static void
qtty_cursor_right(qtty_t *t)
{
  if(t->t_posn < t->t_end) {
    t->t_posn++;
    qtty_writestr(t, "\x1b[C");
  }
}

static void
qtty_cursor_left(qtty_t *t)
{
  if(t->t_posn > t->t_start) {
    t->t_posn--;
    qtty_writestr(t, "\x1b[D");
  }
}

static int
qtty_insert_at(qtty_t *t, int c, off_t p)
{
  bool atend = false;

  assert(p >= 0 && p < sizeof(t->t_line));

  if(p == t->t_end) {
    atend = true;
  }

  if(t->t_start <= p && p <= t->t_end) {
    if(t->t_end + 1 < (int)sizeof(t->t_line)) {
      memmove(&t->t_line[p+1], &t->t_line[p], t->t_end - p);
      t->t_line[p] = c;
      t->t_end += 1;

      if(atend) {
        qtty_writechr(t, c);
      } else {
        qtty_redraw(t);
      }

      return 1;
    }
  }

  return 0;
}

static int
qtty_remove_at(qtty_t *t, off_t p)
{
  bool atend = false;

  assert(p >= 0 && p < sizeof(t->t_line));

  if(p == t->t_end - 1) {
    atend = true;
  }

  if(t->t_start <= p && p < t->t_end) {
    if(t->t_start <= (t->t_end - 1)) {
      memmove(&t->t_line[p], &t->t_line[p+1], (t->t_end - p - 1));
      t->t_end -= 1;
      t->t_line[t->t_end] = 0;

      if(atend) {
        qtty_writestr(t, "\b \b");
      } else {
        qtty_redraw(t);
      }

      return 1;
    }
  }

  return 0;
}

static void
qtty_feed_escape_bracket(qtty_t *t, int c)
{
  switch(c) {
  case 'A':
    break;
  case 'B':
    break;
  case 'C':
    qtty_cursor_right(t);
    break;
  case 'D':
    qtty_cursor_left(t);
    break;
  default:
    break;
  }
  t->t_state = TTY_STATE_PLAIN;
}

static void
qtty_feed_escape(qtty_t *t, int c)
{
  if(c == '[') {
    t->t_state = TTY_STATE_ESCAPE_BRACKET;
  } else {
    t->t_state = TTY_STATE_PLAIN;
  }
}

static void
qtty_feed_plain(qtty_t *t, int c)
{
  bool redraw = false;
  bool newline = false;
  if(c == '?') {
    if(t->t_help_handler) {
      qtty_leave(t);
      t->t_help_handler(t, t->t_cookie, &t->t_line[t->t_start]);
      qtty_enter(t);
    }
    redraw = true;
  } else if(isgraph(c) || (c == ' ')) {
    if(qtty_insert_at(t, c, t->t_posn)) {
      t->t_posn++;
      redraw = true;
    }
  } else {
    switch(c) {
    case CTRL('l'):
      newline = true;
      qtty_reset(t);
      break;
    case CTRL('c'):
      newline = true;
      qtty_reset(t);
      break;
    case CTRL('d'):
      newline = true;
      qtty_reset(t);
      break;
    case CTRL('b'):
      qtty_cursor_left(t);
      break;
    case CTRL('f'):
      qtty_cursor_right(t);
      break;
    case CTRL('a'):
      t->t_posn = t->t_start;
      redraw = true;
      break;
    case CTRL('e'):
      t->t_posn = t->t_end;
      redraw = true;
      break;
    case DELETE:
    case CTRL('h'):
      if(qtty_remove_at(t, t->t_posn - 1)) {
        t->t_posn--;
        redraw = true;
      }
      break;
    case CTRL('m'):
      if(strlen(&t->t_line[t->t_start])) {
        if(t->t_exec_handler) {
          qtty_leave(t);
          t->t_exec_handler(t, t->t_cookie, &t->t_line[t->t_start]);
          qtty_enter(t);
        }
      } else {
        newline = true;
      }
      qtty_clear(t);
      redraw = true;
      break;
    case ESCAPE:
      t->t_state = TTY_STATE_ESCAPE;
      break;
    default:
      break;
    }
  }
  if(newline) {
    qtty_writechr(t, '\n');
  }
  if(newline || redraw) {
    qtty_redraw(t);
  }
#ifdef USE_STDIO
  fflush(t->t_os);
#endif
}

void
qtty_feed(qtty_t *t, int c)
{
  switch(t->t_state) {
  case TTY_STATE_PLAIN:
    qtty_feed_plain(t, c);
    break;
  case TTY_STATE_ESCAPE:
    qtty_feed_escape(t, c);
    break;
  case TTY_STATE_ESCAPE_BRACKET:
    qtty_feed_escape_bracket(t, c);
    break;
  default:
    /* for safety */
    t->t_state = TTY_STATE_PLAIN;
    break;
  }
}

void
qtty_reset(qtty_t *t)
{
  t->t_state = TTY_STATE_PLAIN;
  qtty_redraw(t);
}
