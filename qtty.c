
#include "qtty.h"

// XXX verify includes
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define CTRL(x) (x&037)
#define ESCAPE (0x1b)
#define DELETE (0x7f)

#define TTY_WIDTH  80
#define TTY_HEIGHT 25

enum {
  TTY_STATE_PLAIN,
  TTY_STATE_ESCAPE,
  TTY_STATE_ESCAPE_BRACKET,
};

struct _qtty {
  int t_state;

  off_t t_posn;

  off_t t_start;
  off_t t_end;

  qtty_handler_t t_exec_handler;
  qtty_handler_t t_help_handler;

  FILE *t_is;
  FILE *t_os;

  void *t_cookie;

  char  t_line[TTY_WIDTH+1];
  char  t_prompt[TTY_WIDTH+1];
};

void
qtty_write(qtty_t *t, const char *buf, size_t len)
{
  fwrite(buf, 1, len, t->t_os);
}

void
qtty_writestr(qtty_t *t, const char *str)
{
  qtty_write(t, str, strlen(str));
}

void
qtty_writechr(qtty_t *t, char c)
{
  qtty_write(t, &c, 1);
}

void
qtty_setprompt(qtty_t *t, const char *p)
{
  strncpy(t->t_prompt, p, sizeof(t->t_prompt));
}

qtty_t *
qtty_new(FILE *is, FILE *os)
{
  qtty_t *t = calloc(1, sizeof(qtty_t));
  t->t_is = is;
  t->t_os = os;
  qtty_setprompt(t, "> ");
  qtty_reset(t);
  return t;
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

void
qtty_message(qtty_t *t, const char *msg)
{
  qtty_writestr(t, "\r\x1b[K");
  qtty_writestr(t, msg);

  qtty_redraw(t);
}

void
qtty_reset(qtty_t *t)
{
  memcpy(t->t_line, t->t_prompt, sizeof(t->t_line));

  t->t_state = TTY_STATE_PLAIN;

  t->t_start = strlen(t->t_prompt);
  t->t_end = t->t_start;
  t->t_posn = t->t_start;

  qtty_redraw(t);
}

int
qtty_insert_at(qtty_t *t, int c, off_t p)
{
  int atend = 0;

  if(p == t->t_end) {
    atend = 1;
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

int
qtty_remove_at(qtty_t *t, off_t p)
{
  int atend = 0;

  if(p == t->t_end - 1) {
    atend = 1;
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

void
dotokenize(qtty_t *t, const char *cmd,
       void (*handler)(qtty_t *t, void *cookie, int argc, char **argv))
{
  char *tok[512];
  char buf[512];
  char *pos = buf;
  int toknum = 0;
  size_t len;

  memset(tok, 0, sizeof(tok));
  strncpy(buf, cmd, sizeof(buf)-1);

  while(*pos) {
    pos += strspn(pos, " \t\r\n");
    len = strcspn(pos, " \t\r\n");
    if(len) {
      tok[toknum++] = pos;
      pos += len;
      if(*pos) {
        *pos = 0;
        pos++;
      }
    } else {
      break;
    }
  }

  handler(t, t->t_cookie, toknum, tok);
}

void
docmd(qtty_t *t, void *cookie, int argc, char **argv)
{
  qtty_writechr(t, '\n');
  fflush(stdout);

  if(t->t_exec_handler) {
    t->t_exec_handler(t, t->t_cookie, argc, argv);
  } else {
    qtty_message(t, "No exec handler!\n");
  }

  fflush(stdout);
}

void
dohelp(qtty_t *t, void *cookie, int argc, char **argv)
{
  qtty_writechr(t, '\n');
  fflush(stdout);

  if(t->t_help_handler) {
    t->t_help_handler(t, t->t_cookie, argc, argv);
  } else {
    qtty_message(t, "No help handler!\n");
  }

  fflush(stdout);
}

void
qtty_cursor_right(qtty_t *t)
{
  if(t->t_posn < t->t_end) {
    t->t_posn++;
    qtty_writestr(t, "\x1b[C");
  }
}

void
qtty_cursor_left(qtty_t *t)
{
  if(t->t_posn > t->t_start) {
    t->t_posn--;
    qtty_writestr(t, "\x1b[D");
  }
}

void
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

void
qtty_feed_escape(qtty_t *t, int c)
{
  if(c == '[') {
    t->t_state = TTY_STATE_ESCAPE_BRACKET;
  } else {
    t->t_state = TTY_STATE_PLAIN;
  }
}

void
qtty_feed_plain(qtty_t *t, int c)
{
  if(c == '?') {
    dotokenize(t, &t->t_line[t->t_start], &dohelp);
    qtty_redraw(t);
  } else if(isgraph(c) || (c == ' ')) {
    if(qtty_insert_at(t, c, t->t_posn)) {
      t->t_posn++;
      qtty_redraw(t);
    }
  } else {
    switch(c) {
    case CTRL('l'):
      qtty_redraw(t);
      break;
    case CTRL('c'):
      qtty_writechr(t, '\n');
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
      qtty_redraw(t);
      break;
    case CTRL('e'):
      t->t_posn = t->t_end;
      qtty_redraw(t);
      break;
    case DELETE:
    case CTRL('h'):
      if(qtty_remove_at(t, t->t_posn - 1)) {
        t->t_posn--;
        qtty_redraw(t);
      }
      break;
    case CTRL('m'):
      if(strlen(&t->t_line[t->t_start])) {
        dotokenize(t, &t->t_line[t->t_start], &docmd);
      } else {
        qtty_writechr(t, '\n');
      }
      qtty_reset(t);
      break;
    case ESCAPE:
      t->t_state = TTY_STATE_ESCAPE;
      break;
    default:
      break;
    }
  }
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
qtty_process(qtty_t *t)
{
  int c;

  while((c = fgetc(t->t_is)) != EOF) {
    qtty_feed(t, c);
  }
}

void
qtty_finish(qtty_t *t)
{
  qtty_writestr(t, "\r\n");
}

void
qtty_command_handler(qtty_t *t, qtty_handler_t exec, qtty_handler_t help, void *cookie)
{
  t->t_exec_handler = exec;
  t->t_help_handler = help;
  t->t_cookie = cookie;
}
