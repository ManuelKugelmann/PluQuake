# Quake Source Code Architectural Guide

This document provides an architectural overview of the Quake engine source code as implemented in PluQuake (based on Ironwail/QuakeSpasm).

---

## Table of Contents

1. [File Organization](#file-organization)
2. [Naming Conventions](#naming-conventions)
3. [System Overview](#system-overview)
4. [Major Data Structures](#major-data-structures)
5. [Initialization Flow](#initialization-flow)
6. [Game Loop Flow](#game-loop-flow)
7. [Subsystem Details](#subsystem-details)
8. [Architecture Patterns](#architecture-patterns)

---

## File Organization

The Quake engine organizes code into functional subsystems using consistent file prefixes:

### File Prefix Categories

| Prefix | Subsystem | Key Files |
|--------|-----------|-----------|
| `gl_*` | OpenGL Rendering | `gl_rmain.c`, `gl_draw.c`, `gl_texmgr.c`, `gl_mesh.c` |
| `r_*` | Rendering Logic | `r_alias.c`, `r_brush.c`, `r_sprite.c`, `r_world.c` |
| `cl_*` | Client | `cl_main.c`, `cl_input.c`, `cl_parse.c`, `cl_demo.c` |
| `sv_*` | Server | `sv_main.c`, `sv_phys.c`, `sv_move.c`, `sv_user.c` |
| `snd_*` | Sound Engine | `snd_dma.c`, `snd_mix.c`, `snd_sdl.c` |
| `net_*` | Networking | `net_main.c`, `net_dgrm.c`, `net_udp.c` |
| `pr_*` | QuakeC VM | `pr_exec.c`, `pr_edict.c`, `pr_cmds.c` |
| `in_*` | Input | `in_sdl.c` |
| `pl_*` | Platform Layer | `pl_linux.c`, `pl_win.c` |
| `sys_*` | System Interface | `sys_sdl_unix.c`, `sys_sdl_win.c` |
| `pluq_*` | PluQ IPC | `pluq_backend.c`, `pluq_frontend.c` |

### Core Infrastructure Files

```
host.c          - Engine core, main loop, subsystem coordination
main_sdl.c      - Entry point, SDL initialization
common.c        - File I/O, parsing, utilities
zone.c          - Memory management (Hunk, Zone, Cache)
cmd.c           - Console command system
cvar.c          - Console variable system
console.c       - Console interface
menu.c          - Menu system
keys.c          - Keyboard mapping
view.c          - View/camera calculations
world.c         - BSP collision detection
```

---

## Naming Conventions

### Function Prefixes

The engine uses a strict prefix convention to identify which subsystem a function belongs to:

| Prefix | Subsystem | Examples |
|--------|-----------|----------|
| `R_` | Rendering | `R_Init()`, `R_RenderView()`, `R_DrawEntities()` |
| `GL_` | OpenGL API | `GL_Init()`, `GL_Bind()`, `GL_Upload32()` |
| `V_` | View/Camera | `V_Init()`, `V_CalcBlend()`, `V_RenderView()` |
| `CL_` | Client | `CL_Init()`, `CL_SendCmd()`, `CL_ParseServerInfo()` |
| `SV_` | Server | `SV_Init()`, `SV_RunClients()`, `SV_Physics()` |
| `S_` | Sound | `S_Init()`, `S_Update()`, `S_StartSound()` |
| `BGM_` | Background Music | `BGM_Init()`, `BGM_Update()`, `BGM_Play()` |
| `NET_` | Networking | `NET_Init()`, `NET_SendMessage()`, `NET_Poll()` |
| `Con_` | Console | `Con_Printf()`, `Con_Init()`, `Con_DrawConsole()` |
| `Cmd_` | Commands | `Cmd_Init()`, `Cmd_AddCommand()`, `Cmd_ExecuteString()` |
| `Cvar_` | Console Vars | `Cvar_Init()`, `Cvar_Set()`, `Cvar_RegisterVariable()` |
| `COM_` | Common/File I/O | `COM_Init()`, `COM_LoadFile()`, `COM_Parse()` |
| `Key_` | Keyboard | `Key_Init()`, `Key_Event()`, `Key_GetBinding()` |
| `IN_` | Input | `IN_Init()`, `IN_Commands()`, `IN_Move()` |
| `PR_` | QuakeC VM | `PR_Init()`, `PR_ExecuteProgram()`, `PR_RunError()` |
| `Host_` | Engine Core | `Host_Init()`, `Host_Frame()`, `Host_ServerFrame()` |
| `VID_` | Video | `VID_Init()`, `VID_SetMode()`, `VID_Shutdown()` |
| `SCR_` | Screen | `SCR_Init()`, `SCR_UpdateScreen()`, `SCR_DrawLoading()` |
| `Draw_` | 2D Drawing | `Draw_Init()`, `Draw_String()`, `Draw_Pic()` |
| `Mod_` | Model Loading | `Mod_Init()`, `Mod_LoadModel()`, `Mod_ForName()` |
| `PluQ_` | IPC System | `PluQ_Backend_Init()`, `PluQ_BroadcastWorldState()` |
| `Sys_` | System | `Sys_Init()`, `Sys_Error()`, `Sys_DoubleTime()` |
| `Hunk_` | Hunk Memory | `Hunk_Alloc()`, `Hunk_LowMark()`, `Hunk_FreeToLowMark()` |
| `Zone_` | Zone Memory | `Z_Malloc()`, `Z_Free()`, `Z_Realloc()` |
| `Cache_` | Cache Memory | `Cache_Alloc()`, `Cache_Free()`, `Cache_Check()` |

### Function Suffix Patterns

| Suffix | Meaning | Examples |
|--------|---------|----------|
| `_Init()` | Initialize subsystem | `R_Init()`, `S_Init()` |
| `_Shutdown()` | Clean up subsystem | `VID_Shutdown()`, `NET_Shutdown()` |
| `_Update()` | Per-frame update | `S_Update()`, `BGM_Update()` |
| `_Draw()` | Rendering operation | `SCR_DrawConsole()`, `Sbar_Draw()` |
| `_Parse*()` | Network message parsing | `CL_ParseServerInfo()`, `CL_ParseUpdate()` |
| `_f()` | Console command handler | `Host_Quit_f()`, `Cmd_Exec_f()` |
| `_c()` | Cvar callback | `R_Fullbright_c()` |

### Variable Naming

- **Global state structs**: Lowercase abbreviations (`sv`, `cl`, `cls`, `host`)
- **Console variables**: `cvar_t` type, lowercase with underscores (`sv_cheats`, `cl_name`)
- **Constants/Defines**: `UPPERCASE_WITH_UNDERSCORES`
- **Local variables**: `lowercase_with_underscores` or `camelCase`

---

## System Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                          QUAKE ENGINE                               │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌────────────┐ │
│  │   CLIENT    │  │   SERVER    │  │  RENDERER   │  │   SOUND    │ │
│  │  (cl_*.c)   │  │  (sv_*.c)   │  │ (gl_*/r_*) │  │  (snd_*)   │ │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └─────┬──────┘ │
│         │                │                │                │        │
│         └────────┬───────┴────────┬───────┴────────────────┘        │
│                  │                │                                  │
│           ┌──────┴──────┐  ┌──────┴──────┐                          │
│           │    HOST     │  │  NETWORKING │                          │
│           │  (host.c)   │  │  (net_*.c)  │                          │
│           └──────┬──────┘  └─────────────┘                          │
│                  │                                                   │
│  ┌───────────────┼───────────────────────────────────────────────┐  │
│  │               │         CORE SYSTEMS                          │  │
│  │  ┌────────┐ ┌─┴────────┐ ┌──────────┐ ┌──────────┐ ┌────────┐ │  │
│  │  │ MEMORY │ │ COMMANDS │ │  CVARS   │ │ FILE I/O │ │QUAKEC  │ │  │
│  │  │(zone.c)│ │ (cmd.c)  │ │(cvar.c)  │ │(common.c)│ │(pr_*.c)│ │  │
│  │  └────────┘ └──────────┘ └──────────┘ └──────────┘ └────────┘ │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │                    PLATFORM LAYER (SDL)                       │  │
│  │  main_sdl.c  │  sys_sdl_*.c  │  in_sdl.c  │  snd_sdl.c       │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Subsystem Dependency Graph

```
                    ┌──────────────┐
                    │  main_sdl.c  │
                    │ (Entry Point)│
                    └──────┬───────┘
                           │
                           ▼
                    ┌──────────────┐
                    │   host.c     │
                    │ (Main Loop)  │
                    └──────┬───────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
┌───────────────┐  ┌───────────────┐  ┌───────────────┐
│    CLIENT     │  │    SERVER     │  │   RENDERER    │
│   cl_main.c   │  │   sv_main.c   │  │  gl_rmain.c   │
└───────┬───────┘  └───────┬───────┘  └───────┬───────┘
        │                  │                  │
        │                  │                  │
        │                  ▼                  │
        │          ┌───────────────┐          │
        │          │   QUAKEC VM   │          │
        │          │   pr_exec.c   │          │
        │          └───────┬───────┘          │
        │                  │                  │
        ▼                  ▼                  ▼
┌─────────────────────────────────────────────────────┐
│                   NETWORKING                        │
│                   net_main.c                        │
└─────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────┐
│              CORE INFRASTRUCTURE                    │
│  zone.c  │  cmd.c  │  cvar.c  │  common.c          │
└─────────────────────────────────────────────────────┘
```

### Client-Server Model

```
┌─────────────────────────────────────────────────────────────────┐
│                    SINGLE PLAYER / LISTEN SERVER                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────┐    ┌──────────────────────┐          │
│  │       CLIENT         │    │        SERVER        │          │
│  │                      │    │                      │          │
│  │  - Input handling    │◄──►│  - Physics (sv_phys) │          │
│  │  - View rendering    │    │  - QuakeC execution  │          │
│  │  - Sound playback    │    │  - Entity management │          │
│  │  - Prediction        │    │  - World simulation  │          │
│  │                      │    │                      │          │
│  │   cl_main.c         │    │   sv_main.c          │          │
│  │   cl_input.c        │    │   sv_user.c          │          │
│  │   cl_parse.c        │    │   sv_move.c          │          │
│  └──────────────────────┘    └──────────────────────┘          │
│            │                           │                        │
│            └─────────┬─────────────────┘                        │
│                      │                                          │
│              ┌───────┴───────┐                                  │
│              │   LOOPBACK    │                                  │
│              │  (net_loop.c) │                                  │
│              └───────────────┘                                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                    MULTIPLAYER (REMOTE)                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐                    ┌─────────────────┐    │
│  │     CLIENT      │                    │     SERVER      │    │
│  │                 │   ┌───────────┐    │                 │    │
│  │                 │◄─►│    UDP    │◄──►│                 │    │
│  │                 │   │(net_udp.c)│    │                 │    │
│  │                 │   └───────────┘    │                 │    │
│  └─────────────────┘                    └─────────────────┘    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Major Data Structures

### Client State (`client.h`)

```c
// client_state_t (cl) - Dynamic per-connection state
struct client_state_t {
    // Movement
    usercmd_t   cmd;              // Current input command
    vec3_t      viewangles;       // View direction

    // Game state
    int         stats[MAX_CL_STATS]; // Health, ammo, armor, etc.
    int         items;            // Inventory flags

    // Entities
    entity_t    *entities;        // Client-side entity array
    int         num_entities;

    // Level data
    char        levelname[128];   // Current map name
    qmodel_t    *worldmodel;      // BSP model

    // Interpolation
    float       mtime[2];         // Server times for lerping
    float       ctime;            // Client time
};

// client_static_t (cls) - Persistent across connections
struct client_static_t {
    cactive_t   state;            // ca_disconnected, ca_connected
    qsocket_t   *netcon;          // Network connection

    // Demo state
    qboolean    demoplayback;
    qboolean    demorecording;
    FILE        *demofile;

    sizebuf_t   message;          // Outgoing message buffer
};
```

### Server State (`server.h`)

```c
// server_t (sv) - Server state for current level
struct server_t {
    qboolean    active;           // Server running?
    char        name[64];         // Map name

    // World
    qmodel_t    *worldmodel;
    char        *model_precache[MAX_MODELS];
    char        *sound_precache[MAX_SOUNDS];

    // Network
    sizebuf_t   signon;           // Signon data for new clients
    unsigned    protocol;         // Protocol version
};

// server_static_t (svs) - Persistent server data
struct server_static_t {
    client_t    clients[MAX_SCOREBOARD];
    int         maxclients;
    int         serverflags;      // Episode completion flags
};

// client_t - Per-client connection (server-side)
struct client_t {
    qboolean    active;
    qboolean    spawned;
    char        name[32];
    qsocket_t   *netconnection;
    edict_t     *edict;           // Player entity
    usercmd_t   cmd;              // Last movement command
    sizebuf_t   message;          // Reliable message buffer
};
```

### Entity Types

```c
// entity_t (render.h) - Client-side renderable entity
struct entity_t {
    vec3_t      origin;           // Current position
    vec3_t      angles;           // Current angles
    qmodel_t    *model;           // Model to render
    int         frame;            // Animation frame
    float       alpha;            // Transparency

    // Interpolation
    vec3_t      msg_origins[2];   // Position history
    vec3_t      msg_angles[2];    // Angle history
    float       lerp_start;       // Interpolation start time
};

// edict_t (progs.h) - Server-side game entity (QuakeC)
struct edict_t {
    qboolean    free;             // Available for reuse?
    link_t      area;             // World linkage

    // Entity state
    entvars_t   v;                // QuakeC fields (origin, velocity, etc.)

    // Collision
    float       freetime;         // When entity was freed
};
```

### Model Structure (`gl_model.h`)

```c
// qmodel_t - Loaded model (BSP, MDL, or SPR)
struct qmodel_t {
    char        name[MAX_QPATH];
    modtype_t   type;             // mod_brush, mod_sprite, mod_alias

    // Bounds
    vec3_t      mins, maxs;
    float       radius;

    // Type-specific data
    union {
        brushmodel_t    brush;    // BSP world/submodels
        aliasmodel_t    alias;    // MDL animated models
        spritemodel_t   sprite;   // SPR sprites
    };
};
```

---

## Initialization Flow

### Startup Sequence

```
main() [main_sdl.c]
│
├─► Parse command line (COM_InitArgv)
│
├─► Initialize SDL (Sys_InitSDL)
│   └─► SDL_Init(SDL_INIT_TIMER | SDL_INIT_EVENTS)
│
├─► Platform initialization (Sys_Init)
│
├─► Allocate engine memory heap
│   └─► malloc(parms.memsize)  // Default ~256MB
│
├─► Host_Init() [host.c]
│   │
│   ├─► Memory_Init()              // Initialize Hunk/Zone/Cache
│   ├─► AsyncQueue_Init()          // PluQ async operations
│   ├─► Cbuf_Init()                // Command buffer
│   ├─► Cmd_Init()                 // Command system
│   ├─► LOG_Init()                 // Logging
│   ├─► Cvar_Init()                // Console variables
│   ├─► COM_Init()                 // Common utilities
│   ├─► COM_InitFilesystem()       // Mount PAK files
│   ├─► Host_InitLocal()           // Host cvars
│   ├─► W_LoadWadFile()            // gfx.wad (sprites, fonts)
│   │
│   ├─► [IF NOT DEDICATED]:
│   │   ├─► Key_Init()             // Keyboard bindings
│   │   ├─► Con_Init()             // Console
│   │   ├─► V_Init()               // View system
│   │   ├─► Chase_Init()           // Chase camera
│   │   └─► M_Init()               // Menu system
│   │
│   ├─► [IF NOT HEADLESS]:
│   │   ├─► VID_Init()             // OpenGL context
│   │   ├─► IN_Init()              // Input devices
│   │   ├─► TexMgr_Init()          // Texture manager
│   │   ├─► Draw_Init()            // 2D drawing
│   │   ├─► SCR_Init()             // Screen
│   │   ├─► R_Init()               // 3D Renderer
│   │   ├─► S_Init()               // Sound engine
│   │   ├─► CDAudio_Init()         // CD audio
│   │   ├─► BGM_Init()             // Background music
│   │   └─► Sbar_Init()            // Status bar
│   │
│   ├─► PR_Init()                  // QuakeC VM
│   ├─► Mod_Init()                 // Model loader
│   ├─► NET_Init()                 // Networking
│   ├─► SV_Init()                  // Server
│   ├─► CL_Init()                  // Client
│   │
│   ├─► ExtraMaps_Init()           // Map list
│   ├─► DemoList_Init()            // Demo list
│   ├─► SaveList_Init()            // Save list
│   ├─► SkyList_Init()             // Skybox list
│   │
│   ├─► PluQ_Backend_Init()        // IPC system
│   │
│   └─► Cbuf_InsertText("exec quake.rc\n")
│
└─► Main Loop
    └─► while(1) { Host_Frame(time); }
```

### Subsystem Initialization Order

```
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 1: Core Infrastructure                                    │
├─────────────────────────────────────────────────────────────────┤
│ Memory_Init → Cbuf_Init → Cmd_Init → Cvar_Init → COM_Init      │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 2: File System                                            │
├─────────────────────────────────────────────────────────────────┤
│ COM_InitFilesystem → W_LoadWadFile                              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 3: User Interface (if not dedicated)                      │
├─────────────────────────────────────────────────────────────────┤
│ Key_Init → Con_Init → V_Init → Chase_Init → M_Init             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 4: Hardware Abstraction (if not headless)                 │
├─────────────────────────────────────────────────────────────────┤
│ VID_Init → IN_Init → TexMgr_Init → Draw_Init → SCR_Init        │
│ R_Init → S_Init → CDAudio_Init → BGM_Init → Sbar_Init          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 5: Game Systems                                           │
├─────────────────────────────────────────────────────────────────┤
│ PR_Init → Mod_Init → NET_Init → SV_Init → CL_Init              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 6: PluQ & Config                                          │
├─────────────────────────────────────────────────────────────────┤
│ PluQ_Backend_Init → exec quake.rc                               │
└─────────────────────────────────────────────────────────────────┘
```

---

## Game Loop Flow

### Main Frame (`_Host_Frame`)

```
┌─────────────────────────────────────────────────────────────────┐
│                     HOST_FRAME (one frame)                      │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ TIMING                                                          │
├─────────────────────────────────────────────────────────────────┤
│ Calculate delta time                                            │
│ Update realtime, host_frametime                                 │
│ Accumulate time for network updates                             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ ASYNC / IPC                                                     │
├─────────────────────────────────────────────────────────────────┤
│ AsyncQueue_RunQueue()                                           │
│ PluQ_ProcessInputCommands()                                     │
│ PluQ_ProcessResourceRequests()                                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ INPUT (if not headless)                                         │
├─────────────────────────────────────────────────────────────────┤
│ Key_UpdateForDest()                                             │
│ Sys_SendKeyEvents()  ──► IN_Commands()                          │
│ Host_GetConsoleCommands()                                       │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ COMMAND EXECUTION                                               │
├─────────────────────────────────────────────────────────────────┤
│ Cbuf_Execute()  ──► Process console command buffer              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ NETWORKING                                                      │
├─────────────────────────────────────────────────────────────────┤
│ NET_Poll()  ──► Read incoming packets                           │
│ CL_AccumulateCmd()  ──► Build movement command                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
          ┌───────────────────┴───────────────────┐
          │     IF accumulated_time >= tick       │
          │         (host_netinterval)            │
          └───────────────────┬───────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ SIMULATION TICK                                                 │
├─────────────────────────────────────────────────────────────────┤
│ CL_SendCmd()  ──► Send movement to server                       │
│                                                                 │
│ IF sv.active:                                                   │
│   Host_ServerFrame()                                            │
│   ├─► SV_ReadClientMessage()  ──► Read client commands          │
│   ├─► SV_Physics()  ──► Run physics simulation                  │
│   │   ├─► SV_RunThink()  ──► QuakeC think functions             │
│   │   ├─► SV_CheckVelocity()                                    │
│   │   ├─► SV_FlyMove() / SV_WalkMove()                          │
│   │   └─► SV_LinkEdict()  ──► Update collision                  │
│   └─► SV_SendClientMessages()  ──► Broadcast state              │
│                                                                 │
│ PluQ_BroadcastWorldState()                                      │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ CLIENT RECEIVE                                                  │
├─────────────────────────────────────────────────────────────────┤
│ CL_ReadFromServer()                                             │
│ ├─► CL_ParseServerMessage()                                     │
│ │   ├─► svc_update  ──► Update entity states                    │
│ │   ├─► svc_sound  ──► Trigger sounds                           │
│ │   └─► svc_*  ──► Other server commands                        │
│ └─► Update client-side interpolation                            │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ RENDERING (if not headless)                                     │
├─────────────────────────────────────────────────────────────────┤
│ SCR_UpdateScreen()                                              │
│ ├─► R_RenderView()                                              │
│ │   ├─► R_SetupFrame()                                          │
│ │   ├─► R_MarkLeaves()  ──► PVS culling                         │
│ │   ├─► R_DrawWorld()                                           │
│ │   ├─► R_DrawEntities()                                        │
│ │   │   ├─► R_DrawAliasModel()  ──► MDL models                  │
│ │   │   ├─► R_DrawBrushModel()  ──► BSP submodels               │
│ │   │   └─► R_DrawSpriteModel()  ──► Sprites                    │
│ │   └─► R_DrawParticles()                                       │
│ ├─► SCR_DrawNet()  ──► Network status                           │
│ ├─► SCR_DrawFPS()  ──► FPS counter                              │
│ ├─► Sbar_Draw()  ──► Status bar (HUD)                           │
│ ├─► SCR_DrawConsole()                                           │
│ └─► M_Draw()  ──► Menu                                          │
│                                                                 │
│ CL_RunParticles()  ──► Update particle positions                │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ AUDIO (if not headless)                                         │
├─────────────────────────────────────────────────────────────────┤
│ BGM_Update()  ──► Stream background music                       │
│ S_Update(listener_origin, forward, right, up)                   │
│ ├─► Update sound positions                                      │
│ ├─► Mix active channels                                         │
│ └─► Fill DMA buffer                                             │
│ CL_DecayLights()                                                │
│ CDAudio_Update()                                                │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ FRAME END                                                       │
├─────────────────────────────────────────────────────────────────┤
│ host_framecount++                                               │
│ Return to main loop                                             │
└─────────────────────────────────────────────────────────────────┘
```

### Server Frame Detail

```
Host_ServerFrame()
│
├─► pr_global_struct->frametime = host_frametime
│
├─► SV_ReadClientMessage() [for each client]
│   ├─► Parse clc_move  ──► Store movement command
│   ├─► Parse clc_stringcmd  ──► Execute command
│   └─► Handle disconnects
│
├─► SV_Physics()
│   │
│   ├─► [FOR EACH ENTITY]
│   │   │
│   │   ├─► SV_RunThink()
│   │   │   └─► PR_ExecuteProgram(ent->v.think)
│   │   │
│   │   ├─► [BASED ON movetype]
│   │   │   │
│   │   │   ├─► MOVETYPE_PUSH
│   │   │   │   └─► SV_PushMove()  ──► Doors, plats
│   │   │   │
│   │   │   ├─► MOVETYPE_WALK
│   │   │   │   └─► SV_WalkMove()  ──► Players
│   │   │   │
│   │   │   ├─► MOVETYPE_FLY
│   │   │   │   └─► SV_FlyMove()  ──► Flying monsters
│   │   │   │
│   │   │   ├─► MOVETYPE_TOSS
│   │   │   │   └─► SV_Physics_Toss()  ──► Grenades
│   │   │   │
│   │   │   └─► MOVETYPE_BOUNCE
│   │   │       └─► SV_Physics_Toss()  ──► Bouncing
│   │   │
│   │   └─► SV_LinkEdict()  ──► Update area links
│   │
│   └─► Increment sv.time
│
└─► SV_SendClientMessages() [for each client]
    ├─► SV_SendClientDatagram()
    │   ├─► Write entity updates
    │   └─► Write temp entities (effects)
    └─► Send reliable messages
```

### Render Frame Detail

```
R_RenderView()
│
├─► R_SetupFrame()
│   ├─► Set r_framecount
│   ├─► Calculate vieworg, viewangles
│   └─► Build frustum planes
│
├─► R_SetFrustum()
│   └─► Calculate 4 frustum planes
│
├─► R_MarkLeaves()
│   ├─► Find PVS for viewleaf
│   └─► Mark visible leaves
│
├─► R_SetupGL()
│   ├─► Set viewport
│   ├─► Set projection matrix
│   └─► Set modelview matrix
│
├─► R_DrawWorld()
│   ├─► R_RecursiveWorldNode()
│   │   └─► Walk BSP tree, cull invisible
│   └─► Draw visible surfaces
│
├─► S_ExtraUpdate()  ──► Keep audio flowing
│
├─► R_DrawEntities()
│   └─► [FOR EACH VISIBLE ENTITY]
│       │
│       ├─► [IF mod_alias]
│       │   └─► R_DrawAliasModel()
│       │       ├─► Interpolate frames
│       │       ├─► Transform vertices
│       │       └─► Draw mesh
│       │
│       ├─► [IF mod_brush]
│       │   └─► R_DrawBrushModel()
│       │       └─► Draw submodel surfaces
│       │
│       └─► [IF mod_sprite]
│           └─► R_DrawSpriteModel()
│               └─► Billboard sprite
│
├─► R_DrawWaterSurfaces()
│   └─► Draw transparent water
│
├─► R_DrawParticles()
│   └─► Draw particle effects
│
└─► R_DrawViewModel()
    └─► Draw weapon model
```

---

## Subsystem Details

### Memory Management (`zone.c`)

Three-tier memory system:

```
┌─────────────────────────────────────────────────────────────────┐
│                      MEMORY LAYOUT                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    HUNK MEMORY                          │   │
│  │  • Level-specific data (maps, models)                   │   │
│  │  • Cleared on level change                              │   │
│  │  • Hunk_Alloc(), Hunk_FreeToLowMark()                   │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    ZONE MEMORY                          │   │
│  │  • General purpose allocations                          │   │
│  │  • Persists across levels                               │   │
│  │  • Z_Malloc(), Z_Free()                                 │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   CACHE MEMORY                          │   │
│  │  • Model/sound data caching                             │   │
│  │  • LRU eviction when full                               │   │
│  │  • Cache_Alloc(), Cache_Free()                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Network Protocol

```
┌─────────────────────────────────────────────────────────────────┐
│                    NETWORK ARCHITECTURE                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐         ┌─────────────────┐               │
│  │     CLIENT      │◄───────►│     SERVER      │               │
│  └────────┬────────┘         └────────┬────────┘               │
│           │                           │                         │
│           ▼                           ▼                         │
│  ┌─────────────────┐         ┌─────────────────┐               │
│  │  clc_* messages │         │  svc_* messages │               │
│  │                 │         │                 │               │
│  │ clc_move        │         │ svc_update      │               │
│  │ clc_stringcmd   │         │ svc_sound       │               │
│  │ clc_disconnect  │         │ svc_print       │               │
│  │ clc_nop         │         │ svc_stufftext   │               │
│  └────────┬────────┘         │ svc_setangle    │               │
│           │                  │ svc_serverinfo  │               │
│           │                  │ svc_lightstyle  │               │
│           │                  │ ...             │               │
│           │                  └────────┬────────┘               │
│           │                           │                         │
│           ▼                           ▼                         │
│  ┌─────────────────────────────────────────────┐               │
│  │              DATAGRAM LAYER                 │               │
│  │                                             │               │
│  │  • Unreliable: Entity updates (fast)        │               │
│  │  • Reliable: Commands, config (guaranteed)  │               │
│  │                                             │               │
│  │  net_dgrm.c                                 │               │
│  └─────────────────────────────────────────────┘               │
│                       │                                         │
│                       ▼                                         │
│  ┌─────────────────────────────────────────────┐               │
│  │              TRANSPORT LAYER                │               │
│  │                                             │               │
│  │  net_udp.c (UDP)                            │               │
│  │  net_loop.c (Local loopback)                │               │
│  │                                             │               │
│  └─────────────────────────────────────────────┘               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### QuakeC Virtual Machine (`pr_*.c`)

```
┌─────────────────────────────────────────────────────────────────┐
│                    QUAKEC VM ARCHITECTURE                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    progs.dat                            │   │
│  │  Compiled QuakeC bytecode                               │   │
│  └─────────────────────────────────────────────────────────┘   │
│                          │                                      │
│                          ▼                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    PR_LoadProgs()                       │   │
│  │  • Load bytecode into memory                            │   │
│  │  • Resolve function addresses                           │   │
│  │  • Setup global variables                               │   │
│  └─────────────────────────────────────────────────────────┘   │
│                          │                                      │
│                          ▼                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                  PR_ExecuteProgram()                    │   │
│  │                                                         │   │
│  │  ┌─────────────────────────────────────────────┐       │   │
│  │  │           BYTECODE INTERPRETER              │       │   │
│  │  │                                             │       │   │
│  │  │  • Stack-based execution                    │       │   │
│  │  │  • Opcodes: OP_ADD, OP_IF, OP_CALL, etc.    │       │   │
│  │  │  • Entity field access                      │       │   │
│  │  │  • Global variable access                   │       │   │
│  │  │                                             │       │   │
│  │  └─────────────────────────────────────────────┘       │   │
│  └─────────────────────────────────────────────────────────┘   │
│                          │                                      │
│                          ▼                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                  Built-in Functions                     │   │
│  │  (pr_cmds.c)                                            │   │
│  │                                                         │   │
│  │  PF_makevectors    PF_setorigin    PF_setmodel          │   │
│  │  PF_spawn          PF_remove       PF_precache_*        │   │
│  │  PF_sound          PF_traceline    PF_findradius        │   │
│  │  PF_dprint         PF_walkmove     PF_droptofloor       │   │
│  │  PF_lightstyle     PF_checkbottom  PF_pointcontents     │   │
│  │  ...                                                    │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Rendering Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                    RENDERING PIPELINE                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────┐                                               │
│  │ FRAME START │                                               │
│  └──────┬──────┘                                               │
│         │                                                       │
│         ▼                                                       │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    VIEW SETUP                           │   │
│  │  • Calculate view origin, angles                        │   │
│  │  • Build view frustum (4 planes)                        │   │
│  │  • Set up matrices                                      │   │
│  └─────────────────────────────────────────────────────────┘   │
│         │                                                       │
│         ▼                                                       │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                  VISIBILITY (PVS)                       │   │
│  │  • Find current BSP leaf                                │   │
│  │  • Decompress PVS data                                  │   │
│  │  • Mark visible leaves                                  │   │
│  └─────────────────────────────────────────────────────────┘   │
│         │                                                       │
│         ▼                                                       │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   WORLD RENDERING                       │   │
│  │  • Walk BSP tree (front-to-back)                        │   │
│  │  • Frustum cull nodes                                   │   │
│  │  • Batch surfaces by texture                            │   │
│  │  • Draw opaque surfaces                                 │   │
│  └─────────────────────────────────────────────────────────┘   │
│         │                                                       │
│         ▼                                                       │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                  ENTITY RENDERING                       │   │
│  │                                                         │   │
│  │  ┌───────────────┐  ┌───────────────┐  ┌─────────────┐ │   │
│  │  │ Alias (MDL)   │  │ Brush (BSP)   │  │ Sprite (SPR)│ │   │
│  │  │               │  │               │  │             │ │   │
│  │  │ • Interpolate │  │ • Transform   │  │ • Billboard │ │   │
│  │  │ • Skin/Frame  │  │ • Draw faces  │  │ • Alpha     │ │   │
│  │  │ • Lighting    │  │ • Lightmaps   │  │             │ │   │
│  │  └───────────────┘  └───────────────┘  └─────────────┘ │   │
│  └─────────────────────────────────────────────────────────┘   │
│         │                                                       │
│         ▼                                                       │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                 TRANSPARENCY PASS                       │   │
│  │  • Draw water surfaces                                  │   │
│  │  • Draw particles                                       │   │
│  │  • Draw transparent entities                            │   │
│  └─────────────────────────────────────────────────────────┘   │
│         │                                                       │
│         ▼                                                       │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    2D OVERLAY                           │   │
│  │  • HUD (status bar)                                     │   │
│  │  • Console                                              │   │
│  │  • Menu                                                 │   │
│  │  • Net graph                                            │   │
│  └─────────────────────────────────────────────────────────┘   │
│         │                                                       │
│         ▼                                                       │
│  ┌─────────────┐                                               │
│  │ SWAP BUFFER │                                               │
│  └─────────────┘                                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Architecture Patterns

### Design Patterns Used

| Pattern | Usage | Example |
|---------|-------|---------|
| **Singleton** | Global subsystem instances | `sv`, `cl`, `cls`, `host` |
| **State Machine** | Client connection states | `ca_disconnected` → `ca_connected` |
| **Observer** | Cvar callbacks | `R_Fullbright_c()` triggers on change |
| **Command** | Console command system | `Cmd_AddCommand("map", Host_Map_f)` |
| **Factory** | Model loading | `Mod_LoadModel()` dispatches by type |
| **Strategy** | Entity rendering | Different draw functions per model type |
| **Double Buffer** | Network messages | Staging buffers for send/receive |
| **Ring Buffer** | Audio DMA | Circular buffer for sound mixing |

### Global State Objects

```c
// Core engine globals
host_parms_t host_parms;    // Startup parameters
host_t       host;          // Host state

// Client-side globals
client_state_t   cl;        // Client game state
client_static_t  cls;       // Client persistent state

// Server-side globals
server_t         sv;        // Server game state
server_static_t  svs;       // Server persistent state

// QuakeC VM globals
qcvm_t          *qcvm;      // Active VM instance
globalvars_t    *pr_global_struct;  // QuakeC globals
```

### Error Handling

```
┌─────────────────────────────────────────────────────────────────┐
│                    ERROR HANDLING                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Sys_Error(char *error, ...)                                    │
│  ├─► Display error message                                      │
│  ├─► Shutdown all subsystems                                    │
│  └─► Exit process                                               │
│                                                                 │
│  Host_Error(char *error, ...)                                   │
│  ├─► Recoverable error                                          │
│  ├─► Disconnect from server                                     │
│  └─► Return to console                                          │
│                                                                 │
│  Con_Printf(char *fmt, ...)                                     │
│  └─► Non-fatal message to console                               │
│                                                                 │
│  Con_DPrintf(char *fmt, ...)                                    │
│  └─► Debug message (if developer mode)                          │
│                                                                 │
│  Con_Warning(char *fmt, ...)                                    │
│  └─► Warning message                                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## PluQ IPC Integration

For PluQ-specific architecture (backend/frontend split, IPC protocol), see:
- [`PLUQ.md`](PLUQ.md) - PluQ architecture overview
- [`IPC_IMPLEMENTATION_STATUS.md`](IPC_IMPLEMENTATION_STATUS.md) - IPC status

```
┌─────────────────────────────────────────────────────────────────┐
│                    PLUQ IPC INTEGRATION                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────┐         ┌─────────────────────┐       │
│  │      BACKEND        │         │      FRONTEND       │       │
│  │  (main Quake)       │◄───────►│   (separate proc)   │       │
│  │                     │   IPC   │                     │       │
│  │  pluq_backend.c     │  pipes  │  pluq_frontend.c    │       │
│  └─────────────────────┘         └─────────────────────┘       │
│                                                                 │
│  Integration Points in Host_Frame():                            │
│  • PluQ_ProcessInputCommands()  - Read input from frontend      │
│  • PluQ_ProcessResourceRequests() - Send resources on demand    │
│  • PluQ_BroadcastWorldState() - Send world updates              │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Quick Reference

### Important Files by Function

| Function | File |
|----------|------|
| Entry point | `main_sdl.c:main()` |
| Main loop | `host.c:_Host_Frame()` |
| Initialization | `host.c:Host_Init()` |
| Server frame | `host.c:Host_ServerFrame()` |
| Physics | `sv_phys.c:SV_Physics()` |
| Client receive | `cl_main.c:CL_ReadFromServer()` |
| Rendering | `gl_rmain.c:R_RenderView()` |
| Sound update | `snd_dma.c:S_Update()` |
| QuakeC execution | `pr_exec.c:PR_ExecuteProgram()` |
| Model loading | `gl_model.c:Mod_LoadModel()` |
| Command parsing | `cmd.c:Cmd_ExecuteString()` |

### Key Cvars

| Cvar | Default | Description |
|------|---------|-------------|
| `host_maxfps` | 250 | Frame rate limit |
| `host_framerate` | 0 | Fixed timestep (0 = variable) |
| `sv_cheats` | 0 | Allow cheat commands |
| `developer` | 0 | Developer mode |
| `r_novis` | 0 | Disable PVS culling |
| `snd_speed` | 44100 | Audio sample rate |
