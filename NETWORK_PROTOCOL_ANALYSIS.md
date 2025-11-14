# Network Protocol Analysis: FlatBuffers vs Quake Native Protocol

## TL;DR Recommendation

**Keep FlatBuffers for PluQ IPC** ✅

**Reasons:**
1. Cross-platform safety (no endianness issues)
2. Schema evolution support
3. Zero-copy parsing
4. Clean separation from game logic
5. Better tooling and validation

**However:** Could consider using Quake's MSG_ format for the Input channel only (simple, already tested).

---

## Quake 1 Native Network Protocol

### What Quake Already Has

#### 1. **Data Structures** (protocol.h:257-278)
```c
typedef struct {
    vec3_t        origin;
    vec3_t        angles;
    unsigned short modelindex;
    unsigned short frame;
    unsigned char  colormap;
    unsigned char  skin;
    unsigned char  alpha;
    unsigned char  scale;
    int           effects;
} entity_state_t;

typedef struct {
    vec3_t viewangles;
    float  forwardmove;
    float  sidemove;
    float  upmove;
} usercmd_t;
```

#### 2. **Serialization Functions** (common.h:220-243)
```c
// Writing
void MSG_WriteChar(sizebuf_t *sb, int c);
void MSG_WriteByte(sizebuf_t *sb, int c);
void MSG_WriteShort(sizebuf_t *sb, int c);
void MSG_WriteLong(sizebuf_t *sb, int c);
void MSG_WriteFloat(sizebuf_t *sb, float f);
void MSG_WriteString(sizebuf_t *sb, const char *s);
void MSG_WriteCoord(sizebuf_t *sb, float f, unsigned int flags);
void MSG_WriteAngle(sizebuf_t *sb, float f, unsigned int flags);

// Reading
int MSG_ReadChar(void);
int MSG_ReadByte(void);
int MSG_ReadShort(void);
int MSG_ReadLong(void);
float MSG_ReadFloat(void);
char *MSG_ReadString(void);
float MSG_ReadCoord(unsigned int flags);
float MSG_ReadAngle(unsigned int flags);
```

#### 3. **Delta Compression** (protocol.h:43-64)
```c
#define U_MOREBITS    (1<<0)
#define U_ORIGIN1     (1<<1)  // Only send if changed
#define U_ORIGIN2     (1<<2)
#define U_ORIGIN3     (1<<3)
#define U_ANGLE2      (1<<4)
#define U_FRAME       (1<<6)
#define U_ANGLE1      (1<<8)
#define U_ANGLE3      (1<<9)
#define U_MODEL       (1<<10)
```

**How it works:**
- Only changed fields are sent
- First 1-2 bytes are flags indicating what follows
- Can compress entity updates to just a few bytes if nothing changed

#### 4. **Buffer Management** (common.h:115-122)
```c
typedef struct sizebuf_s {
    qboolean  allowoverflow;
    qboolean  overflowed;
    byte     *data;
    int       maxsize;
    int       cursize;
} sizebuf_t;
```

---

## Comparison Matrix

| Feature | Quake Native | FlatBuffers | Winner |
|---------|--------------|-------------|--------|
| **Binary Size** | 🟢 Smaller (delta compression) | 🟡 Larger (full state) | Quake |
| **Endianness Safety** | 🔴 Platform-dependent | 🟢 Always little-endian | FlatBuffers |
| **Schema Evolution** | 🔴 Hard (break protocol) | 🟢 Easy (versioned) | FlatBuffers |
| **Zero-Copy** | 🔴 No (must parse) | 🟢 Yes (direct access) | FlatBuffers |
| **Delta Compression** | 🟢 Built-in (U_* flags) | 🔴 None | Quake |
| **Bandwidth** | 🟢 ~10-50 bytes/entity | 🟡 ~100 bytes/entity | Quake |
| **Latency** | 🟢 Minimal parsing | 🟢 Zero-copy | Tie |
| **Type Safety** | 🔴 Manual reading order | 🟢 Schema enforced | FlatBuffers |
| **Existing Code** | 🟢 Already tested | 🟡 New code | Quake |
| **Tooling** | 🔴 None | 🟢 flatc compiler, validators | FlatBuffers |
| **Cross-platform** | 🔴 Endian issues | 🟢 Guaranteed | FlatBuffers |
| **Complexity** | 🟢 Simple | 🟡 Requires schema | Quake |

---

## Deep Dive: Issues with Quake Native Protocol

### 1. **Endianness Problems**
```c
// Quake's MSG_WriteShort (common.c:716)
void MSG_WriteShort (sizebuf_t *sb, int c) {
    byte *buf = (byte *)SZ_GetSpace(sb, 2);
    buf[0] = c & 0xff;
    buf[1] = c >> 8;  // Little-endian hardcoded\!
}
```

