/*
PluQ - Inter-Process Communication for Quake Engines
C Implementation using NNG + FlatBuffers

MIT License - See LICENSE file
Integrated into QuakeSpasm
*/

#include "pluq.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifndef _WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif

#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pubsub0/sub.h>

// FlatCC runtime for C
#include "flatcc/flatcc_builder.h"
#include "generated/pluq_builder.h"

// Global state
static struct {
    pluq_config_t config;
    nng_socket socket;
    bool initialized;
    pluq_stats_t stats;
    char lastError[256];

    // Backend state
    flatcc_builder_t builder;

    // Frontend state
    uint8_t *receivedData;
    size_t receivedSize;
    const PluQ_Frame_table_t receivedFrame;

    // Timing
    double startTime;
} g_pluq = {0};

// Helper: Get current time in seconds
static double PluQ_GetTime(void)
{
#ifndef _WIN32
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
#else
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart;
#endif
}

// Helper: Set last error
static void PluQ_SetError(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_pluq.lastError, sizeof(g_pluq.lastError), fmt, args);
    va_end(args);
}

// Initialize PluQ
bool PluQ_Initialize(const pluq_config_t *config)
{
    if (g_pluq.initialized) {
        PluQ_SetError("Already initialized");
        return false;
    }

    if (!config) {
        PluQ_SetError("Invalid config");
        return false;
    }

    g_pluq.config = *config;
    g_pluq.startTime = PluQ_GetTime();

    int rv;

    // Create socket based on mode
    if (config->mode == PLUQ_MODE_BACKEND) {
        // Backend: Publisher
        rv = nng_pub0_open(&g_pluq.socket);
        if (rv != 0) {
            PluQ_SetError("Failed to create publisher: %s", nng_strerror(rv));
            return false;
        }

        // Listen on address
        rv = nng_listen(g_pluq.socket, config->address, NULL, 0);
        if (rv != 0) {
            PluQ_SetError("Failed to listen on %s: %s", config->address, nng_strerror(rv));
            nng_close(g_pluq.socket);
            return false;
        }

        // Initialize FlatBuffers builder
        flatcc_builder_init(&g_pluq.builder);

    } else if (config->mode == PLUQ_MODE_FRONTEND) {
        // Frontend: Subscriber
        rv = nng_sub0_open(&g_pluq.socket);
        if (rv != 0) {
            PluQ_SetError("Failed to create subscriber: %s", nng_strerror(rv));
            return false;
        }

        // Subscribe to all topics
        rv = nng_socket_set(g_pluq.socket, NNG_OPT_SUB_SUBSCRIBE, "", 0);
        if (rv != 0) {
            PluQ_SetError("Failed to subscribe: %s", nng_strerror(rv));
            nng_close(g_pluq.socket);
            return false;
        }

        // Set non-blocking if requested
        if (config->non_blocking) {
            nng_duration timeout = 0;
            nng_socket_set_ms(g_pluq.socket, NNG_OPT_RECVTIMEO, timeout);
        } else if (config->timeout_ms >= 0) {
            nng_socket_set_ms(g_pluq.socket, NNG_OPT_RECVTIMEO, config->timeout_ms);
        }

        // Connect to address
        rv = nng_dial(g_pluq.socket, config->address, NULL, 0);
        if (rv != 0) {
            PluQ_SetError("Failed to connect to %s: %s", config->address, nng_strerror(rv));
            nng_close(g_pluq.socket);
            return false;
        }

    } else {
        PluQ_SetError("Invalid mode");
        return false;
    }

    g_pluq.initialized = true;
    memset(&g_pluq.stats, 0, sizeof(g_pluq.stats));
    g_pluq.stats.min_frame_time_ms = 999999.0;

    return true;
}

// Shutdown PluQ
void PluQ_Shutdown(void)
{
    if (!g_pluq.initialized)
        return;

    if (g_pluq.config.mode == PLUQ_MODE_BACKEND) {
        flatcc_builder_clear(&g_pluq.builder);
    } else if (g_pluq.config.mode == PLUQ_MODE_FRONTEND) {
        free(g_pluq.receivedData);
        g_pluq.receivedData = NULL;
    }

    nng_close(g_pluq.socket);
    g_pluq.initialized = false;
}

bool PluQ_IsInitialized(void)
{
    return g_pluq.initialized;
}

pluq_mode_t PluQ_GetMode(void)
{
    return g_pluq.config.mode;
}

// === Backend Functions ===

