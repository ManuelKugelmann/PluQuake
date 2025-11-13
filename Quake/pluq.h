/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#ifndef _PLUQ_H_
#define _PLUQ_H_

// pluq.h -- PluQ Shared Definitions
// Used by both backend (pluq_backend.c) and frontend (pluq_frontend.c)
// Three-channel architecture: Resources, Gameplay, Input

#include "quakedef.h"
#include <nng/nng.h>

// Include generated FlatBuffers C headers
#include "pluq_reader.h"
#include "pluq_builder.h"

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

#define PLUQ_URL_RESOURCES  "tcp://127.0.0.1:9001"
#define PLUQ_URL_GAMEPLAY   "tcp://127.0.0.1:9002"
#define PLUQ_URL_INPUT      "tcp://127.0.0.1:9003"

// ============================================================================
// SHARED TYPE DEFINITIONS
// ============================================================================

// Input command structure
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
// SHARED HELPER FUNCTIONS
// ============================================================================

// vec3_t conversion helpers
static inline PluQ_Vec3_t QuakeVec3_To_FB(const vec3_t v)
{
	PluQ_Vec3_t fb_vec;
	memcpy(&fb_vec, v, sizeof(PluQ_Vec3_t));
	return fb_vec;
}

static inline void FB_Vec3_To_Quake(const PluQ_Vec3_t *fb_vec, vec3_t v)
{
	memcpy(v, fb_vec, sizeof(PluQ_Vec3_t));
}

// Shared initialization
void PluQ_Init(void);

// Statistics (shared between backend and frontend)
void PluQ_GetStats(pluq_stats_t *stats);
void PluQ_SetStats(const pluq_stats_t *stats);
void PluQ_ResetStats(void);

#endif // _PLUQ_H_
