# PLQ Naming Conventions & Planning Guide

This document establishes naming conventions for PLQ development, modeled after vanilla Quake source patterns.

---

## Table of Contents

1. [Prefix Standard](#prefix-standard)
2. [File Naming](#file-naming)
3. [Function Naming](#function-naming)
4. [Type Naming](#type-naming)
5. [Macro & Constant Naming](#macro--constant-naming)
6. [Variable Naming](#variable-naming)
7. [Planned Files & Functions](#planned-files--functions)
8. [FlatBuffers Schema Naming](#flatbuffers-schema-naming)
9. [Quick Reference](#quick-reference)

---

## Prefix Standard

### Primary Prefix: `PLQ`

Following Quake's pattern of short uppercase prefixes (`CL_`, `SV_`, `R_`, `S_`, `NET_`):

| Context | Prefix | Example |
|---------|--------|---------|
| Functions | `PLQ_` | `PLQ_Init()` |
| Files | `plq_` | `plq_backend.c` |
| Macros/Defines | `PLQ_` | `PLQ_MAX_ENTITIES` |
| Types | `plq_` | `plq_state_t` |
| Globals | `plq_` | `plq_state` |

### Subsystem Suffixes (Quake-style)

Like Quake uses `CL_` for client and `SV_` for server, PLQ uses component suffixes:

| Prefix | Component | Quake Equivalent |
|--------|-----------|------------------|
| `PLQ_` | Core/shared | `COM_` |
| `PLQ_BE_` | Backend (engine) | `SV_` |
| `PLQ_FE_` | Frontend (renderer) | `CL_` |
| `PLQ_Res_` | Resources | `Mod_` |
| `PLQ_Msg_` | Messages | `MSG_` |

---

## File Naming

### Pattern: `plq_<component>.c` / `plq_<component>.h`

Following Quake's pattern: `cl_main.c`, `sv_main.c`, `net_main.c`

### Core Files

| File | Purpose | Quake Equivalent |
|------|---------|------------------|
| `plq.h` | Core definitions, public API | `quakedef.h` |
| `plq.c` | Core implementation | `common.c` |
| `plq.fbs` | FlatBuffers schema | - |

### Backend Files (Engine Side)

| File | Purpose | Quake Equivalent |
|------|---------|------------------|
| `plq_be_main.c` | Backend init, shutdown, frame | `sv_main.c` |
| `plq_be_send.c` | Send world state to frontend | `sv_send.c` |
| `plq_be_user.c` | Process frontend input | `sv_user.c` |

### Frontend Files (Renderer Side)

| File | Purpose | Quake Equivalent |
|------|---------|------------------|
| `plq_fe_main.c` | Frontend init, shutdown, frame | `cl_main.c` |
| `plq_fe_parse.c` | Parse backend messages | `cl_parse.c` |
| `plq_fe_input.c` | Send input to backend | `cl_input.c` |

### Shared/Utility Files

| File | Purpose | Quake Equivalent |
|------|---------|------------------|
| `plq_msg.c` | Message construction/parsing | `net_main.c` |
| `plq_res.c` | Resource streaming | `mod_*.c` |

### Generated Files (FlatBuffers)

| File | Purpose |
|------|---------|
| `plq_builder.h` | Message building |
| `plq_reader.h` | Message reading |
| `plq_verifier.h` | Message verification |

---

## Function Naming

### Pattern: `PLQ_<Component>_<Action>`

Following Quake: `CL_SendCmd`, `SV_RunClients`, `NET_SendMessage`

### Core Functions

```c
void PLQ_Init(void);
void PLQ_Shutdown(void);
```

### Backend Functions (`PLQ_BE_`)

```c
// Lifecycle (like SV_Init, SV_Shutdown)
void PLQ_BE_Init(void);
void PLQ_BE_Shutdown(void);
void PLQ_BE_Frame(void);           // Per-frame processing

// Sending (like SV_SendClientMessages)
void PLQ_BE_SendWorldState(void);
void PLQ_BE_SendMapChange(const char *mapname);
void PLQ_BE_SendDisconnect(void);

// Receiving (like SV_ReadClientMessage)
void PLQ_BE_ReadInput(void);
void PLQ_BE_ProcessCommands(void);

// Resources
void PLQ_BE_SendResource(int type, int index);
void PLQ_BE_ProcessResourceRequests(void);
```

### Frontend Functions (`PLQ_FE_`)

```c
// Lifecycle (like CL_Init, CL_Disconnect)
qboolean PLQ_FE_Init(void);
void PLQ_FE_Shutdown(void);
void PLQ_FE_Frame(void);           // Per-frame processing

// Receiving (like CL_ParseServerMessage)
qboolean PLQ_FE_ReadWorldState(void);
void PLQ_FE_ParseWorldState(void);
void PLQ_FE_ParseMapChange(void);

// Sending (like CL_SendCmd)
void PLQ_FE_SendInput(usercmd_t *cmd);
void PLQ_FE_SendCommand(const char *text);

// Resources (like CL_RequestModel)
qboolean PLQ_FE_RequestResource(int type, int index, const char *name);
void *PLQ_FE_GetResource(int type, int index);
```

### Message Functions (`PLQ_Msg_`)

```c
// Like MSG_WriteXxx / MSG_ReadXxx
void PLQ_Msg_Init(void);
void PLQ_Msg_BeginWrite(void);
void PLQ_Msg_WriteEntity(entity_t *ent);
void PLQ_Msg_WriteStats(int *stats);
void *PLQ_Msg_EndWrite(size_t *size);

qboolean PLQ_Msg_BeginRead(const void *buf, size_t size);
int PLQ_Msg_ReadType(void);
void PLQ_Msg_ReadEntity(entity_t *ent);
```

### Resource Functions (`PLQ_Res_`)

```c
// Like Mod_LoadModel, W_GetLumpinfo
void PLQ_Res_Init(void);
void PLQ_Res_Shutdown(void);
void PLQ_Res_ClearCache(void);

void PLQ_Res_PackTexture(gltexture_t *tex, void **data, size_t *size);
void PLQ_Res_PackModel(qmodel_t *mod, void **data, size_t *size);

void PLQ_Res_UnpackTexture(const void *data, size_t size, gltexture_t *tex);
void PLQ_Res_UnpackModel(const void *data, size_t size, qmodel_t *mod);
```

---

## Type Naming

### Pattern: `plq_<name>_t`

Following Quake: `usercmd_t`, `entity_t`, `client_t`, `server_t`

### Enumerations

```c
// Connection state (like cactive_t)
typedef enum {
    plq_disconnected,
    plq_connecting,
    plq_connected,
    plq_active
} plq_state_t;

// Resource types
typedef enum {
    plq_res_none,
    plq_res_texture,
    plq_res_model,
    plq_res_lightmap,
    plq_res_sound
} plq_restype_t;

// Message types
typedef enum {
    plq_msg_none,
    plq_msg_frame,
    plq_msg_mapchange,
    plq_msg_disconnect,
    plq_msg_input,
    plq_msg_command
} plq_msgtype_t;
```

### Structures

```c
// Statistics (like net stats)
typedef struct {
    uint64_t    sent_messages;
    uint64_t    recv_messages;
    uint64_t    sent_bytes;
    uint64_t    recv_bytes;
} plq_stats_t;

// Backend state (like server_t)
typedef struct {
    qboolean    active;
    plq_state_t state;
    double      time;
    char        mapname[MAX_QPATH];
} plq_backend_t;

// Frontend state (like client_state_t)
typedef struct {
    plq_state_t state;
    double      time;
    char        mapname[MAX_QPATH];
    int         stats[MAX_CL_STATS];
    vec3_t      vieworg;
    vec3_t      viewangles;
} plq_frontend_t;

// Resource request
typedef struct {
    plq_restype_t   type;
    int             index;
    char            name[MAX_QPATH];
} plq_resreq_t;
```

---

## Macro & Constant Naming

### Pattern: `PLQ_<NAME>`

Following Quake: `MAX_EDICTS`, `MAX_MODELS`, `PROTOCOL_VERSION`

### Protocol

```c
#define PLQ_PROTOCOL_VERSION    1

// IPC endpoints
#define PLQ_PORT_WORLD          9001
#define PLQ_PORT_INPUT          9002
#define PLQ_PORT_RESOURCES      9003

#define PLQ_URL_WORLD           "tcp://127.0.0.1:9001"
#define PLQ_URL_INPUT           "tcp://127.0.0.1:9002"
#define PLQ_URL_RESOURCES       "tcp://127.0.0.1:9003"
```

### Limits

```c
#define PLQ_MAX_ENTITIES        8192
#define PLQ_MAX_MODELS          4096
#define PLQ_MAX_TEXTURES        4096
#define PLQ_MAX_LIGHTMAPS       512
#define PLQ_MAX_MSGSIZE         (16 * 1024 * 1024)  // 16 MB
```

### Timeouts (milliseconds)

```c
#define PLQ_TIMEOUT_SEND        100
#define PLQ_TIMEOUT_RECV        100
#define PLQ_TIMEOUT_CONNECT     5000
```

### Message Types (if not using FlatBuffers union)

```c
#define PLQ_MSG_FRAME           1
#define PLQ_MSG_MAPCHANGE       2
#define PLQ_MSG_DISCONNECT      3
#define PLQ_MSG_INPUT           4
#define PLQ_MSG_COMMAND         5
#define PLQ_MSG_RESOURCE_REQ    6
#define PLQ_MSG_RESOURCE_RESP   7
```

### Error Codes

```c
#define PLQ_OK                  0
#define PLQ_ERR_TIMEOUT         -1
#define PLQ_ERR_DISCONNECT      -2
#define PLQ_ERR_INVALID         -3
#define PLQ_ERR_NOMEM           -4
#define PLQ_ERR_NOTFOUND        -5
```

---

## Variable Naming

### Global State (like `sv`, `cl`, `cls`)

```c
plq_backend_t   plq_be;         // Backend state
plq_frontend_t  plq_fe;         // Frontend state
plq_stats_t     plq_stats;      // Statistics
```

### Console Variables (like `sv_maxspeed`, `cl_name`)

```c
cvar_t  plq_enabled;            // "plq_enabled" "1"
cvar_t  plq_debug;              // "plq_debug" "0"
cvar_t  plq_timeout;            // "plq_timeout" "100"
```

### Local Variables

Standard Quake style - short, descriptive:

```c
void PLQ_BE_SendWorldState(void)
{
    int         i, num;
    entity_t   *ent;
    size_t      size;
    void       *buf;
}
```

---

## Planned Files & Functions

### File Structure

```
Quake/
├── plq.h                   # Core definitions, public API
├── plq.c                   # Core implementation, cvars
├── plq.fbs                 # FlatBuffers schema
│
├── plq_be_main.c           # Backend lifecycle
├── plq_be_send.c           # Backend -> Frontend messages
├── plq_be_user.c           # Frontend -> Backend input
│
├── plq_fe_main.c           # Frontend lifecycle
├── plq_fe_parse.c          # Parse backend messages
├── plq_fe_input.c          # Send input to backend
│
├── plq_msg.c               # Message utilities
├── plq_res.c               # Resource packing/unpacking
│
├── plq_builder.h           # Generated FlatBuffers
├── plq_reader.h            # Generated FlatBuffers
└── plq_verifier.h          # Generated FlatBuffers
```

### Function Summary

```c
// ═══════════════════════════════════════════════════════════════
// plq.c - Core
// ═══════════════════════════════════════════════════════════════
void PLQ_Init(void);
void PLQ_Shutdown(void);
void PLQ_Error(const char *fmt, ...);       // Like Sys_Error
void PLQ_Printf(const char *fmt, ...);      // Like Con_Printf
void PLQ_DPrintf(const char *fmt, ...);     // Like Con_DPrintf

// ═══════════════════════════════════════════════════════════════
// plq_be_main.c - Backend Lifecycle
// ═══════════════════════════════════════════════════════════════
void PLQ_BE_Init(void);
void PLQ_BE_Shutdown(void);
void PLQ_BE_Frame(void);

// ═══════════════════════════════════════════════════════════════
// plq_be_send.c - Backend Sending
// ═══════════════════════════════════════════════════════════════
void PLQ_BE_SendWorldState(void);
void PLQ_BE_SendMapChange(const char *mapname);
void PLQ_BE_SendDisconnect(void);
void PLQ_BE_SendResource(plq_restype_t type, int index, const char *name);

// ═══════════════════════════════════════════════════════════════
// plq_be_user.c - Backend Receiving
// ═══════════════════════════════════════════════════════════════
void PLQ_BE_ReadInput(void);
void PLQ_BE_ProcessResourceRequests(void);
void PLQ_BE_ApplyMove(usercmd_t *cmd);
void PLQ_BE_ApplyViewAngles(vec3_t angles);
void PLQ_BE_ExecuteCommand(const char *cmd);

// ═══════════════════════════════════════════════════════════════
// plq_fe_main.c - Frontend Lifecycle
// ═══════════════════════════════════════════════════════════════
qboolean PLQ_FE_Init(void);
void PLQ_FE_Shutdown(void);
void PLQ_FE_Frame(void);

// ═══════════════════════════════════════════════════════════════
// plq_fe_parse.c - Frontend Receiving
// ═══════════════════════════════════════════════════════════════
qboolean PLQ_FE_ReadMessage(void);
void PLQ_FE_ParseFrame(void);
void PLQ_FE_ParseMapChange(void);
void PLQ_FE_ParseDisconnect(void);
void PLQ_FE_ParseResource(void);

// ═══════════════════════════════════════════════════════════════
// plq_fe_input.c - Frontend Sending
// ═══════════════════════════════════════════════════════════════
void PLQ_FE_SendMove(usercmd_t *cmd);
void PLQ_FE_SendViewAngles(vec3_t angles);
void PLQ_FE_SendCommand(const char *text);
void PLQ_FE_RequestResource(plq_restype_t type, int index, const char *name);

// ═══════════════════════════════════════════════════════════════
// plq_msg.c - Message Utilities
// ═══════════════════════════════════════════════════════════════
void PLQ_Msg_Init(void);
void PLQ_Msg_Shutdown(void);

// Building (uses FlatBuffers internally)
void *PLQ_Msg_BuildFrame(size_t *size);
void *PLQ_Msg_BuildMapChange(const char *mapname, size_t *size);
void *PLQ_Msg_BuildInput(usercmd_t *cmd, vec3_t angles, size_t *size);
void *PLQ_Msg_BuildResourceReq(plq_restype_t type, int idx, const char *name, size_t *size);

// Parsing
plq_msgtype_t PLQ_Msg_GetType(const void *buf, size_t size);
qboolean PLQ_Msg_Verify(const void *buf, size_t size);

// ═══════════════════════════════════════════════════════════════
// plq_res.c - Resource Streaming
// ═══════════════════════════════════════════════════════════════
void PLQ_Res_Init(void);
void PLQ_Res_Shutdown(void);
void PLQ_Res_ClearCache(void);

// Backend: Pack resources for sending
void *PLQ_Res_PackTexture(gltexture_t *tex, size_t *size);
void *PLQ_Res_PackModel(qmodel_t *mod, size_t *size);
void *PLQ_Res_PackLightmap(int index, size_t *size);

// Frontend: Unpack and cache resources
qboolean PLQ_Res_UnpackTexture(const void *data, size_t size);
qboolean PLQ_Res_UnpackModel(const void *data, size_t size);
qboolean PLQ_Res_UnpackLightmap(const void *data, size_t size);

// Frontend: Get cached resources
gltexture_t *PLQ_Res_GetTexture(int index);
qmodel_t *PLQ_Res_GetModel(int index);
```

---

## FlatBuffers Schema Naming

### File: `plq.fbs`

### Namespace

```flatbuffers
namespace PLQ;
```

### Tables (PascalCase)

```flatbuffers
table Entity {
    num:int;
    model:int;
    frame:int;
    skin:int;
    origin:Vec3;
    angles:Vec3;
    effects:int;
    alpha:float;
    scale:float;
}

table Frame {
    time:double;
    vieworg:Vec3;
    viewangles:Vec3;
    entities:[Entity];
    stats:[int];
}

table MapChange {
    name:string;
}

table Input {
    forwardmove:float;
    sidemove:float;
    upmove:float;
    angles:Vec3;
    buttons:int;
    impulse:int;
}

table Command {
    text:string;
}

table ResourceReq {
    type:int;
    index:int;
    name:string;
}

table Texture {
    index:int;
    name:string;
    width:int;
    height:int;
    data:[ubyte];
}

table Model {
    index:int;
    name:string;
    data:[ubyte];
}
```

### Enums (PascalCase)

```flatbuffers
enum MsgType:byte {
    None = 0,
    Frame = 1,
    MapChange = 2,
    Disconnect = 3,
    Input = 4,
    Command = 5,
    ResourceReq = 6,
    ResourceResp = 7
}

enum ResType:byte {
    None = 0,
    Texture = 1,
    Model = 2,
    Lightmap = 3,
    Sound = 4
}
```

### Unions

```flatbuffers
union Payload {
    Frame,
    MapChange,
    Input,
    Command,
    ResourceReq,
    Texture,
    Model
}

table Message {
    type:MsgType;
    payload:Payload;
}

root_type Message;
```

---

## Quick Reference

```
┌────────────────────────────────────────────────────────────────┐
│                    PLQ NAMING CHEAT SHEET                      │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  FILES         plq_<component>.c        plq_be_main.c          │
│                                         plq_fe_parse.c         │
│                                                                │
│  FUNCTIONS     PLQ_<Component>_<Action> PLQ_BE_Init()          │
│                                         PLQ_FE_SendMove()      │
│                                         PLQ_Msg_BuildFrame()   │
│                                                                │
│  TYPES         plq_<name>_t             plq_state_t            │
│                                         plq_backend_t          │
│                                                                │
│  MACROS        PLQ_<NAME>               PLQ_MAX_ENTITIES       │
│                                         PLQ_TIMEOUT_SEND       │
│                                                                │
│  GLOBALS       plq_<name>               plq_be, plq_fe         │
│                                         plq_stats              │
│                                                                │
│  CVARS         plq_<name>               plq_enabled            │
│                                         plq_debug              │
│                                                                │
├────────────────────────────────────────────────────────────────┤
│  COMPONENT PREFIXES (like CL_, SV_ in Quake)                   │
│                                                                │
│  PLQ_BE_       Backend (engine side)    PLQ_BE_SendWorldState  │
│  PLQ_FE_       Frontend (renderer)      PLQ_FE_ParseFrame      │
│  PLQ_Msg_      Message utilities        PLQ_Msg_BuildFrame     │
│  PLQ_Res_      Resource streaming       PLQ_Res_PackTexture    │
│                                                                │
├────────────────────────────────────────────────────────────────┤
│  FLATBUFFERS                                                   │
│                                                                │
│  Namespace     PLQ                                             │
│  Tables        PascalCase               Entity, Frame, Input   │
│  Enums         PascalCase               MsgType, ResType       │
│  Fields        lowercase                num, model, origin     │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

---

## Comparison with Quake Conventions

| Quake | PLQ | Purpose |
|-------|-----|---------|
| `sv_main.c` | `plq_be_main.c` | Server/Backend main |
| `cl_main.c` | `plq_fe_main.c` | Client/Frontend main |
| `cl_parse.c` | `plq_fe_parse.c` | Parse incoming messages |
| `cl_input.c` | `plq_fe_input.c` | Handle/send input |
| `sv_send.c` | `plq_be_send.c` | Send state to clients |
| `sv_user.c` | `plq_be_user.c` | Process client input |
| `SV_Init()` | `PLQ_BE_Init()` | Initialize server/backend |
| `CL_SendCmd()` | `PLQ_FE_SendMove()` | Send movement command |
| `CL_ParseServerMessage()` | `PLQ_FE_ParseFrame()` | Parse server state |
| `server_t sv` | `plq_backend_t plq_be` | Global server state |
| `client_state_t cl` | `plq_frontend_t plq_fe` | Global client state |
