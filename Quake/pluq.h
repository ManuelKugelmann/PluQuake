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

// vec3_t conversion helpers (for Vec3 - full precision floats)
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

// Vec3Coord conversion helpers (for coordinates - 16-bit with 3 fractional bits)
// Schema: "divide by 8.0 to get float" -> multiply float by 8 to get int16
static inline PluQ_Vec3Coord_t QuakeVec3_To_Vec3Coord(const vec3_t v)
{
	PluQ_Vec3Coord_t coord;
	coord.x = (int16_t)(v[0] * 8.0f);
	coord.y = (int16_t)(v[1] * 8.0f);
	coord.z = (int16_t)(v[2] * 8.0f);
	return coord;
}

static inline void Vec3Coord_To_Quake(const PluQ_Vec3Coord_t *coord, vec3_t v)
{
	v[0] = coord->x / 8.0f;
	v[1] = coord->y / 8.0f;
	v[2] = coord->z / 8.0f;
}

// Vec3Angle conversion helpers (for angles - 8-bit)
// Schema: "multiply by 360.0/256.0 to get degrees" -> multiply degrees by 256/360
static inline PluQ_Vec3Angle_t QuakeAngles_To_Vec3Angle(const vec3_t v)
{
	PluQ_Vec3Angle_t angle;
	angle.pitch = (uint8_t)((int)(v[0] * 256.0f / 360.0f) & 255);
	angle.yaw = (uint8_t)((int)(v[1] * 256.0f / 360.0f) & 255);
	angle.roll = (uint8_t)((int)(v[2] * 256.0f / 360.0f) & 255);
	return angle;
}

static inline void Vec3Angle_To_Quake(const PluQ_Vec3Angle_t *angle, vec3_t v)
{
	v[0] = angle->pitch * 360.0f / 256.0f;
	v[1] = angle->yaw * 360.0f / 256.0f;
	v[2] = angle->roll * 360.0f / 256.0f;
}

// Statistics (shared between backend and frontend)
void PluQ_GetStats(pluq_stats_t *stats);
void PluQ_SetStats(const pluq_stats_t *stats);
void PluQ_ResetStats(void);

#endif // _PLUQ_H_
