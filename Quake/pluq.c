/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// pluq.c -- PluQ Shared Code (used by both backend and frontend)

#include "quakedef.h"
#include "pluq.h"
#include <string.h>

// ============================================================================
// SHARED STATISTICS
// ============================================================================

static pluq_stats_t perf_stats = {0};

void PluQ_GetStats(pluq_stats_t *stats)
{
	if (stats)
		*stats = perf_stats;
}

void PluQ_SetStats(const pluq_stats_t *stats)
{
	if (stats)
		perf_stats = *stats;
}

void PluQ_ResetStats(void)
{
	memset(&perf_stats, 0, sizeof(perf_stats));
}

