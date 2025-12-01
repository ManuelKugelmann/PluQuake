# PLQ Data Structure Plan

**Goal:** Maximize reuse of existing Quake data structures. Minimize new PLQ-specific types.

---

## Philosophy

1. **Reuse Quake structs** - Don't create parallel types
2. **Native precision** - No compression, use Quake's exact data types
3. **Direct memcpy** - FlatBuffers structs match C struct layout exactly
4. **Zero conversion** - No field-by-field copying, no math operations

---

## Binary-Compatible Structs

### `usercmd_t` (24 bytes)

```c
// Quake: protocol.h:270
typedef struct {
    vec3_t  viewangles;     // float[3], offset 0
    float   forwardmove;    // offset 12
    float   sidemove;       // offset 16
    float   upmove;         // offset 20
} usercmd_t;
```

```flatbuffers
// FlatBuffers: same name, same layout
struct usercmd_t {
    viewangles_x:float;
    viewangles_y:float;
    viewangles_z:float;
    forwardmove:float;
    sidemove:float;
    upmove:float;
}
```

**Usage:** `memcpy(&fb_cmd, &quake_cmd, sizeof(usercmd_t))`

---

### `entity_state_t` (36 bytes)

```c
// Quake: protocol.h:257
typedef struct {
    vec3_t          origin;      // float[3], offset 0
    vec3_t          angles;      // float[3], offset 12
    unsigned short  modelindex;  // offset 24
    unsigned short  frame;       // offset 26
    unsigned char   colormap;    // offset 28
    unsigned char   skin;        // offset 29
    unsigned char   alpha;       // offset 30
    unsigned char   scale;       // offset 31
    int             effects;     // offset 32
} entity_state_t;
```

```flatbuffers
// FlatBuffers: same name, same layout
struct entity_state_t {
    origin_x:float;
    origin_y:float;
    origin_z:float;
    angles_x:float;
    angles_y:float;
    angles_z:float;
    modelindex:uint16;
    frame:uint16;
    colormap:uint8;
    skin:uint8;
    alpha:uint8;
    scale:uint8;
    effects:int32;
}
```

**Usage:** `memcpy(fb_entities, quake_entities, count * sizeof(entity_state_t))`

---

### `dlight_t` (48 bytes)

```c
// Quake: client.h:69
typedef struct {
    vec3_t  origin;     // float[3], offset 0
    float   radius;     // offset 12
    float   spawn;      // offset 16
    float   die;        // offset 20
    float   decay;      // offset 24
    float   minlight;   // offset 28
    int     key;        // offset 32
    vec3_t  color;      // float[3], offset 36
} dlight_t;
```

```flatbuffers
// FlatBuffers: same name, same layout
struct dlight_t {
    origin_x:float;
    origin_y:float;
    origin_z:float;
    radius:float;
    spawn:float;
    die:float;
    decay:float;
    minlight:float;
    key:int32;
    color_r:float;
    color_g:float;
    color_b:float;
}
```

**Usage:** `memcpy(fb_dlights, cl_dlights, count * sizeof(dlight_t))`

---

### `cshift_t` (16 bytes)

```c
// Quake: client.h:46
typedef struct {
    int     destcolor[3];   // int[3], offset 0
    float   percent;        // offset 12
} cshift_t;
```

```flatbuffers
// FlatBuffers: same name, same layout
struct cshift_t {
    destcolor_r:int32;
    destcolor_g:int32;
    destcolor_b:int32;
    percent:float;
}
```

**Usage:** `memcpy(fb_cshifts, cl.cshifts, NUM_CSHIFTS * sizeof(cshift_t))`

---

## Composite Structs (PLQ-specific)

### `plq_viewstate_t` - Client view data

Combines fields from `client_state_t` for per-frame updates:

```flatbuffers
struct plq_viewstate_t {
    // cl.viewangles
    viewangles_x:float;
    viewangles_y:float;
    viewangles_z:float;
    // cl.velocity
    velocity_x:float;
    velocity_y:float;
    velocity_z:float;
    // cl.punchangle
    punchangle_x:float;
    punchangle_y:float;
    punchangle_z:float;
    // cl.viewheight
    viewheight:float;
    // cl.time
    time:float64;
    // cl.viewentity
    viewentity:int16;
    // flags: paused|onground|inwater|intermission
    flags:uint16;
}
```

### `plq_stats_t` - Player stats

Maps directly to `cl.stats[]` indices:

