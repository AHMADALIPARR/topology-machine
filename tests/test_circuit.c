/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "check.h"
#include "tm.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    tm_net *net = NULL;
    tm_net *round = NULL;
    char err[128];
    FILE *mem;
    char buf[256 * 1024];
    int a, b;
    size_t n;
    CHECK(tm_net_gf256_mul(&net) == TM_OK);
    CHECK(tm_net_ninputs(net) == 16);
    CHECK(tm_net_noutputs(net) == 8);
    CHECK(tm_net_ngates(net) > 16);
    CHECK(tm_net_ngates(net) < TM_NET_MAX_GATES);
    for (a = 0; a < 256; a++) {
        for (b = 0; b < 256; b++) {
            uint64_t in = (uint64_t)a | ((uint64_t)b << 8);
            uint64_t y = 0;
            CHECK(tm_net_eval(net, in, &y) == TM_OK);
            CHECK((uint8_t)y == tm_gf_mul((uint8_t)a, (uint8_t)b));
        }
    }
    mem = tmpfile();
    CHECK(mem != NULL);
    CHECK(tm_net_write(net, mem) == TM_OK);
    rewind(mem);
    n = fread(buf, 1, sizeof buf - 1, mem);
    buf[n] = '\0';
    CHECK(strstr(buf, "tm-netlist 1\n") != NULL);
    CHECK(strstr(buf, "input a0\n") != NULL);
    CHECK(strstr(buf, "output y7 ") != NULL);
    rewind(mem);
    CHECK(tm_net_read(mem, &round, err, sizeof err) == TM_OK);
    fclose(mem);
    for (a = 0; a < 256; a += 17) {
        for (b = 0; b < 256; b += 19) {
            uint64_t in = (uint64_t)a | ((uint64_t)b << 8);
            uint64_t y1 = 0, y2 = 0;
            CHECK(tm_net_eval(net, in, &y1) == TM_OK);
            CHECK(tm_net_eval(round, in, &y2) == TM_OK);
            CHECK(y1 == y2);
        }
    }
    {
        const char *bad[] = {
            "",
            "not a netlist\n",
            "tm-netlist 1\n",
            "tm-netlist 1\noutput y a\n",
            "tm-netlist 1\nand a b t\noutput y t\n",
            "tm-netlist 1\ninput a\ninput a\noutput y a\n",
            "tm-netlist 2\n",
            "tm-netlist 1\nconst 2 c\noutput y c\n",
            "tm-netlist 1\ninput a\nxor a z t\noutput y t\n",
            NULL
        };
        int i;
        for (i = 0; bad[i]; i++) {
            tm_net *n2 = NULL;
            FILE *fp = tmpfile();
            CHECK(fp != NULL);
            fputs(bad[i], fp);
            rewind(fp);
            CHECK(tm_net_read(fp, &n2, err, sizeof err) != TM_OK);
            CHECK(n2 == NULL);
            fclose(fp);
        }
    }
    {
        const char *ok =
            "tm-netlist 1\n"
            "input a\n"
            "input b\n"
            "and a b t\n"
            "not t n\n"
            "output y n\n";
        tm_net *n2 = NULL;
        FILE *fp = tmpfile();
        uint64_t y = 0;
        fputs(ok, fp);
        rewind(fp);
        CHECK(tm_net_read(fp, &n2, err, sizeof err) == TM_OK);
        CHECK(tm_net_eval(n2, 0, &y) == TM_OK && y == 1);
        CHECK(tm_net_eval(n2, 3, &y) == TM_OK && y == 0);
        tm_net_free(n2);
        fclose(fp);
    }
    tm_net_free(round);
    tm_net_free(net);
    return check_report("circuit");
}
