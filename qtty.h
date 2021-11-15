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

  FILE *t_is;
  FILE *t_os;

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
extern int qtty_init(qtty_t *t, FILE *is, FILE *os);

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