**Problem:** Breaks on big-endian systems (PowerPC, some ARM, MIPS)

**FlatBuffers solution:** Always uses little-endian, with automatic conversion

### 2. **No Version Control**
```c
// If you add a new field to entity_state_t, you must:
// 1. Add new U_NEWFIELD flag
// 2. Update SV_WriteEntitiesToClient()
// 3. Update CL_ParseUpdate()
// 4. Break compatibility with old clients
```

**FlatBuffers solution:** 
- Optional fields
- Default values
- Forward/backward compatibility

### 3. **Manual Reading Order**
```c
// CL_ParseUpdate() - MUST read in exact order\!
if (bits & U_MODEL)
    modnum = MSG_ReadByte();
if (bits & U_FRAME)
    frame = MSG_ReadByte();
// Order matters\! Wrong order = corrupt data
```

**FlatBuffers solution:** Fields accessed by name, order doesn't matter

### 4. **Buffer Overflow Risk**
```c
sizebuf_t msg;
msg.data = buf;
msg.maxsize = 1024;
msg.cursize = 0;

MSG_WriteByte(&msg, 123);  // What if buffer full?
// Relies on allowoverflow flag and manual checking
```

**FlatBuffers solution:** Builder handles overflow automatically

---

## Bandwidth Analysis

### Quake Native Protocol (with delta compression)

**Entity Update (only position changed):**
```
Flags: 2 bytes  (U_ORIGIN1|U_ORIGIN2|U_ORIGIN3)
X:     2 bytes  (coord)
Y:     2 bytes  (coord)
Z:     2 bytes  (coord)
Total: 8 bytes
```

**Full Entity Update (first send):**
```
Flags:  2 bytes
Origin: 6 bytes (3 coords)
Angles: 6 bytes (3 angles)
Model:  2 bytes
Frame:  2 bytes
Skin:   1 byte
Alpha:  1 byte
Total:  20 bytes
```

### FlatBuffers (PluQ current implementation)

**Entity (from pluq.fbs):**
```
Origin:   12 bytes (3x float32)
Angles:   12 bytes (3x float32)
Model_id:  2 bytes (uint16)
Frame:     1 byte  (uint8)
Colormap:  1 byte  (uint8)
Skin:      1 byte  (uint8)
Effects:   4 bytes (uint32)
Alpha:     4 bytes (float32)
Overhead: ~8 bytes (vtable, alignment)
Total:   ~45 bytes
```

**Bandwidth for 100 entities @ 72 FPS:**

| Protocol | Per Frame | Per Second | Per Minute |
|----------|-----------|------------|------------|
| **Quake Native** | ~800 bytes | 58 KB/s | 3.4 MB/min |
| **FlatBuffers** | ~4.5 KB | 324 KB/s | 19 MB/min |

**Verdict:** Quake is 5-6x more efficient, but 324 KB/s is still very reasonable for LAN/localhost.

---

## Could We Use Quake's Protocol?

### ✅ Yes, Technically Feasible

**What would change:**

1. **Replace FlatBuffers with MSG_***
```c
// Instead of:
PluQ_Entity_vec_push_start(&builder);
PluQ_Entity_origin_add(&builder, &origin);

// Use:
MSG_WriteCoord(&msg, ent->origin[0], protocol_flags);
MSG_WriteCoord(&msg, ent->origin[1], protocol_flags);
MSG_WriteCoord(&msg, ent->origin[2], protocol_flags);
```

2. **Reuse existing entity_state_t**
```c
// Already defined and stable
typedef struct {
    vec3_t origin;
    vec3_t angles;
    unsigned short modelindex;
    unsigned short frame;
    unsigned char colormap;
    unsigned char skin;
    unsigned char alpha;
    unsigned char scale;
    int effects;
} entity_state_t;
```

3. **Reuse SV_WriteEntitiesToClient() logic**
```c
// Already has delta compression
// Already handles visibility culling
// Already tested for 25+ years
```

### ❌ But You'd Lose

1. **Cross-platform safety** - Endianness issues on non-x86
2. **Schema evolution** - Can't add new fields without breaking protocol
3. **Type safety** - Must read in correct order manually
4. **Tooling** - No schema compiler, validators, or introspection
5. **Separation of concerns** - IPC tied to game protocol version
6. **Zero-copy** - Must deserialize into structs

---

## Hybrid Approach: Best of Both Worlds?

### Option 1: Quake Protocol for Input Channel Only

