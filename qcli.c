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

#include "qcli.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

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

static qcli_cmd_t *
qcli_find(qcli_tbl_t *t, const char *token)
{
  int i;

  for(i = 0; i < t->cmdc; i++) {
    qcli_cmd_t *c = t->cmds + i;
    if(!strcmp(token, c->cmd_name)) {
      return c;
    }
  }

  return NULL;
}

static void
qcli_tokenize(qcli_t *c, const char *cmd, qcli_fun_t fun)
{
  char *tok[512];
  char buf[512];
  char *pos = buf;
  int tokcnt = 0;
  size_t len;

  memset(tok, 0, sizeof(tok));
  strncpy(buf, cmd, sizeof(buf)-1);

  while(*pos) {
    pos += strspn(pos, " \t\r\n");
    len = strcspn(pos, " \t\r\n");
    if(len) {
      tok[tokcnt++] = pos;
      pos += len;
      if(*pos) {
        *pos = 0;
        pos++;
      }
    } else {
      break;
    }
  }

  tok[tokcnt] = NULL;

  fun(c, tokcnt, tok);
}

static void tty_exec_handler(qtty_t *t, void *cookie, const char *line) {
  qcli_t *c = (qcli_t *)cookie;
  qcli_tokenize(c, line, qcli_exec);
}

static void tty_help_handler(qtty_t *t, void *cookie, const char *line) {
  qcli_t *c = (qcli_t *)cookie;
  qcli_tokenize(c, line, qcli_help);
}

int qcli_init(qcli_t *c, qtty_t *tty, qcli_tbl_t *tbl)
{
  memset(c, 0, sizeof(*c));
  c->tty = tty;
  c->commands = tbl;

  qtty_setup(tty, "> ", c, tty_exec_handler, tty_help_handler);

  return 0;
}

int qcli_loop(qcli_t *c)
{
  return qtty_loop(c->tty);;
}

int qcli_exec(qcli_t *c, int argc, char **argv)
{
  int i;
  qcli_tbl_t *t = c->commands;

  qtty_finish(c->tty);

  for(i = 0; i < argc; i++) {
    qcli_cmd_t *cmd = qcli_find(t, argv[i]);

    if(cmd) {
      if(cmd->cmd_fun) {
        return cmd->cmd_fun(c, argc - i - 1, argv + i + 1);
      } else if (cmd->cmd_sub) {
        t = cmd->cmd_sub;
      } else {
        qtty_message(c->tty, "Invalid command table entry.\n");
        return 1;
      }
    } else {
      qtty_message(c->tty, "Unknown command.\n");
      return 1;
    }
  }

  qtty_message(c->tty, "Incomplete command.\n");
  return 1;
}

int qcli_help(qcli_t *c, int argc, char **argv)
{
  int i;
  qcli_tbl_t *t = c->commands;

  qtty_finish(c->tty);

  for(i = 0; i < argc; i++) {
    qcli_cmd_t *cmd = qcli_find(t, argv[i]);
    if(cmd) {
      indent(i);
      printf("%s - %s\n", cmd->cmd_name, cmd->cmd_help);
      if(cmd->cmd_fun) {
        printf("command is complete.");
        return 0;
      } else if (cmd->cmd_sub) {
        t = cmd->cmd_sub;
      } else {
        qtty_message(c->tty, "Invalid command table entry.\n");
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
