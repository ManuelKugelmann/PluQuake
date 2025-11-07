/*
PluQ - Inter-Process Communication for Quake Engines
C Implementation using NNG + FlatBuffers

MIT License - See LICENSE file

Integrated into QuakeSpasm
*/

#ifndef PLUQ_NNG_H
#define PLUQ_NNG_H

#include <stdint.h>
#include <stdbool.h>

// Use QuakeSpasm's vector types
#define PLUQ_CUSTOM_TYPES
#include "quakedef.h"

#ifdef __cplusplus
extern "C" {
#endif

// QuakeSpasm uses vec3_t for angles too (pitch, yaw, roll)
typedef vec3_t angles_t;

// PluQ operation modes
typedef enum {
    PLUQ_MODE_DISABLED,   // PluQ not active
    PLUQ_MODE_BACKEND,    // Backend: run simulation, broadcast state
    PLUQ_MODE_FRONTEND,   // Frontend: receive state, send input
} pluq_mode_t;

// Transport types
typedef enum {
    PLUQ_TRANSPORT_IPC,       // Local IPC (fastest)
    PLUQ_TRANSPORT_TCP,       // Network TCP
    PLUQ_TRANSPORT_WEBSOCKET, // WebSocket (for browsers)
    PLUQ_TRANSPORT_INPROC,    // In-process (same process, different threads)
} pluq_transport_t;

// Performance statistics
typedef struct {
    uint64_t frames_sent;      // Total frames sent (backend)
    uint64_t frames_received;  // Total frames received (frontend)
    uint64_t bytes_sent;       // Total bytes sent
    uint64_t bytes_received;   // Total bytes received
    double avg_frame_time_ms;  // Average frame processing time
    double max_frame_time_ms;  // Maximum frame processing time
    double min_frame_time_ms;  // Minimum frame processing time
} pluq_stats_t;

// Configuration
typedef struct {
    pluq_mode_t mode;
    pluq_transport_t transport;
    const char *address;       // e.g., "tcp://0.0.0.0:5555", "ipc:///tmp/pluq"
    bool non_blocking;         // Non-blocking receive
    int timeout_ms;            // Timeout for blocking operations (-1 = infinite)
} pluq_config_t;

// Forward declarations for engine-specific types
// These will be defined by the integrating engine (QuakeSpasm, Ironwail, etc.)
#ifndef PLUQ_CUSTOM_TYPES
typedef struct { float x, y, z; } vec3_t;
typedef struct { float pitch, yaw, roll; } angles_t;
#endif

// === Initialization and Cleanup ===

/**
 * Initialize PluQ with configuration
 * Returns: true on success, false on failure
 */
bool PluQ_Initialize(const pluq_config_t *config);

/**
 * Shutdown PluQ and cleanup resources
 */
void PluQ_Shutdown(void);

/**
 * Check if PluQ is initialized
 */
bool PluQ_IsInitialized(void);

/**
 * Get current mode
 */
pluq_mode_t PluQ_GetMode(void);

// === Backend Functions (Broadcasting) ===

/**
 * Start building a frame
 * Call this before adding entities/lights
 */
void PluQ_BeginFrame(uint32_t sequence, double timestamp);

/**
 * Set player state for current frame
 */
void PluQ_SetPlayerState(
    const vec3_t *origin,
    const angles_t *angles,
    const vec3_t *velocity,
    float health, float armor,
    int weapon, int ammo,
    int flags
);

/**
 * Set game state for current frame
 */
void PluQ_SetGameState(
    bool paused, bool in_game, bool intermission,
    const char *mapname,
    float time, float gravity, float maxspeed
);

/**
 * Add an entity to the current frame
 */
void PluQ_AddEntity(
    const vec3_t *origin,
    const angles_t *angles,
    int model_id, int skin, int frame,
    int effects, float alpha, float scale
);

/**
 * Add a dynamic light to the current frame
 */
void PluQ_AddDLight(
    const vec3_t *origin,
    float radius,
    float color_r, float color_g, float color_b,
    float decay, int key
);

/**
 * Finish building and broadcast the frame
 * Returns: number of bytes sent, or -1 on error
 */
int PluQ_EndFrame(void);

// === Frontend Functions (Receiving) ===

/**
 * Receive a frame (blocking or non-blocking depending on config)
 * Returns: true if frame received, false otherwise
 */
bool PluQ_ReceiveFrame(void);

/**
 * Get frame metadata
 */
bool PluQ_GetFrameInfo(uint32_t *sequence, double *timestamp);

/**
 * Get player state from received frame
 */
bool PluQ_GetPlayerState(
    vec3_t *origin,
    angles_t *angles,
    vec3_t *velocity,
    float *health, float *armor,
    int *weapon, int *ammo,
    int *flags
);

/**
 * Get game state from received frame
 */
bool PluQ_GetGameState(
    bool *paused, bool *in_game, bool *intermission,
    char *mapname, size_t mapname_size,
    float *time, float *gravity, float *maxspeed
);

/**
 * Get number of entities in received frame
 */
int PluQ_GetEntityCount(void);

/**
 * Get entity data by index
 */
bool PluQ_GetEntity(
    int index,
    vec3_t *origin,
    angles_t *angles,
    int *model_id, int *skin, int *frame,
    int *effects, float *alpha, float *scale
);

/**
 * Get number of dynamic lights in received frame
 */
int PluQ_GetDLightCount(void);

/**
 * Get dynamic light data by index
 */
bool PluQ_GetDLight(
    int index,
    vec3_t *origin,
    float *radius,
    float *color_r, float *color_g, float *color_b,
    float *decay, int *key
);

// === Input Functions (Frontend -> Backend) ===

/**
 * Send input command to backend
 */
bool PluQ_SendInput(
    uint32_t sequence,
    double timestamp,
    float forward_move, float side_move, float up_move,
    const angles_t *view_angles,
    uint32_t buttons,
    uint8_t impulse,
    const char *cmd_text
);

/**
 * Receive input command (backend)
 * Returns: true if input received
 */
bool PluQ_ReceiveInput(
    uint32_t *sequence,
    double *timestamp,
    float *forward_move, float *side_move, float *up_move,
    angles_t *view_angles,
    uint32_t *buttons,
    uint8_t *impulse,
    char *cmd_text, size_t cmd_text_size
);

// === Statistics ===

/**
 * Get performance statistics
 */
void PluQ_GetStats(pluq_stats_t *stats);

/**
 * Reset statistics
 */
void PluQ_ResetStats(void);

// === Utility ===

/**
 * Get last error message
 */
const char *PluQ_GetLastError(void);

// === QuakeSpasm Integration ===

/**
 * Initialize PluQ subsystem (called from Host_Init)
 * Registers console commands and cvars
 */
void PluQ_Init(void);

#ifdef __cplusplus
}
#endif

#endif // PLUQ_NNG_H