```flatbuffers
struct plq_stats_t {
    health:int32;           // STAT_HEALTH = 0
    frags:int32;            // STAT_FRAGS = 1
    weapon:int32;           // STAT_WEAPON = 2
    ammo:int32;             // STAT_AMMO = 3
    armor:int32;            // STAT_ARMOR = 4
    weaponframe:int32;      // STAT_WEAPONFRAME = 5
    shells:int32;           // STAT_SHELLS = 6
    nails:int32;            // STAT_NAILS = 7
    rockets:int32;          // STAT_ROCKETS = 8
    cells:int32;            // STAT_CELLS = 9
    activeweapon:int32;     // STAT_ACTIVEWEAPON = 10
    totalsecrets:int32;     // STAT_TOTALSECRETS = 11
    totalmonsters:int32;    // STAT_TOTALMONSTERS = 12
    secrets:int32;          // STAT_SECRETS = 13
    monsters:int32;         // STAT_MONSTERS = 14
    items:uint32;           // cl.items
}
```

**Usage:** `memcpy(&fb_stats, cl.stats, 15 * 4); fb_stats.items = cl.items;`

---

## Complete FlatBuffers Schema

```flatbuffers
namespace PLQ;

// ═══════════════════════════════════════════════════════════════
// QUAKE STRUCTS (binary-compatible, memcpy directly)
// ═══════════════════════════════════════════════════════════════

// protocol.h:270 - 24 bytes
struct usercmd_t {
    viewangles_x:float;
    viewangles_y:float;
    viewangles_z:float;
    forwardmove:float;
    sidemove:float;
    upmove:float;
}

// protocol.h:257 - 36 bytes
struct entity_state_t {
    origin_x:float;
    origin_y:float;
    origin_z:float;
    angles_x:float;
    angles_y:float;
    angles_z:float;
    modelindex:uint16;
    frame:uint16;
    colormap:uint8;
    skin:uint8;
    alpha:uint8;
    scale:uint8;
    effects:int32;
}

// client.h:69 - 48 bytes
struct dlight_t {
    origin_x:float;
    origin_y:float;
    origin_z:float;
    radius:float;
    spawn:float;
    die:float;
    decay:float;
    minlight:float;
    key:int32;
    color_r:float;
    color_g:float;
    color_b:float;
}

// client.h:46 - 16 bytes
struct cshift_t {
    destcolor_r:int32;
    destcolor_g:int32;
    destcolor_b:int32;
    percent:float;
}

// ═══════════════════════════════════════════════════════════════
// PLQ STRUCTS (composite, PLQ-specific)
// ═══════════════════════════════════════════════════════════════

// View state from client_state_t fields
struct plq_viewstate_t {
    viewangles_x:float;
    viewangles_y:float;
    viewangles_z:float;
    velocity_x:float;
    velocity_y:float;
    velocity_z:float;
    punchangle_x:float;
    punchangle_y:float;
    punchangle_z:float;
    viewheight:float;
    time:float64;
    viewentity:int16;
    flags:uint16;
}

// Stats from cl.stats[] (int32 to match Quake's int)
struct plq_stats_t {
    health:int32;
    frags:int32;
    weapon:int32;
    ammo:int32;
    armor:int32;
    weaponframe:int32;
    shells:int32;
    nails:int32;
    rockets:int32;
    cells:int32;
    activeweapon:int32;
    totalsecrets:int32;
    totalmonsters:int32;
    secrets:int32;
    monsters:int32;
    items:uint32;
}

// ═══════════════════════════════════════════════════════════════
// MESSAGES
// ═══════════════════════════════════════════════════════════════

// Backend → Frontend: Per-frame world state
table plq_frame_t {
    view:plq_viewstate_t;
    stats:plq_stats_t;
    entities:[entity_state_t];
    dlights:[dlight_t];
    cshifts:[cshift_t];
}

// Backend → Frontend: Map loaded
table plq_mapchange_t {
    mapname:string;
    levelname:string;
    maxclients:uint8;
    gametype:uint8;
}

// Backend → Frontend: Light style
table plq_lightstyle_t {
    index:uint8;
    map:string;
}

table plq_lightstyles_t {
    styles:[plq_lightstyle_t];
}

// Backend → Frontend: Disconnect
table plq_disconnect_t {
    reason:string;
}

// Frontend → Backend: Input
table plq_input_t {
    cmd:usercmd_t;
    msec:uint8;
}

// Frontend → Backend: Console command
table plq_command_t {
    text:string;
}

// Frontend → Backend: Resource request
table plq_resourcereq_t {
    type:uint8;
    index:uint16;
    name:string;
}

// Backend → Frontend: Resource data
table plq_resourceresp_t {
    type:uint8;
    index:uint16;
    name:string;
    data:[ubyte];
}

// ═══════════════════════════════════════════════════════════════
// MESSAGE WRAPPER
// ═══════════════════════════════════════════════════════════════

enum plq_msgtype_t:uint8 {
    none = 0,
    frame = 1,
    mapchange = 2,
    lightstyles = 3,
    disconnect = 4,
    input = 5,
    command = 6,
    resourcereq = 7,
    resourceresp = 8
}

union plq_payload_t {
    plq_frame_t,
    plq_mapchange_t,
    plq_lightstyles_t,
    plq_disconnect_t,
    plq_input_t,
    plq_command_t,
    plq_resourcereq_t,
    plq_resourceresp_t
}

table plq_message_t {
    type:plq_msgtype_t;
    payload:plq_payload_t;
}

root_type plq_message_t;
```

