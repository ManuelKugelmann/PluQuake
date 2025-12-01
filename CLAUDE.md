# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Philosophy

**Keep as close as possible to the respective original ironwail or quakespasm.** PluQ code is isolated in separate files (pluq_*.c) to minimize intrusion into the core engine.

## Branch Structure

- **master**: Ironwail + PluQ (will become ironwail-pluq)
- **ironwail**: Original upstream Ironwail
- **quakespasm**: Original upstream QuakeSpasm (ancestor of Ironwail)
- **quakespasm-pluq**: QuakeSpasm + PluQ

## Quick Reference

```bash
cd Quake && make -j4                  # Build backend
cd tests && make -j4                  # Build tests
```

## Documentation

| Topic | Location |
|-------|----------|
| Build instructions | `BUILD_INSTRUCTIONS.md` |
| PluQ architecture | `CLAUDE_Docs/PLUQ.md` |
| IPC implementation | `CLAUDE_Docs/IPC_IMPLEMENTATION_STATUS.md` |
| Resource streaming | `CLAUDE_Docs/RESOURCE_STREAMING.md` |
| Test programs | `tests/README.md` |
| Windows build | `Windows/PLUQ_BUILD.md` |
| IPC definitions | `Quake/pluq.h` |
| Message schema | `Quake/pluq.fbs` |

## Commit Guidelines

- Do not add Claude info to commit messages (no "Generated with Claude Code" or "Co-Authored-By: Claude" footers)

## Headless Mode

**Intent**: Headless mode (`-headless`) should act like a regular Quake client, loading all resources normally but skipping actual display and audio output. Resources must still be loaded to be forwarded to the frontend via IPC.

**Current Limitation**: Headless mode is incomplete. Loading maps crashes because GL calls still execute but `gl_max_texture_size` is 0 without OpenGL initialization. A proper fix requires stubbing more GL functions or using a virtual framebuffer (Xvfb).

**Useful Flags**:
- `-conout`: Unbuffered console output to stdout (useful for headless/scripted usage)
- `-condebug`: Writes console output to `qconsole.log`

## CLAUDE_Docs Folder

Place temporary dev coordination files (status, design notes, progress) in `CLAUDE_Docs/`.
- Do not care for old pluq code, we are in initial development and anything can be changed. No backward compatibility or holding with existing pluq patterns. only the vanilla quake source should be kept as is as much as possible and serve as an design template.