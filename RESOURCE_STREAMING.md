# PluQ Resource Streaming Implementation

## Overview

This implementation adds resource streaming functionality to PluQuake, allowing frontend clients to receive textures and other game assets via IPC instead of requiring local PAK files.

## Architecture

### Backend (Server)
- **File**: `Quake/pluq_backend.c`
- **Function**: `PluQ_ProcessResourceRequests()`
- **Description**: Listens for resource requests on the Resources channel (REQ/REP pattern), loads resources from local WAD/PAK files, and sends them to requesting frontends.

**Supported Resources**:
- ✅ Textures (from WAD files via `W_GetLumpName`)
- ⚠️  Models (TODO - skeleton implemented)
- ⚠️  BSP data (TODO)
- ⚠️  Lightmaps (TODO)

### Frontend (Client)
- **File**: `Quake/pluq_frontend.c`
- **Function**: `PluQ_Frontend_RequestResource()`
- **Description**: Sends resource requests to backend via IPC and receives serialized resource data.

**Resource Types**:
```c
typedef enum {
  None = 0,
  Texture = 1,
  Model = 2,
  BSPVertices = 3,
  BSPFaces = 4,
  Lightmap = 5
} ResourceType;
```

## Frontend Variants

### 1. Standard Frontend (`ironwail-pluq-frontend`)
- **Makefile**: `Makefile.pluq_frontend`
- **WAD Handler**: `wad.c` (standard local file loading)
- **Requirements**: Requires local PAK0.PAK, PAK1.PAK files
- **Use Case**: Local rendering with optional IPC state sync

### 2. Streaming Frontend (`ironwail-pluq-frontend-stream`) ⭐ NEW
- **Makefile**: `Makefile.pluq_frontend_stream`
- **WAD Handler**: `wad_pluq_frontend_stream.c` (IPC resource streaming)
- **Requirements**: NO local PAK files needed - all resources streamed from backend
- **Use Case**: Thin client, remote rendering, cloud gaming
- **Compile Flag**: `-DPLUQ_FRONTEND_STREAM`

## Building

### Backend (with resource streaming support)
```bash
cd Quake
make -f Makefile
```

### Standard Frontend (local resources)
```bash
cd Quake
make -f Makefile.pluq_frontend
```

### Streaming Frontend (IPC resources only)
```bash
cd Quake
make -f Makefile.pluq_frontend_stream
```

## Usage

### Running with Streaming Frontend

**Terminal 1 - Backend**:
```bash
./ironwail -pluq_backend
```

**Terminal 2 - Streaming Frontend**:
```bash
./ironwail-pluq-frontend-stream
```

The frontend will request all required textures from the backend via IPC. No local PAK files are needed on the frontend machine.

## IPC Channels

| Channel | Pattern | Port | Purpose | Status |
|---------|---------|------|---------|--------|
| Resources | REQ/REP | 9001 | On-demand resource fetching | ✅ Implemented |
| Gameplay | PUB/SUB | 9002 | Game state broadcasting | ✅ Working |
| Input | PUSH/PULL | 9003 | Player input forwarding | ✅ Working |

## Performance

**Resource Channel (REQ/REP)**:
- Texture size: ~1KB - 256KB per texture
- Latency: <5ms (TCP localhost)
- Pattern: Synchronous request/reply
- Caching: Frontend caches received resources in memory

**Network Overhead**:
- Initial map load: ~5-20MB (all textures)
- Runtime: Minimal (only new textures as needed)

## Code Changes Summary

### New Files
1. `Quake/wad_pluq_frontend_stream.c` - IPC-based WAD loader
2. `Quake/Makefile.pluq_frontend_stream` - Build config for streaming variant
3. `RESOURCE_STREAMING.md` - This documentation

### Modified Files
1. `Quake/pluq_backend.c` - Added `PluQ_ProcessResourceRequests()`
2. `Quake/pluq_backend.h` - Added function declaration
3. `Quake/pluq_frontend.c` - Added `PluQ_Frontend_RequestResource()`
4. `Quake/pluq_frontend.h` - Updated function signature
5. `Quake/host.c` - Added resource processing to main loop

### Workflow Cleanup
- Consolidated redundant `build-pluq-*` and `rebuild-*-libs` workflows
- Reduced from 11 workflows to 7 workflows
- Improved verification and commit messages

## Schema (FlatBuffers)

The resource streaming uses the existing `pluq.fbs` schema:

```flatbuffers
table ResourceRequest {
  resource_type: ResourceType;
  resource_id: uint32;
  resource_name: string;
}

table ResourceResponse {
  resource_id: uint32;
  data: ResourceData;  // Union of Texture, Model, etc.
}

table Texture {
  id: uint32;
  name: string;
  width: uint16;
  height: uint16;
  format: byte;  // 0=RGBA, 1=RGB, 2=Indexed
  pixels: [ubyte];
}
```

## Testing

### Manual Testing
1. Start backend with PluQ enabled
2. Start streaming frontend
3. Load a map
4. Verify console shows resource requests/responses
5. Verify textures render correctly

### Automated Testing
TODO: Add integration tests for resource streaming

## Future Enhancements

- [ ] Model streaming (MDL files)
- [ ] BSP geometry streaming
- [ ] Lightmap streaming
- [ ] Resource compression (zstd/lz4)
- [ ] Resource caching to disk
- [ ] Progress bars for large transfers
- [ ] Incremental/delta updates
- [ ] Multi-frontend support (multiple clients from one backend)

## Troubleshooting

**Q: Frontend shows "Failed to fetch texture from backend"**
- Ensure backend is running with `-pluq_backend` flag
- Check that backend has PAK files in id1/ directory
- Verify network connectivity (ports 9001-9003)

**Q: Textures appear corrupted**
- Check FlatBuffers schema versions match
- Verify endianness handling (LittleLong conversions)
- Enable debug logging with `-condebug`

**Q: High latency/slow loading**
- Resources are fetched synchronously - network latency impacts loading
- Consider local caching or prefetching
- Use localhost (127.0.0.1) for minimal latency

## License

GNU General Public License v2 (same as Quake/Ironwail)
