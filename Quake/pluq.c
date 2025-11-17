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

// ============================================================================
// SHARED INITIALIZATION
// ============================================================================

void PluQ_Init(void)
{
	Con_Printf("PluQ: Initializing nng library...\n");

	// nng library initialization is automatic - no explicit init needed
	// This function exists for future expansion (cvars, config, etc.)

	Con_Printf("PluQ: Initialization complete\n");
}

// ============================================================================
// FRONTEND WRAPPERS (used when compiling as frontend)
// ============================================================================

#ifdef PLUQ_FRONTEND

// Forward declarations from pluq_frontend.h
extern void PluQ_Frontend_ApplyViewAngles(void);
extern void PluQ_Frontend_Move(usercmd_t *cmd);

void PluQ_ApplyViewAngles(void)
{
	PluQ_Frontend_ApplyViewAngles();
}

void PluQ_Move(usercmd_t *cmd)
{
	PluQ_Frontend_Move(cmd);
}

#endif // PLUQ_FRONTEND

