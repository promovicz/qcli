
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "qtty.h"
#include "qcli.h"

static int cmd_quit(qcli_t *c, int argc, char **argv) {
  exit(0);
}

static int cmd_hello(qcli_t *c, int argc, char **argv) {
  if(argc < 1) {
    puts("Hello world!");
  } else {
    puts(argv[0]);
  }
  return 0;
}

static qcli_cmd_t main_cmd[] = {
  {"quit", "quit qcli", cmd_quit, NULL},
  {"hello", "say hello", cmd_hello, NULL},
};

static qcli_tbl_t main_tbl = {
  .cmds = main_cmd,
  .cmdc = array_size(main_cmd)
};

int main(void) {
  qtty_t t;
  qcli_t c;
  struct termios save, raw;

  qtty_init(&t, stdin, stdout);
  qcli_init(&c, &t, &main_tbl);
  qcli_loop(&c);

  return 0;
}