**Rationale:**
- usercmd_t is simple and stable (hasn't changed in 25 years)
- Input is sent frequently (72 FPS)
- Bandwidth savings matter more for input

**Implementation:**
```c
// Instead of FlatBuffers for InputCommand
typedef struct {
    byte header;        // clc_move
    usercmd_t cmd;      // Already defined
    char cmd_text[256]; // Console command
} pluq_input_msg_t;
```

**Size:**
- FlatBuffers: ~54-66 bytes
- Native: ~30 bytes
- Savings: 40%

**Verdict:** ✅ Worth considering for Input channel

### Option 2: Quake Protocol for Gameplay Channel

**Rationale:**
- entity_state_t already defined
- SV_WriteEntitiesToClient() already has delta compression
- Bandwidth matters for 100+ entities

**Concerns:**
- Loses schema evolution (what if we want to add new fields?)
- Loses cross-platform (IPC might not always be localhost)
- Loses separation (IPC tied to game version)

**Verdict:** ❌ Risky, not worth it

---

## Recommendations

### 1. **Keep FlatBuffers for PluQ** ✅

**Why:**
- Cross-platform guarantee
- Schema evolution for future features
- Clean separation from game logic
- Modern tooling
- Bandwidth is acceptable (324 KB/s for 100 entities)

### 2. **Consider Quake Protocol for Input Channel** 🤔

**Implementation:**
```c
// pluq_input_native.c
typedef struct {
    usercmd_t cmd;
    char cmd_text[256];
} pluq_input_native_t;

// Send
void PluQ_Frontend_SendInput_Native(usercmd_t *cmd, const char *cmd_text) {
    sizebuf_t sb;
    byte buf[512];
    SZ_Init(&sb, buf, sizeof(buf));
    
    MSG_WriteFloat(&sb, cmd->forwardmove);
    MSG_WriteFloat(&sb, cmd->sidemove);
    MSG_WriteFloat(&sb, cmd->upmove);
    MSG_WriteAngle(&sb, cmd->viewangles[0], 0);
    MSG_WriteAngle(&sb, cmd->viewangles[1], 0);
    MSG_WriteAngle(&sb, cmd->viewangles[2], 0);
    MSG_WriteString(&sb, cmd_text ? cmd_text : "");
    
    nng_send(input_push, sb.data, sb.cursize, 0);
}

// Receive
qboolean PluQ_Backend_ReceiveInput_Native(usercmd_t *cmd_out, char *text_out) {
    // MSG_ReadFloat(), MSG_ReadAngle(), MSG_ReadString()
}
```

**Benefits:**
- 40% bandwidth reduction (66 → 30 bytes)
- Uses battle-tested code
- Simple data structure

**Drawbacks:**
- Can't easily add buttons/impulse later
- Endianness issues if IPC goes over network
- Loses schema validation

**Verdict:** Only worth it if bandwidth is critical

### 3. **Optimize FlatBuffers Instead** ✅ BEST

**Better approach:**
```flatbuffers
// Use smaller types in pluq.fbs
table Entity {
    origin: Vec3Short;      // 3x int16 instead of float32 = 6 bytes
    angles: Vec3Short;      // 6 bytes
    model_id: uint16;       // 2 bytes
    frame: uint8;           // 1 byte
    colormap: uint8;        // 1 byte
    skin: uint8;            // 1 byte
    effects: uint16;        // 2 bytes (was uint32)
    alpha: uint8;           // 1 byte (0-255, was float32)
}
// Total: ~20 bytes (same as Quake\!)
```

**With fixed-point coords:**
```c
struct Vec3Short {
    int16 x;  // coord * 8 (gives 0.125 precision)
    int16 y;
    int16 z;
}
```

**Benefits:**
- Keep FlatBuffers' safety and features
- Match Quake's bandwidth
- Still cross-platform
- Still have schema evolution

---

## Conclusion

### Current Status: ✅ Excellent

The PluQ IPC system with FlatBuffers is production-ready:
- 324 KB/s for 100 entities @ 72 FPS
- Cross-platform safe
- Schema evolution support
- Zero-copy parsing
- Well-tested

### Future Optimization (if needed):

**Phase 1: Optimize FlatBuffers schema** ⭐ RECOMMENDED
- Use int16 for coords (6 bytes vs 12 bytes)
- Use uint8 for alpha (1 byte vs 4 bytes)
- Use uint16 for effects (2 bytes vs 4 bytes)
- **Result:** ~20 bytes/entity (same as Quake\!)

**Phase 2: Add delta compression**
- Track previous state
- Only send changed entities
- **Result:** 5-10x bandwidth reduction

**Phase 3: Consider Quake protocol for Input only**
- Saves 40% on input bandwidth
- Only if IPC stays on localhost/LAN
- Only if schema flexibility not needed

### Final Verdict

**Keep FlatBuffers** ✅

Optimize the schema if bandwidth becomes an issue, but the current implementation is sound architecture that prioritizes:
1. Correctness (cross-platform, type-safe)
2. Maintainability (schema evolution)
3. Performance (good enough for LAN/localhost)

The Quake native protocol is optimized for 1990s dial-up modems (28.8 kbps). Modern IPC doesn't need that level of optimization.
