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

#ifndef QCLI_H
#define QCLI_H

#include "qtty.h"

#define array_size(arr) (sizeof(arr) / sizeof((arr)[0]))

struct _qcli;
struct _qcli_cmd;
struct _qcli_tbl;

typedef struct _qcli     qcli_t;
typedef struct _qcli_cmd qcli_cmd_t;
typedef struct _qcli_tbl qcli_tbl_t;

typedef int (*qcli_fun_t) (qcli_t *c, int argc, char **argv);

struct _qcli_cmd {
  char       *cmd_name;
  char       *cmd_help;
  qcli_fun_t  cmd_fun;
  qcli_tbl_t *cmd_sub;
};

struct _qcli_tbl {
  int         cmdc;
  qcli_cmd_t *cmds;
};

#define QCLI_DEFINE(_sym)                       \
  static qcli_cmd_t _sym ## _cmds [];           \
  qcli_tbl_t _sym = {                           \
    .cmds = & _sym ## _cmds [0],                \
    .cmdc = array_size(_sym ## _cmds),          \
  };                                            \
  static qcli_cmd_t symbol ## _cmds[] =

struct _qcli {
  qtty_t *tty;
  qcli_tbl_t *commands;
};

int qcli_init(qcli_t *c, qtty_t *tty, qcli_tbl_t *tbl);

int qcli_loop(qcli_t *c);

int qcli_help(qcli_t *c, int argc, char **argv);
int qcli_exec(qcli_t *c, int argc, char **argv);

#endif /* !QCLI_H */
