/**
 * @file
 * Test code for mutt_ch_convert_string()
 *
 * @authors
 * Copyright (C) 2019 Richard Russon <rich@flatcap.org>
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "email/lib.h"
#include "core/lib.h"
#include "convert/lib.h"
#include "test_common.h"

static struct ConfigDef CharsetVars[] = {
  // clang-format off
  { "config_charset", DT_STRING, 0, 0, charset_validator, },
  { "send_charset", DT_SLIST|SLIST_SEP_COLON|SLIST_ALLOW_EMPTY|DT_CHARSET_STRICT, IP "us-ascii:iso-8859-1:utf-8", 0, charset_slist_validator, },
  { NULL },
  // clang-format on
};

void test_mutt_get_content_info(void)
{
  // struct Content *mutt_get_content_info(const char *fname, struct Body *b, struct ConfigSubset *sub);

  const char *text = "file\ncontent";
  const char *tmpdir = mutt_str_getenv("TMPDIR");
  if (!tmpdir)
    tmpdir = "/tmp";

  char fname[PATH_MAX] = { 0 };
  snprintf(fname, sizeof(fname), "%s/mutt-test-file-XXXXXX", tmpdir);

  int fd = mkstemp(fname);
  TEST_CHECK(fd != -1);
  TEST_MSG("unable to open temp file for writing");

  TEST_CHECK(write(fd, text, strlen(text)) > 0);
  TEST_MSG("unable to write to temp file: %s", fname);
  close(fd);

  test_neomutt_create();
  struct ConfigSubset *sub = NeoMutt->sub;
  struct ConfigSet *cs = sub->cs;
  cs_register_variables(cs, CharsetVars, DT_NO_FLAGS);

  struct Body *body = mutt_body_new();
  struct Content *content = mutt_get_content_info(fname, body, sub);
  TEST_CHECK(content != NULL);

  TEST_CHECK(content->hibin == 0);
  TEST_MSG("Check failed: %d == 0", content->hibin);
  TEST_CHECK(content->lobin == 0);
  TEST_MSG("Check failed: %d == 0", content->lobin);
  TEST_CHECK(content->nulbin == 0);
  TEST_MSG("Check failed: %d == 0", content->nulbin);
  TEST_CHECK(content->crlf == 1);
  TEST_MSG("Check failed: %d == 1", content->crlf);
  TEST_CHECK(content->ascii == 11);
  TEST_MSG("Check failed: %d == 11", content->ascii);
  TEST_CHECK(content->linemax == 7);
  TEST_MSG("Check failed: %d == 7", content->linemax);
  TEST_CHECK(!content->space);
  TEST_MSG("Check failed: %d == 0", content->space);
  TEST_CHECK(!content->binary);
  TEST_MSG("Check failed: %d == 0", content->binary);
  TEST_CHECK(!content->from);
  TEST_MSG("Check failed: %d == 0", content->from);
  TEST_CHECK(!content->dot);
  TEST_MSG("Check failed: %d == 0", content->dot);
  TEST_CHECK(content->cr == 0);
  TEST_MSG("Check failed: %d == 0", content->cr);

  mutt_body_free(&body);
  cs_free(&cs);
  FREE(&content);
}
