/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// stubs_pluq_frontend.c -- Minimal stubs for PluQ frontend
// Only stubs functions from excluded files: host_cmd.c, cl_parse.c
// Maximizes code reuse from backend

#include "quakedef.h"

// ============================================================================
// Stubs for host_cmd.c (excluded - server/host commands not needed)
// ============================================================================

// Variables from host_cmd.c
qboolean noclip_anglehack = false;

// Completion lists from host_cmd.c
filelist_item_t *extralevels = NULL;
filelist_item_t **extralevels_sorted = NULL;
filelist_item_t *modlist = NULL;
filelist_item_t *demolist = NULL;
filelist_item_t *savelist = NULL;
filelist_item_t *skylist = NULL;

// Map/mod management stubs
const char *ExtraMaps_GetMessage (const filelist_item_t *item) { return ""; }
maptype_t ExtraMaps_GetType (const filelist_item_t *item) { return 0; }
qboolean ExtraMaps_IsStart (maptype_t type) { return false; }
void ExtraMaps_Clear (void) {}
void ExtraMaps_Init (void) {}
void ExtraMaps_ShutDown (void) {}

// Mod management stubs
const char *Modlist_GetFullName (const filelist_item_t *item) { return ""; }
const char *Modlist_GetDescription (const filelist_item_t *item) { return ""; }
const char *Modlist_GetAuthor (const filelist_item_t *item) { return ""; }
const char *Modlist_GetDate (const filelist_item_t *item) { return ""; }
double Modlist_GetDownloadSize (const filelist_item_t *item) { return 0.0; }
modstatus_t Modlist_GetStatus (const filelist_item_t *item) { return 0; }
float Modlist_GetDownloadProgress (const filelist_item_t *item) { return 0.0f; }
qboolean Modlist_IsInstalling (void) { return false; }
qboolean Modlist_StartInstalling (const filelist_item_t *item) { return false; }
void Modlist_Init (void) {}
void Modlist_ShutDown (void) {}

// List management stubs
void DemoList_Init (void) {}
void DemoList_Rebuild (void) {}
void SaveList_Init (void) {}
void SaveList_Rebuild (void) {}
void SkyList_Init (void) {}
void SkyList_Rebuild (void) {}
// M_CheckMods is in menu.o

// Save/load stubs
qboolean Host_IsSaving (void) { return false; }
void Host_WaitForSaveThread (void) {}
void Host_ShutdownSave (void) {}
void Host_BackgroundSave (const char *name) {}

// Demo management stubs
void Host_Resetdemos (void) {}

// ============================================================================
// Stubs for cl_parse.c (excluded - network parsing handled by PluQ)
// ============================================================================

// Main parsing function - PluQ handles this via PluQ_ApplyReceivedState()
void CL_ParseServerMessage (void)
{
	// Frontend receives world state via PluQ, not network messages
	// This function is called from cl_main.c but not used in frontend mode
}

// Network message keeping stub
void CL_KeepaliveMessage (void) {}

// Note: CL_ClearSignons is in cl_demo.o
// Note: CL_PrintEntities_f is in cl_main.o

// ============================================================================
// Global variables needed by included files
// ============================================================================

// Server globals (needed by various client code for null checks)
server_t sv = {0};
server_static_t svs = {0};
client_t *host_client = NULL;
edict_t *sv_player = NULL;
THREAD_LOCAL globalvars_t *pr_global_struct = NULL;

// Time variables (managed by main loop)
double realtime = 0;
double host_frametime = 0;
double host_rawframetime = 0;
double host_timescale = 1.0;
int host_framecount = 0;
double host_netinterval = 0.013; // ~72 FPS

// Host variables
quakeparms_t *host_parms = NULL;
qboolean host_initialized = false;
byte *host_colormap = NULL;
int minimum_memory = 0x1000000; // 16 MB

// Development/Debug variables
cvar_t developer = {"developer", "0", CVAR_NONE};
cvar_t map_checks = {"map_checks", "0", CVAR_NONE};
cvar_t devstats = {"devstats", "0", CVAR_NONE};
devstats_t dev_stats = {0};
devstats_t dev_peakstats = {0};
overflowtimes_t dev_overflows = {0};

