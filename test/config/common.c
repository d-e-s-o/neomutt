/**
 * @file
 * Shared Testing Code
 *
 * @authors
 * Copyright (C) 2017-2018 Richard Russon <rich@flatcap.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define TEST_NO_MAIN
#include "config.h"
#include "acutest.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "common.h"

const char *line = "--------------------------------------------------------------------------------";

bool dont_fail = false;

int validator_fail(const struct ConfigSet *cs, const struct ConfigDef *cdef,
                   intptr_t value, struct Buffer *result)
{
  if (dont_fail)
    return CSR_SUCCESS;

  if (value > 1000000)
    buf_printf(result, "%s: %s, (ptr)", __func__, cdef->name);
  else
    buf_printf(result, "%s: %s, %ld", __func__, cdef->name, value);
  return CSR_ERR_INVALID;
}

int validator_warn(const struct ConfigSet *cs, const struct ConfigDef *cdef,
                   intptr_t value, struct Buffer *result)
{
  if (value > 1000000)
    buf_printf(result, "%s: %s, (ptr)", __func__, cdef->name);
  else
    buf_printf(result, "%s: %s, %ld", __func__, cdef->name, value);
  return CSR_SUCCESS | CSR_SUC_WARNING;
}

int validator_succeed(const struct ConfigSet *cs, const struct ConfigDef *cdef,
                      intptr_t value, struct Buffer *result)
{
  if (value > 1000000)
    buf_printf(result, "%s: %s, (ptr)", __func__, cdef->name);
  else
    buf_printf(result, "%s: %s, %ld", __func__, cdef->name, value);
  return CSR_SUCCESS;
}

void log_line(const char *fn)
{
  int len = 44 - mutt_str_len(fn);
  TEST_MSG("\033[36m---- %s %.*s\033[m\n", fn, len, line);
}

void short_line(void)
{
  TEST_MSG("%s\n", line + 40);
}

int log_observer(struct NotifyCallback *nc)
{
  if (!nc)
    return -1;

  struct EventConfig *ec = nc->event_data;

  struct Buffer *result = buf_pool_get();

  const char *events[] = { "set", "reset", "initial-set" };

  buf_reset(result);

  cs_he_string_get(ec->sub->cs, ec->he, result);

  TEST_MSG("Event: %s has been %s to '%s'\n", ec->name,
           events[nc->event_subtype - 1], buf_string(result));

  buf_pool_release(&result);
  return true;
}

void set_list(const struct ConfigSet *cs)
{
  log_line(__func__);
  cs_dump_set(cs);
  log_line(__func__);
}

int sort_list_cb(const void *a, const void *b)
{
  const char *stra = *(char const *const *) a;
  const char *strb = *(char const *const *) b;
  return mutt_str_cmp(stra, strb);
}

void cs_dump_set(const struct ConfigSet *cs)
{
  if (!cs)
    return;

  struct HashElem *he = NULL;
  struct HashWalkState state;
  memset(&state, 0, sizeof(state));

  struct Buffer *result = buf_pool_get();

  char tmp[128];
  char *list[26] = { 0 };
  size_t index = 0;
  size_t i = 0;

  for (i = 0; (he = mutt_hash_walk(cs->hash, &state)); i++)
  {
    if (he->type == DT_SYNONYM)
      continue;

    const char *name = NULL;

    if (he->type & DT_INHERITED)
    {
      struct Inheritance *inh = he->data;
      he = inh->parent;
      name = inh->name;
    }
    else
    {
      name = he->key.strkey;
    }

    const struct ConfigSetType *cst = cs_get_type_def(cs, he->type);
    if (!cst)
    {
      snprintf(tmp, sizeof(tmp), "Unknown type: %d", he->type);
      list[index] = mutt_str_dup(tmp);
      index++;
      continue;
    }

    buf_reset(result);
    struct ConfigDef *cdef = he->data;

    int rc = cst->string_get(cs, &cdef->var, cdef, result);
    if (CSR_RESULT(rc) == CSR_SUCCESS)
      snprintf(tmp, sizeof(tmp), "%s %s = %s", cst->name, name, buf_string(result));
    else
      snprintf(tmp, sizeof(tmp), "%s %s: ERROR: %s", cst->name, name, buf_string(result));
    list[index] = mutt_str_dup(tmp);
    index++;
  }

  qsort(list, index, sizeof(list[0]), sort_list_cb);
  for (i = 0; list[i]; i++)
  {
    TEST_MSG("%s\n", list[i]);
    FREE(&list[i]);
  }

  buf_pool_release(&result);
}
