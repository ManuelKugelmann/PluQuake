/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#ifndef _PLUQ_BACKEND_H_
#define _PLUQ_BACKEND_H_

// pluq_backend.h -- PluQ Backend (Server) IPC API
// Backend listens on REP, PUB, PULL sockets

#include "pluq.h"

// ============================================================================
// BACKEND INITIALIZATION
// ============================================================================

void PluQ_Backend_Init(void);
qboolean PluQ_Backend_Enable(void);
void PluQ_Backend_Shutdown(void);
qboolean PluQ_Backend_IsEnabled(void);

// ============================================================================
// BACKEND TRANSPORT LAYER
// ============================================================================

qboolean PluQ_Backend_SendResource(const void *flatbuf, size_t size);
qboolean PluQ_Backend_PublishFrame(const void *flatbuf, size_t size);
qboolean PluQ_Backend_ReceiveInput(void **flatbuf_out, size_t *size_out);

// ============================================================================
// BACKEND HIGH-LEVEL API
// ============================================================================

void PluQ_BroadcastWorldState(void);
qboolean PluQ_HasPendingInput(void);
void PluQ_ProcessInputCommands(void);
void PluQ_Move(usercmd_t *cmd);
void PluQ_ApplyViewAngles(void);

#endif // _PLUQ_BACKEND_H_
