/*
 * Copyright (C) 2016 Mario Cianciolo <mr.udda@gmail.com>
 *
 * This file is part of multiload-ng.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include <config.h>

#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <sys/utsname.h>

#include "graph-data.h"
#include "info-file.h"
#include "preferences.h"
#include "util.h"

enum {
	LOADAVG_1,
	LOADAVG_5,
	LOADAVG_15,
	LOADAVG_PROC
};

#define PATH_LOADAVG "/proc/loadavg"


void
multiload_graph_load_init (LoadGraph *g, LoadData *xd)
{
	struct utsname un;
	if (0 == uname(&un)) {
		g_snprintf(xd->uname, sizeof(xd->uname), "%s %s (%s)", un.sysname, un.release, un.machine);
	} else {
		g_warning("uname() failed: could not get kernel name and version.");
	}
}

void
multiload_graph_load_get_data (int Maximum, int data [1], LoadGraph *g, LoadData *xd, gboolean first_call)
{
	int n;

	// load average
	n = getloadavg(xd->loadavg, 3);
	g_assert_cmpint(n, >=, 0);

	// threads stats
	FILE *f = info_file_required_fopen(PATH_LOADAVG, "r");
	n = fscanf(f, "%*s %*s %*s %u/%u", &xd->proc_active, &xd->proc_count);
	fclose(f);
	g_assert_cmpint(n, ==, 2);

	int max = autoscaler_get_max(&xd->scaler, g, rint(xd->loadavg[LOADAVG_1]));
	if (max == 0) {
		data[0] = 0;
	} else {
		data [0] = rint ((float) Maximum * xd->loadavg[LOADAVG_1] / max);
	}
}

void
multiload_graph_load_cmdline_output (LoadGraph *g, LoadData *xd)
{
	g_snprintf(g->output_str[LOADAVG_1], sizeof(g->output_str[LOADAVG_1]), "%.02f", xd->loadavg[LOADAVG_1]);
	g_snprintf(g->output_str[LOADAVG_5], sizeof(g->output_str[LOADAVG_5]), "%.02f", xd->loadavg[LOADAVG_5]);
	g_snprintf(g->output_str[LOADAVG_15], sizeof(g->output_str[LOADAVG_15]), "%.02f", xd->loadavg[LOADAVG_15]);
	g_snprintf(g->output_str[LOADAVG_PROC], sizeof(g->output_str[LOADAVG_PROC]), "%u/%u", xd->proc_active, xd->proc_count);
}

void
multiload_graph_load_tooltip_update (char *buf_title, size_t len_title, char *buf_text, size_t len_text, LoadGraph *g, LoadData *xd, gint style)
{
	if (style == MULTILOAD_TOOLTIP_STYLE_DETAILED) {
		if (xd->uname[0] != 0)
			strncpy(buf_title, xd->uname, len_title);
		g_snprintf(buf_text, len_text, _(	"Last minute: %0.02f\n"
											"Last 5 minutes: %0.02f\n"
											"Last 15 minutes: %0.02f\n"
											"Processes/threads: %u active out of %u."),
											xd->loadavg[LOADAVG_1], xd->loadavg[LOADAVG_5], xd->loadavg[LOADAVG_15],
											xd->proc_active, xd->proc_count);
	} else {
		g_snprintf(buf_text, len_text, "%0.02f", xd->loadavg[LOADAVG_1]);
	}
}