// Network variables (unused in frontend but referenced)
sizebuf_t net_message = {0};
cvar_t max_edicts = {"max_edicts", "8192", CVAR_NONE};

// Server cvars (unused but referenced by client code)
cvar_t sv_gravity = {"sv_gravity", "800", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t sv_maxspeed = {"sv_maxspeed", "320", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t hostname = {"hostname", "PluQ Frontend", CVAR_SERVERINFO};
cvar_t teamplay = {"teamplay", "0", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t pausable = {"pausable", "0", CVAR_NONE}; // Frontend doesn't control pause
cvar_t nomonsters = {"nomonsters", "0", CVAR_NONE};
cvar_t skill = {"skill", "1", CVAR_NONE};
cvar_t coop = {"coop", "0", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t fraglimit = {"fraglimit", "0", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t timelimit = {"timelimit", "0", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t host_maxfps = {"host_maxfps", "72", CVAR_ARCHIVE};

// Network globals (unused but referenced)
qboolean tcpipAvailable = false;
qboolean ipxAvailable = false;
char my_tcpip_address[64] = "PluQ Frontend";
char my_ipx_address[64] = "";
int net_activeconnections = 0;
double net_time = 0.0;
int net_hostport = 26000;
int DEFAULTnet_hostport = 26000;
qboolean slistInProgress = false;
qboolean slistSilent = false;
qboolean slistLocal = true;
int hostCacheCount = 0;

// QuakeC VM (not used in frontend)
THREAD_LOCAL qcvm_t *qcvm = NULL;

// ============================================================================
// Minimal stubs for server/network/QuakeC functions
// (These are called by included files but not needed in frontend)
// ============================================================================

// Server stubs
void SV_Init (void) {}
void SV_Shutdown (void) {}
void SV_Frame (void) {}
void SV_Physics (void) {}
void SV_DropClient (qboolean crash) {}
void SV_BroadcastPrintf (const char *fmt, ...) {}
void SV_ClientPrintf (const char *fmt, ...) {}
void SV_WriteClientdataToMessage (edict_t *ent, sizebuf_t *msg) {}
void SV_ShutdownServer (qboolean crash) {}
qboolean SV_FilterPacket (void) { return false; }
void SV_ReadClientMessage (void) {}
void SV_RunClients (void) {}
void SV_SaveSpawnparms (void) {}
void SV_SpawnServer (const char *server) {}
void SV_ClearDatagram (void) {}
void SV_SendClientMessages (void) {}
void SV_LinkEdict (edict_t *ent, qboolean touch_triggers) {}
qboolean SV_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace) { return true; }
qboolean SV_EdictInPVS (edict_t *ent) { return false; }
qboolean SV_BoxInPVS (vec3_t mins, vec3_t maxs) { return false; }
byte *SV_FatPVS (vec3_t org) { return NULL; }
void SV_CheckVelocity (edict_t *ent) {}
void SV_AddGravity (edict_t *ent) {}
void SV_Impact (edict_t *e1, edict_t *e2) {}
void SV_ClipMoveToEntity (edict_t *ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end) {}

// Network stubs
void NET_Init (void) {}
void NET_Shutdown (void) {}
void NET_Poll (void) {}
int NET_SendMessage (struct qsocket_s *sock, sizebuf_t *data) { return 0; }
int NET_SendUnreliableMessage (struct qsocket_s *sock, sizebuf_t *data) { return 0; }
qboolean NET_CanSendMessage (struct qsocket_s *sock) { return false; }
int NET_GetMessage (struct qsocket_s *sock) { return 0; }
struct qsocket_s *NET_Connect (const char *host) { return NULL; }
void NET_Close (struct qsocket_s *sock) {}
double NET_QSocketGetTime (const struct qsocket_s *sock) { return 0.0; }
const char *NET_QSocketGetAddressString (const struct qsocket_s *sock) { return "PluQ"; }
void NET_Slist_f (void) {}
const char *NET_SlistPrintServer (int idx) { return ""; }
const char *NET_SlistPrintServerName (int idx) { return ""; }
void NET_SlistSort (void) {}

// QuakeC/Progs stubs
void PR_Init (void) {}
void PR_Shutdown (void) {}
qboolean PR_LoadProgs (const char *filename, qboolean fatal) { return false; }
void PR_SwitchQCVM (qcvm_t *vm) {}
void PR_PushQCVM (qcvm_t *newvm, qcvm_t **oldvm) { if (oldvm) *oldvm = NULL; }
void PR_PopQCVM (qcvm_t *oldvm) {}
string_t PR_SetEngineString (const char *s) { return 0; }
const char *PR_GetString (string_t num) { return ""; }
void PR_ClearProgs (qcvm_t *vm) {}
void PR_ExecuteProgram (func_t fnum) {}
void PR_ReloadPics (qboolean no_overrides) {}
void PR_AutoCvarChanged (cvar_t *var) {}
const char *ED_FieldValueString (edict_t *ed, ddef_t *d) { return ""; }
qboolean ED_IsRelevantField (edict_t *ed, ddef_t *d) { return false; }
void ED_Write (savedata_t *save, edict_t *ed) {}
void ED_ClearEdict (edict_t *e) {}
const char *ED_ParseEdict (const char *data, edict_t *ent) { return data; }
const char *ED_ParseGlobals (const char *data) { return data; }
eval_t *GetEdictFieldValueByName(edict_t *ed, const char *name) { return NULL; }
edict_t *EDICT_NUM(int n) { return NULL; }
int NUM_FOR_EDICT(edict_t *e) { return 0; }

// Host stubs
void Host_ServerFrame (void) {}
void Host_ClearMemory (void) {}
void Host_InitLocal (void) {}
void Host_Shutdown (void) {}
void Host_ShutdownServer (qboolean crash) {}
void Host_WriteConfiguration (void) {}
void Host_InvokeOnMainThread (void (*func)(void *), void *data) {}
void Host_Quit_f (void) { Sys_Quit(); }
void Host_EndGame (const char *message, ...)
{
	va_list argptr;
	char text[1024];
	va_start (argptr, message);
	q_vsnprintf (text, sizeof(text), message, argptr);
	va_end (argptr);
	Sys_Error ("Game Ended: %s", text);
}
void Host_ReportError (const char *fmt, ...)
{
	va_list argptr;
	char text[1024];
	va_start (argptr, fmt);
	q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);
	Sys_Error ("FATAL ERROR: %s", text);
}
double Host_GetFrameInterval (void) { return 1.0 / 72.0; }

// Save/load stubs
void SaveData_Clear (savedata_t *save) {}
void SaveData_Init (savedata_t *save) {}
void SaveData_WriteHeader (savedata_t *save) {}
void SaveData_Fill (savedata_t *save) {}

// Platform stubs
void PL_SetWindowIcon (void) {}
void PL_VID_Shutdown (void) {}
void PL_ErrorDialog (const char *text) {}

// Input stubs (defined in in_sdl.c, which is included in frontend build)
// void IN_Init (void) {}  // Now in in_sdl.c
// void IN_Shutdown (void) {}  // Now in in_sdl.c
// void IN_Commands (void) {}  // Now in in_sdl.c
// void IN_Move (usercmd_t *cmd) {}  // Now in in_sdl.c

// Texture manager stubs (defined in gl_texmgr.c, which is included in frontend build)
// void TexMgr_Init (void) {}  // Now in gl_texmgr.c

// Drawing stubs (defined in gl_draw.c, which is included in frontend build)
// void Draw_Init (void) {}  // Now in gl_draw.c

// Screen stubs (defined in gl_screen.c, which is included in frontend build)
// void SCR_Init (void) {}  // Now in gl_screen.c

// Renderer stubs (defined in gl_rmisc.c, which is included in frontend build)
// void R_Init (void) {}  // Now in gl_rmisc.c

// Status bar stubs (defined in sbar.c, which is included in frontend build)
// void Sbar_Init (void) {}  // Now in sbar.c

// Sound stubs (defined in snd_dma.c, which is included in frontend build)
// void S_Init (void) {}  // Now in snd_dma.c
// void S_Shutdown (void) {}  // Now in snd_dma.c

// CD Audio stubs (defined in cd_null.c, which is included in frontend build)
// int CDAudio_Init (void) { return 0; }  // Now in cd_null.c

// Background music stubs (defined in bgmusic.c, which is included in frontend build)
// void BGM_Init (void) {}  // Now in bgmusic.c

// Client stubs (defined in cl_main.c, which is included in frontend build)
// void CL_Init (void) {}  // Now in cl_main.c

// Menu stubs (defined in menu.c, which is included in frontend build)
// void M_Init (void) {}  // Now in menu.c

// Video stubs (defined in gl_vidsdl.c, which is included in frontend build)
// void VID_Init (void) {}  // Now in gl_vidsdl.c
// void VID_Shutdown (void) {}  // Now in gl_vidsdl.c

// Chase camera stubs (defined in chase.c, which is included in frontend build)
// void Chase_Init (void) {}  // Now in chase.c

// View stubs (defined in view.c, which is included in frontend build)
// void V_Init (void) {}  // Now in view.c

// Client state variables (defined in cl_main.c, which is included in frontend build)
// NOTE: These were previously defined here but caused duplicate symbol errors
// since cl_main.o is included in the frontend build. Removed to fix linking.
// client_static_t cls;  // Now in cl_main.c
// client_state_t cl;    // Now in cl_main.c

// Screen variables (defined in gl_screen.c, which is included in frontend build)
// NOTE: This was previously defined here but caused duplicate symbol errors
// since gl_screen.o is included in the frontend build. Removed to fix linking.
// qboolean scr_disabled_for_loading = false;  // Now in gl_screen.c

// Steam stubs (defined in steam.c, which is included in frontend build)
// qboolean Steam_IsValidPath (const char *path) { return false; }  // Now in steam.c
// const char *Steam_FindGame (int appid) { return NULL; }  // Now in steam.c
// const char *Steam_ResolvePath (const char *path) { return path; }  // Now in steam.c

// Input event stubs (defined in in_sdl.c, which is included in frontend build)
// void IN_SendKeyEvents (void) {}  // Now in in_sdl.c

// Quake flavor selector stub (defined in steam.c, which is included in frontend build)
// int ChooseQuakeFlavor (void) { return 0; }  // Now in steam.c

// Epic Games Store stubs (defined in steam.c, which is included in frontend build)
// const char *EGS_FindGame (int appid) { return NULL; }  // Now in steam.c

// Additional Steam stubs (defined in steam.c, which is included in frontend build)
// void Steam_Init (void) {}  // Now in steam.c

// Background music stubs (defined in bgmusic.c, which is included in frontend build)
// void BGM_Stop (void) {}  // Now in bgmusic.c

// Client disconnect stub (defined in cl_main.c, which is included in frontend build)
// void CL_Disconnect (void) {}  // Now in cl_main.c

// Model management stubs (defined in gl_model.c, which is included in frontend build)
// void Mod_ResetAll (void) {}  // Now in gl_model.c

// Sky management stubs (defined in gl_sky.c, which is included in frontend build)
// void Sky_ClearAll (void) {}  // Now in gl_sky.c

// Menu stubs (additional) - All now defined in menu.c, which is included in frontend build
// void M_CheckMods (void) {}  // Now in menu.c
// void M_Menu_Main_f (void) {}  // Now in menu.c
// void M_ToggleMenu_f (void) {}  // Now in menu.c
// qboolean M_WantsConsole (float *alpha) { if (alpha) *alpha = 0.0f; return false; }  // Now in menu.c
// qboolean M_WaitingForKeyBinding (void) { return false; }  // Now in menu.c
// void M_Keydown (int key) {}  // Now in menu.c
// void M_Charinput (int key) {}  // Now in menu.c
// enum textmode_t M_TextEntry (void) { return TEXTMODE_OFF; }  // Now in menu.c
// void M_PrintWhite (int cx, int cy, const char *str) {}  // Now in menu.c

// Video management stubs (defined in gl_vidsdl.c, which is included in frontend build)
// void VID_Lock (void) {}  // Now in gl_vidsdl.c
// void VID_Toggle (void) {}  // Now in gl_vidsdl.c
// void VID_SetMouseCursor (mousecursor_t cursor) {}  // Now in gl_vidsdl.c

// Texture manager stubs (defined in gl_texmgr.c, which is included in frontend build)
// void TexMgr_NewGame (void) {}  // Now in gl_texmgr.c
// void TexMgr_FreeTexturesForOwner (qmodel_t *owner) {}  // Now in gl_texmgr.c

// Drawing stubs (defined in gl_draw.c, which is included in frontend build)
// void Draw_NewGame (void) {}  // Now in gl_draw.c
// void Draw_Character (int x, int y, int num) {}  // Now in gl_draw.c
// void Draw_String (int x, int y, const char *str) {}  // Now in gl_draw.c
// void Draw_Fill (int x, int y, int w, int h, int c, float alpha) {}  // Now in gl_draw.c
// void Draw_Pic (int x, int y, qpic_t *pic) {}  // Now in gl_draw.c
// void Draw_ConsoleBackground (void) {}  // Now in gl_draw.c
// void Draw_GetCanvasTransform (canvastype canvas, drawtransform_t *transform) {}  // Now in gl_draw.c

// Renderer stubs (defined in gl_rmisc.c, which is included in frontend build)
// void R_NewGame (void) {}  // Now in gl_rmisc.c

// Screen stubs (defined in gl_screen.c, which is included in frontend build)
// void SCR_UpdateScreen (void) {}  // Now in gl_screen.c
// void SCR_EndLoadingPlaque (void) {}  // Now in gl_screen.c

// Sound stubs (defined in snd_dma.c, which is included in frontend build)
// void S_LocalSound (const char *sound) {}  // Now in snd_dma.c

// OpenGL canvas functions (defined in gl_draw.c, which is included in frontend build)
// void GL_SetCanvas (canvastype canvas) {}  // Now in gl_draw.c
// void GL_SetCanvasColor (float r, float g, float b, float a) {}  // Now in gl_draw.c
// void GL_PushCanvasColor (float r, float g, float b, float a) {}  // Now in gl_draw.c
// void GL_PopCanvasColor (void) {}  // Now in gl_draw.c

// Input stubs (defined in in_sdl.c, which is included in frontend build)
// void IN_Activate (void) {}  // Now in in_sdl.c
// void IN_DeactivateForConsole (void) {}  // Now in in_sdl.c
// void IN_UpdateInputMode (void) {}  // Now in in_sdl.c
// qboolean IN_EmulatedCharEvents (void) { return false; }  // Now in in_sdl.c
// gamepadtype_t IN_GetGamepadType (void) { return GAMEPAD_NONE; }  // Now in in_sdl.c

// Platform stubs (additional)
char *PL_GetClipboardData (void) { return NULL; }

// Config variables (defined in cl_main.c, which is included in frontend build)
// cvar_t cfg_unbindall = {"cfg_unbindall","1",CVAR_ARCHIVE};  // Now in cl_main.c

// Global variables - rendering/screen related (defined in gl_screen.c, which is included in frontend build)
// int glwidth = 640;           // Now in gl_screen.c
// int glheight = 480;          // Now in gl_screen.c
// int glx = 0;                 // Now in gl_screen.c
// int gly = 0;                 // Now in gl_screen.c
// int clearnotify = 0;         // Now in gl_screen.c
// int scr_tileclear_updates = 0;  // Now in gl_screen.c

// Video definition (defined in gl_vidsdl.c, which is included in frontend build)
// viddef_t vid;  // Now in gl_vidsdl.c

// Renderer definition (defined in gl_rmain.c, which is included in frontend build)
// refdef_t r_refdef;  // Now in gl_rmain.c

// Pics (defined in gl_draw.c, which is included in frontend build)
// qpic_t *pic_ins = NULL;  // Now in gl_draw.c
// qpic_t *pic_ovr = NULL;  // Now in gl_draw.c

// SIMD flag (defined in gl_rmisc.c, which is included in frontend build)
// qboolean use_simd = false;  // Now in gl_rmisc.c