void PluQ_BeginFrame(uint32_t sequence, double timestamp)
{
    if (!g_pluq.initialized || g_pluq.config.mode != PLUQ_MODE_BACKEND)
        return;

    flatcc_builder_reset(&g_pluq.builder);
}

void PluQ_SetPlayerState(
    const vec3_t *origin, const angles_t *angles, const vec3_t *velocity,
    float health, float armor, int weapon, int ammo, int flags)
{
    // Store in builder - will be used in EndFrame
    // For simplicity, store in static variables
    static struct {
        vec3_t origin, velocity;
        angles_t angles;
        float health, armor;
        int weapon, ammo, flags;
    } player_data;

    // QuakeSpasm's vec3_t is an array, need to copy element by element
    memcpy(player_data.origin, origin, sizeof(vec3_t));
    memcpy(player_data.angles, angles, sizeof(vec3_t));
    memcpy(player_data.velocity, velocity, sizeof(vec3_t));
    player_data.health = health;
    player_data.armor = armor;
    player_data.weapon = weapon;
    player_data.ammo = ammo;
    player_data.flags = flags;
}

void PluQ_SetGameState(
    bool paused, bool in_game, bool intermission,
    const char *mapname, float time, float gravity, float maxspeed)
{
    // Store in builder - will be used in EndFrame
    static struct {
        bool paused, in_game, intermission;
        char mapname[64];
        float time, gravity, maxspeed;
    } game_data;

    game_data.paused = paused;
    game_data.in_game = in_game;
    game_data.intermission = intermission;
    strncpy(game_data.mapname, mapname, sizeof(game_data.mapname) - 1);
    game_data.time = time;
    game_data.gravity = gravity;
    game_data.maxspeed = maxspeed;
}

void PluQ_AddEntity(
    const vec3_t *origin, const angles_t *angles,
    int model_id, int skin, int frame, int effects, float alpha, float scale)
{
    // Add to entities vector in builder
    // Simplified - in real implementation, accumulate in dynamic array
}

void PluQ_AddDLight(
    const vec3_t *origin, float radius,
    float color_r, float color_g, float color_b,
    float decay, int key)
{
    // Add to dlights vector in builder
    // Simplified - in real implementation, accumulate in dynamic array
}

int PluQ_EndFrame(void)
{
    if (!g_pluq.initialized || g_pluq.config.mode != PLUQ_MODE_BACKEND)
        return -1;

    double start = PluQ_GetTime();

    // Finalize FlatBuffers message
    // (Simplified - actual implementation would build complete frame)
    void *buf;
    size_t size;
    buf = flatcc_builder_finalize_buffer(&g_pluq.builder, &size);

    if (!buf) {
        PluQ_SetError("Failed to finalize buffer");
        return -1;
    }

    // Send via NNG
    int rv = nng_send(g_pluq.socket, buf, size, NNG_FLAG_ALLOC);
    if (rv != 0) {
        PluQ_SetError("Failed to send: %s", nng_strerror(rv));
        free(buf);
        return -1;
    }

    // Update statistics
    double elapsed = (PluQ_GetTime() - start) * 1000.0; // ms
    g_pluq.stats.frames_sent++;
    g_pluq.stats.bytes_sent += size;
    g_pluq.stats.avg_frame_time_ms =
        (g_pluq.stats.avg_frame_time_ms * (g_pluq.stats.frames_sent - 1) + elapsed) /
        g_pluq.stats.frames_sent;
    if (elapsed > g_pluq.stats.max_frame_time_ms)
        g_pluq.stats.max_frame_time_ms = elapsed;
    if (elapsed < g_pluq.stats.min_frame_time_ms)
        g_pluq.stats.min_frame_time_ms = elapsed;

    return (int)size;
}

// === Frontend Functions ===

