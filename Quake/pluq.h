/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#ifndef _PLUQ_H_
#define _PLUQ_H_

// pluq.h -- PLQ Shared Definitions
// Used by both backend (pluq_backend.c) and frontend (pluq_frontend.c)
// Three-channel architecture: Resources, Gameplay, Input

#include "quakedef.h"
#include <nng/nng.h>

// Include generated FlatBuffers C headers (new PLQ schema)
#include "plq_reader.h"
#include "plq_builder.h"

// nng 1.x protocol headers (needed by both backend and frontend)
#include <nng/protocol/reqrep0/req.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pubsub0/sub.h>
#include <nng/protocol/pipeline0/push.h>
#include <nng/protocol/pipeline0/pull.h>

// ============================================================================
// CHANNEL ENDPOINTS (shared between backend and frontend)
// ============================================================================

// Use IPC (Unix domain sockets) for local communication - more reliable on WSL
#define PLUQ_URL_RESOURCES  "ipc:///tmp/pluq_resources.ipc"
#define PLUQ_URL_GAMEPLAY   "ipc:///tmp/pluq_gameplay.ipc"
#define PLUQ_URL_INPUT      "ipc:///tmp/pluq_input.ipc"

// ============================================================================
// SHARED TYPE DEFINITIONS
// ============================================================================

// Input command structure (internal representation)
typedef struct
{
	uint32_t sequence;
	double timestamp;
	float forward_move, side_move, up_move;
	vec3_t view_angles;
	uint32_t buttons;
	uint8_t impulse;
	char cmd_text[256];
} pluq_input_cmd_t;

// Performance statistics
typedef struct
{
	uint64_t frames_sent;
	double total_time;
	size_t total_entities;
	double max_frame_time, min_frame_time;
} pluq_stats_t;

// ============================================================================
// VIEWSTATE FLAGS
// ============================================================================

#define PLQ_FLAG_PAUSED      (1 << 0)
#define PLQ_FLAG_ONGROUND    (1 << 1)
#define PLQ_FLAG_INWATER     (1 << 2)
#define PLQ_FLAG_INTERMISSION (1 << 3)

// ============================================================================
// SHARED HELPER FUNCTIONS
// ============================================================================

// Statistics (shared between backend and frontend)
void PluQ_GetStats(pluq_stats_t *stats);
void PluQ_SetStats(const pluq_stats_t *stats);
void PluQ_ResetStats(void);

// Shared initialization
void PluQ_Init(void);

#endif // _PLUQ_H_