---

## Compile-Time Verification

```c
// plq_verify.h
#include <stddef.h>
#include "protocol.h"
#include "client.h"

// Verify binary compatibility at compile time
_Static_assert(sizeof(usercmd_t) == 24, "usercmd_t must be 24 bytes");
_Static_assert(sizeof(entity_state_t) == 36, "entity_state_t must be 36 bytes");
_Static_assert(sizeof(dlight_t) == 48, "dlight_t must be 48 bytes");
_Static_assert(sizeof(cshift_t) == 16, "cshift_t must be 16 bytes");

// Verify field offsets
_Static_assert(offsetof(usercmd_t, forwardmove) == 12, "usercmd_t layout");
_Static_assert(offsetof(entity_state_t, modelindex) == 24, "entity_state_t layout");
_Static_assert(offsetof(entity_state_t, effects) == 32, "entity_state_t layout");
_Static_assert(offsetof(dlight_t, key) == 32, "dlight_t layout");
_Static_assert(offsetof(dlight_t, color) == 36, "dlight_t layout");
_Static_assert(offsetof(cshift_t, percent) == 12, "cshift_t layout");
```

---

## Usage Examples

### Serialize Entities (Backend)

```c
void PLQ_BE_SendFrame(void)
{
    flatcc_builder_t B;
    flatcc_builder_init(&B);

    // Entities: direct memcpy - same struct name, same layout
    PLQ_plq_frame_t_entities_start(&B);
    PLQ_entity_state_t_vec_push(&B, (PLQ_entity_state_t *)cl_entities, cl.num_entities);
    PLQ_plq_frame_t_entities_end(&B);

    // DLights: direct memcpy
    PLQ_plq_frame_t_dlights_start(&B);
    PLQ_dlight_t_vec_push(&B, (PLQ_dlight_t *)cl_dlights, MAX_DLIGHTS);
    PLQ_plq_frame_t_dlights_end(&B);

    // CShifts: direct memcpy
    PLQ_plq_frame_t_cshifts_start(&B);
    PLQ_cshift_t_vec_push(&B, (PLQ_cshift_t *)cl.cshifts, NUM_CSHIFTS);
    PLQ_plq_frame_t_cshifts_end(&B);

    // ... finalize and send
}
```

### Deserialize Entities (Frontend)

```c
void PLQ_FE_ParseFrame(const void *buf, size_t size)
{
    PLQ_plq_message_t_table_t msg = PLQ_plq_message_t_as_root(buf);
    PLQ_plq_frame_t_table_t frame = PLQ_plq_message_t_payload(msg);

    // Entities: direct memcpy - structs are binary identical
    PLQ_entity_state_t_vec_t ents = PLQ_plq_frame_t_entities(frame);
    size_t count = PLQ_entity_state_t_vec_len(ents);
    memcpy(cl_entities, ents, count * sizeof(entity_state_t));
    cl.num_entities = count;

    // DLights: direct memcpy
    PLQ_dlight_t_vec_t dlights = PLQ_plq_frame_t_dlights(frame);
    memcpy(cl_dlights, dlights, PLQ_dlight_t_vec_len(dlights) * sizeof(dlight_t));

    // CShifts: direct memcpy
    PLQ_cshift_t_vec_t cshifts = PLQ_plq_frame_t_cshifts(frame);
    memcpy(cl.cshifts, cshifts, NUM_CSHIFTS * sizeof(cshift_t));
}
```

### Serialize Input (Frontend)

```c
void PLQ_FE_SendInput(usercmd_t *cmd)
{
    flatcc_builder_t B;
    flatcc_builder_init(&B);

    PLQ_plq_input_t_start_as_root(&B);
    // Direct cast - Quake's usercmd_t and PLQ's usercmd_t are identical
    PLQ_plq_input_t_cmd_add(&B, (PLQ_usercmd_t *)cmd);
    PLQ_plq_input_t_msec_add(&B, host_frametime * 1000);
    PLQ_plq_input_t_end_as_root(&B);

    // ... send
}
```

---

## Summary

| Quake Struct | FlatBuffers Struct | Size | Direct Copy |
|--------------|-------------------|------|-------------|
| `usercmd_t` | `usercmd_t` | 24 | YES |
| `entity_state_t` | `entity_state_t` | 36 | YES |
| `dlight_t` | `dlight_t` | 48 | YES |
| `cshift_t` | `cshift_t` | 16 | YES |
| `cl.stats[0..14]` | `plq_stats_t` | 64 | YES |

**Naming convention:**
- Quake structs: Same name (`usercmd_t`, `entity_state_t`, etc.)
- PLQ-specific: `plq_*_t` prefix (`plq_frame_t`, `plq_viewstate_t`, etc.)

**Conversion code needed:** 0 (memcpy only)