bool PluQ_ReceiveFrame(void)
{
    if (!g_pluq.initialized || g_pluq.config.mode != PLUQ_MODE_FRONTEND)
        return false;

    double start = PluQ_GetTime();

    // Receive message
    void *buf = NULL;
    size_t size;
    int rv = nng_recv(g_pluq.socket, &buf, &size, NNG_FLAG_ALLOC);

    if (rv == NNG_ETIMEDOUT || rv == NNG_EAGAIN) {
        return false; // No frame available
    }

    if (rv != 0) {
        PluQ_SetError("Failed to receive: %s", nng_strerror(rv));
        return false;
    }

    // Store received data
    free(g_pluq.receivedData);
    g_pluq.receivedData = buf;
    g_pluq.receivedSize = size;

    // Parse FlatBuffers
    g_pluq.receivedFrame = PluQ_Frame_as_root(buf);

    // Update statistics
    double elapsed = (PluQ_GetTime() - start) * 1000.0; // ms
    g_pluq.stats.frames_received++;
    g_pluq.stats.bytes_received += size;
    g_pluq.stats.avg_frame_time_ms =
        (g_pluq.stats.avg_frame_time_ms * (g_pluq.stats.frames_received - 1) + elapsed) /
        g_pluq.stats.frames_received;
    if (elapsed > g_pluq.stats.max_frame_time_ms)
        g_pluq.stats.max_frame_time_ms = elapsed;
    if (elapsed < g_pluq.stats.min_frame_time_ms)
        g_pluq.stats.min_frame_time_ms = elapsed;

    return true;
}

bool PluQ_GetFrameInfo(uint32_t *sequence, double *timestamp)
{
    if (!g_pluq.receivedFrame)
        return false;

    if (sequence)
        *sequence = PluQ_Frame_sequence(g_pluq.receivedFrame);
    if (timestamp)
        *timestamp = PluQ_Frame_timestamp(g_pluq.receivedFrame);

    return true;
}

bool PluQ_GetPlayerState(
    vec3_t *origin, angles_t *angles, vec3_t *velocity,
    float *health, float *armor, int *weapon, int *ammo, int *flags)
{
    if (!g_pluq.receivedFrame)
        return false;

    PluQ_PlayerState_table_t player = PluQ_Frame_player(g_pluq.receivedFrame);
    if (!player)
        return false;

    if (origin) {
        PluQ_Vec3_struct_t o = PluQ_PlayerState_origin(player);
        origin->x = PluQ_Vec3_x(o);
        origin->y = PluQ_Vec3_y(o);
        origin->z = PluQ_Vec3_z(o);
    }

    if (angles) {
        PluQ_Angles_struct_t a = PluQ_PlayerState_angles(player);
        angles->pitch = PluQ_Angles_pitch(a);
        angles->yaw = PluQ_Angles_yaw(a);
        angles->roll = PluQ_Angles_roll(a);
    }

    if (velocity) {
        PluQ_Vec3_struct_t v = PluQ_PlayerState_velocity(player);
        velocity->x = PluQ_Vec3_x(v);
        velocity->y = PluQ_Vec3_y(v);
        velocity->z = PluQ_Vec3_z(v);
    }

    if (health) *health = PluQ_PlayerState_health(player);
    if (armor) *armor = PluQ_PlayerState_armor(player);
    if (weapon) *weapon = PluQ_PlayerState_weapon(player);
    if (ammo) *ammo = PluQ_PlayerState_ammo(player);
    if (flags) *flags = PluQ_PlayerState_flags(player);

    return true;
}

bool PluQ_GetGameState(
    bool *paused, bool *in_game, bool *intermission,
    char *mapname, size_t mapname_size,
    float *time, float *gravity, float *maxspeed)
{
    if (!g_pluq.receivedFrame)
        return false;

    PluQ_GameState_table_t game = PluQ_Frame_game(g_pluq.receivedFrame);
    if (!game)
        return false;

    if (paused) *paused = PluQ_GameState_paused(game);
    if (in_game) *in_game = PluQ_GameState_in_game(game);
    if (intermission) *intermission = PluQ_GameState_intermission(game);

    if (mapname) {
        flatbuffers_string_t name = PluQ_GameState_mapname(game);
        if (name) {
            strncpy(mapname, name, mapname_size - 1);
            mapname[mapname_size - 1] = '\0';
        }
    }

    if (time) *time = PluQ_GameState_time(game);
    if (gravity) *gravity = PluQ_GameState_gravity(game);
    if (maxspeed) *maxspeed = PluQ_GameState_maxspeed(game);

    return true;
}

int PluQ_GetEntityCount(void)
{
    if (!g_pluq.receivedFrame)
        return 0;

    PluQ_Entity_vec_t entities = PluQ_Frame_entities(g_pluq.receivedFrame);
    if (!entities)
        return 0;

    return (int)PluQ_Entity_vec_len(entities);
}

