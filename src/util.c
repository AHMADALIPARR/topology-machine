/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "tm.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const char *tm_strerror(int err)
{
    switch (err) {
    case TM_OK: return "ok";
    case TM_ERR_RANGE: return "value out of range";
    case TM_ERR_PARSE: return "malformed input";
    case TM_ERR_CAPACITY: return "capacity exceeded";
    case TM_ERR_SINGULAR: return "singular or ill-conditioned matrix";
    case TM_ERR_IO: return "input/output error";
    case TM_ERR_DOMAIN: return "domain error";
    case TM_ERR_NOMEM: return "out of memory";
    default: return "unknown error";
    }
}

int tm_parse_u64(const char *s, uint64_t *out)
{
    int base = 10;
    char *end = NULL;
    unsigned long long v;

    if (!s || !out || !*s)
        return TM_ERR_PARSE;
    if (s[0] == '+' || s[0] == '-')
        return TM_ERR_PARSE;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s += 2;
        if (!*s)
            return TM_ERR_PARSE;
    }
    errno = 0;
    v = strtoull(s, &end, base);
    if (end == s || (end && *end != '\0') || errno == ERANGE)
        return TM_ERR_PARSE;
    *out = (uint64_t)v;
    return TM_OK;
}

int tm_mkdir(const char *path)
{
    struct stat st;

    if (!path || !*path)
        return TM_ERR_IO;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode))
            return TM_OK;
        return TM_ERR_IO;
    }
    if (mkdir(path, 0777) == 0)
        return TM_OK;
    return TM_ERR_IO;
}

int tm_write_text(const char *path, const char *text)
{
    FILE *fp;
    size_t n, w;

    if (!path || !text)
        return TM_ERR_IO;
    fp = fopen(path, "w");
    if (!fp)
        return TM_ERR_IO;
    n = strlen(text);
    w = fwrite(text, 1, n, fp);
    if (w != n || ferror(fp)) {
        fclose(fp);
        return TM_ERR_IO;
    }
    if (fclose(fp) != 0)
        return TM_ERR_IO;
    return TM_OK;
}
