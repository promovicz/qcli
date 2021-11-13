
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "qtty.h"
#include "qcli.h"

int main(void) {
  qtty_t t;
  qcli_t c;
  struct termios save, raw;

  cfmakeraw(&raw);
  tcgetattr(1, &save);
  tcsetattr(1, TCSADRAIN, &raw);

  qtty_init(&t, stdin, stdout);
  qcli_init(&c, &t);
  qcli_loop(&c);

  tcsetattr(1, TCSADRAIN, &save);

  return 0;
}