bool PluQ_GetEntity(
    int index, vec3_t *origin, angles_t *angles,
    int *model_id, int *skin, int *frame,
    int *effects, float *alpha, float *scale)
{
    if (!g_pluq.receivedFrame)
        return false;

    PluQ_Entity_vec_t entities = PluQ_Frame_entities(g_pluq.receivedFrame);
    if (!entities || index < 0 || index >= PluQ_Entity_vec_len(entities))
        return false;

    PluQ_Entity_table_t entity = PluQ_Entity_vec_at(entities, index);

    if (origin) {
        PluQ_Vec3_struct_t o = PluQ_Entity_origin(entity);
        origin->x = PluQ_Vec3_x(o);
        origin->y = PluQ_Vec3_y(o);
        origin->z = PluQ_Vec3_z(o);
    }

    if (angles) {
        PluQ_Angles_struct_t a = PluQ_Entity_angles(entity);
        angles->pitch = PluQ_Angles_pitch(a);
        angles->yaw = PluQ_Angles_yaw(a);
        angles->roll = PluQ_Angles_roll(a);
    }

    if (model_id) *model_id = PluQ_Entity_model_id(entity);
    if (skin) *skin = PluQ_Entity_skin(entity);
    if (frame) *frame = PluQ_Entity_frame(entity);
    if (effects) *effects = PluQ_Entity_effects(entity);
    if (alpha) *alpha = PluQ_Entity_alpha(entity);
    if (scale) *scale = PluQ_Entity_scale(entity);

    return true;
}

int PluQ_GetDLightCount(void)
{
    if (!g_pluq.receivedFrame)
        return 0;

    PluQ_DLight_vec_t dlights = PluQ_Frame_dlights(g_pluq.receivedFrame);
    if (!dlights)
        return 0;

    return (int)PluQ_DLight_vec_len(dlights);
}

bool PluQ_GetDLight(
    int index, vec3_t *origin, float *radius,
    float *color_r, float *color_g, float *color_b,
    float *decay, int *key)
{
    if (!g_pluq.receivedFrame)
        return false;

    PluQ_DLight_vec_t dlights = PluQ_Frame_dlights(g_pluq.receivedFrame);
    if (!dlights || index < 0 || index >= PluQ_DLight_vec_len(dlights))
        return false;

    PluQ_DLight_table_t dlight = PluQ_DLight_vec_at(dlights, index);

    if (origin) {
        PluQ_Vec3_struct_t o = PluQ_DLight_origin(dlight);
        origin->x = PluQ_Vec3_x(o);
        origin->y = PluQ_Vec3_y(o);
        origin->z = PluQ_Vec3_z(o);
    }

    if (radius) *radius = PluQ_DLight_radius(dlight);
    if (color_r) *color_r = PluQ_DLight_color_r(dlight);
    if (color_g) *color_g = PluQ_DLight_color_g(dlight);
    if (color_b) *color_b = PluQ_DLight_color_b(dlight);
    if (decay) *decay = PluQ_DLight_decay(dlight);
    if (key) *key = PluQ_DLight_key(dlight);

    return true;
}

// === Input Functions ===

bool PluQ_SendInput(
    uint32_t sequence, double timestamp,
    float forward_move, float side_move, float up_move,
    const angles_t *view_angles,
    uint32_t buttons, uint8_t impulse, const char *cmd_text)
{
    // TODO: Implement input sending (requires separate NNG socket or request/reply pattern)
    return false;
}

bool PluQ_ReceiveInput(
    uint32_t *sequence, double *timestamp,
    float *forward_move, float *side_move, float *up_move,
    angles_t *view_angles,
    uint32_t *buttons, uint8_t *impulse,
    char *cmd_text, size_t cmd_text_size)
{
    // TODO: Implement input receiving
    return false;
}

// === Statistics ===

void PluQ_GetStats(pluq_stats_t *stats)
{
    if (stats)
        *stats = g_pluq.stats;
}

void PluQ_ResetStats(void)
{
    memset(&g_pluq.stats, 0, sizeof(g_pluq.stats));
    g_pluq.stats.min_frame_time_ms = 999999.0;
}

const char *PluQ_GetLastError(void)
{
    return g_pluq.lastError;
}

// === QuakeSpasm Integration ===

static cvar_t pluq_enable = {"pluq_enable", "0", CVAR_ARCHIVE};
static cvar_t pluq_mode = {"pluq_mode", "backend", CVAR_ARCHIVE};  // backend, frontend, disabled
static cvar_t pluq_transport = {"pluq_transport", "tcp", CVAR_ARCHIVE};  // tcp, ipc, ws
static cvar_t pluq_address = {"pluq_address", "tcp://0.0.0.0:5555", CVAR_ARCHIVE};

