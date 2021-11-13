#ifndef QTTY_H
#define QTTY_H

#include <sys/types.h>
#include <stdio.h>

struct _qtty;

typedef struct _qtty qtty_t;

typedef void (*qtty_handler_t) (qtty_t *t, void *cookie,
                                int argc, char **argv);

qtty_t *qtty_new(FILE *is, FILE *os);

void qtty_reset(qtty_t *t);

void qtty_feed(qtty_t *t, int c);

void qtty_process(qtty_t *t);

void qtty_message(qtty_t *t, const char *msg);

void qtty_command_handler(qtty_t *t, qtty_handler_t handler,
                          qtty_handler_t help, void *cookie);

#endif /* !QTTY_H */
