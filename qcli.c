
#include <string.h>
#include <stdio.h>

#include <stdlib.h> // temp for abort()

#include "qcli.h"

static qcli_cmd_t *
cli_find(qcli_tbl_t *t, char *token)
{
  int i;

  for(i = 0; i < t->cmdc; i++) {
    qcli_cmd_t *c = t->cmds + i;
    if(!strcmp(token, c->cmd_name)) {
      fflush(stdout); // TODO remove!?
      return c;
    }
  }

  return NULL;
}

static void indent(int level)
{
  int i;
  for(i = 0; i < level; i++) {
    printf("  ");
  }
}

static void printtable(qcli_tbl_t *t, int level)
{
  int i;
  for(i = 0; i < t->cmdc; i++) {
    qcli_cmd_t *c = t->cmds + i;
    indent(level);
    printf("%s - %s\n", c->cmd_name, c->cmd_help);
  }
}

void exec_handler(qtty_t *t, void *cookie, int argc, char **argv)
{
  qcli_t *c = (qcli_t *)cookie;
  qcli_exec(c, argc, argv);
}

void help_handler(qtty_t *t, void *cookie, int argc, char **argv)
{
  qcli_t *c = (qcli_t *)cookie;
  qcli_help(c, argc, argv);
}

int qcli_init(qcli_t *c, qtty_t *tty)
{
  c->tty = tty;
  c->commands = &main_tbl;

  qtty_command_handler(tty, &exec_handler, &help_handler, c);

  return 0;
}

int qcli_help(qcli_t *c, int argc, char **argv)
{
  int i;
  qcli_tbl_t *t = c->commands;

  for(i = 0; i < argc; i++) {
    qcli_cmd_t *cmd = cli_find(t, argv[i]);
    if(cmd) {
      indent(i);
      printf("%s - %s\n", cmd->cmd_name, cmd->cmd_help);
      if(cmd->cmd_fun) {
        printf("command is complete.");
        return 0;
      } else if (cmd->cmd_sub) {
        t = cmd->cmd_sub;
      } else {
        printf("invalid command table entry.\n");
        return 1;
      }
    } else {
      indent(i);
      printf("%s unknown, options:\n", argv[i]);
      printtable(t, i + 1);
      return 1;
    }
  }

  indent(i);
  printf("options:\n");
  printtable(t, i + 1);

  return 1;
}

int qcli_exec(qcli_t *c, int argc, char **argv)
{
  int i;
  qcli_tbl_t *t = c->commands;

  for(i = 0; i < argc; i++) {
    qcli_cmd_t *cmd = cli_find(t, argv[i]);

    if(cmd) {
      if(cmd->cmd_fun) {
        return cmd->cmd_fun(c, argc - i - 1, argv + i + 1);
      } else if (cmd->cmd_sub) {
        t = cmd->cmd_sub;
      } else {
        printf("invalid command table entry.\n");
        return 1;
      }
    } else {
      printf("unknown command.\n");
      return 1;
    }
  }

  printf("incomplete command\n");
  return 1;
}