/**
 * Console command: pluq_init
 * Initialize PluQ with current cvar settings
 */
static void PluQ_Init_f(void)
{
    if (!pluq_enable.value) {
        Con_Printf("PluQ is disabled. Set pluq_enable 1 to enable.\n");
        return;
    }

    // Parse mode
    pluq_mode_t mode = PLUQ_MODE_DISABLED;
    if (strcmp(pluq_mode.string, "backend") == 0) {
        mode = PLUQ_MODE_BACKEND;
    } else if (strcmp(pluq_mode.string, "frontend") == 0) {
        mode = PLUQ_MODE_FRONTEND;
    } else {
        Con_Printf("Invalid pluq_mode: %s (use 'backend' or 'frontend')\n", pluq_mode.string);
        return;
    }

    // Parse transport
    pluq_transport_t transport = PLUQ_TRANSPORT_TCP;
    if (strcmp(pluq_transport.string, "tcp") == 0) {
        transport = PLUQ_TRANSPORT_TCP;
    } else if (strcmp(pluq_transport.string, "ipc") == 0) {
        transport = PLUQ_TRANSPORT_IPC;
    } else if (strcmp(pluq_transport.string, "ws") == 0 || strcmp(pluq_transport.string, "websocket") == 0) {
        transport = PLUQ_TRANSPORT_WEBSOCKET;
    } else {
        Con_Printf("Invalid pluq_transport: %s (use 'tcp', 'ipc', or 'ws')\n", pluq_transport.string);
        return;
    }

    // Initialize
    pluq_config_t config = {
        .mode = mode,
        .transport = transport,
        .address = pluq_address.string,
        .non_blocking = true,
        .timeout_ms = 0
    };

    if (PluQ_Initialize(&config)) {
        Con_Printf("PluQ initialized: mode=%s transport=%s address=%s\n",
                   pluq_mode.string, pluq_transport.string, pluq_address.string);
    } else {
        Con_Printf("PluQ initialization failed: %s\n", PluQ_GetLastError());
    }
}

/**
 * Console command: pluq_shutdown
 * Shutdown PluQ
 */
static void PluQ_Shutdown_f(void)
{
    PluQ_Shutdown();
    Con_Printf("PluQ shutdown\n");
}

/**
 * Console command: pluq_stats
 * Show PluQ statistics
 */
static void PluQ_Stats_f(void)
{
    if (!PluQ_IsInitialized()) {
        Con_Printf("PluQ is not initialized\n");
        return;
    }

    pluq_stats_t stats;
    PluQ_GetStats(&stats);

    if (g_pluq.config.mode == PLUQ_MODE_BACKEND) {
        Con_Printf("PluQ Backend Statistics:\n");
        Con_Printf("  Frames sent: %llu\n", (unsigned long long)stats.frames_sent);
        Con_Printf("  Bytes sent: %llu (%.2f MB)\n",
                   (unsigned long long)stats.bytes_sent,
                   stats.bytes_sent / (1024.0 * 1024.0));
    } else {
        Con_Printf("PluQ Frontend Statistics:\n");
        Con_Printf("  Frames received: %llu\n", (unsigned long long)stats.frames_received);
        Con_Printf("  Bytes received: %llu (%.2f MB)\n",
                   (unsigned long long)stats.bytes_received,
                   stats.bytes_received / (1024.0 * 1024.0));
    }

    Con_Printf("  Avg frame time: %.2f ms\n", stats.avg_frame_time_ms);
    Con_Printf("  Min frame time: %.2f ms\n", stats.min_frame_time_ms);
    Con_Printf("  Max frame time: %.2f ms\n", stats.max_frame_time_ms);
}

/**
 * Initialize PluQ subsystem (called from Host_Init)
 * Registers console commands and cvars
 */
void PluQ_Init(void)
{
    Cvar_RegisterVariable(&pluq_enable);
    Cvar_RegisterVariable(&pluq_mode);
    Cvar_RegisterVariable(&pluq_transport);
    Cvar_RegisterVariable(&pluq_address);

    Cmd_AddCommand("pluq_init", PluQ_Init_f);
    Cmd_AddCommand("pluq_shutdown", PluQ_Shutdown_f);
    Cmd_AddCommand("pluq_stats", PluQ_Stats_f);

    Con_Printf("PluQ IPC system initialized\n");
    Con_Printf("Use 'pluq_init' to start broadcasting/receiving\n");
    Con_Printf("Cvars: pluq_enable, pluq_mode, pluq_transport, pluq_address\n");
}
