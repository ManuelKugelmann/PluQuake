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

// stubs_pluq_frontend.c -- Rendering/Audio/Input stubs for headless PluQ test frontend
// Provides stubs for client-side rendering/audio/input not needed by test frontend
// Used ONLY by test frontend (headless), NOT by production frontend

#include "quakedef.h"

// ============================================================================
// Rendering/Audio/Input stubs for headless test frontend
// ============================================================================

// Platform stubs
void PL_SetWindowIcon (void) {}
void PL_VID_Shutdown (void) {}
void PL_ErrorDialog (const char *text) {}

// Input stubs
void IN_Init (void) {}
void IN_Shutdown (void) {}
void IN_Commands (void) {}
void IN_Move (usercmd_t *cmd) {}

// Texture manager stubs
void TexMgr_Init (void) {}

// Drawing stubs
void Draw_Init (void) {}

// Screen stubs
void SCR_Init (void) {}

// Renderer stubs
void R_Init (void) {}

// Status bar stubs
void Sbar_Init (void) {}

// Sound stubs
void S_Init (void) {}
void S_Shutdown (void) {}

// CD Audio stubs
int CDAudio_Init (void) { return 0; }

// Background music stubs
void BGM_Init (void) {}

// Client stubs (beyond what's already stubbed)
void CL_Init (void) {}

// Menu stubs
void M_Init (void) {}

// Video stubs
void VID_Init (void) {}
void VID_Shutdown (void) {}

// Chase camera stubs
void Chase_Init (void) {}

// View stubs
void V_Init (void) {}

// Client state variables (defined, not declared - declaration is in client.h)
client_static_t cls;
client_state_t cl;

// Screen variables (defined, not declared - declaration is elsewhere)
qboolean scr_disabled_for_loading = false;

// Steam stubs
qboolean Steam_IsValidPath (const char *path) { return false; }
const char *Steam_FindGame (int appid) { return NULL; }
const char *Steam_ResolvePath (const char *path) { return path; }

// Input event stubs
void IN_SendKeyEvents (void) {}

// Quake flavor selector stub
int ChooseQuakeFlavor (void) { return 0; }

// Epic Games Store stubs
const char *EGS_FindGame (int appid) { return NULL; }

// Additional Steam stubs
void Steam_Init (void) {}

// Background music stubs (additional)
void BGM_Stop (void) {}

// Client disconnect stub
void CL_Disconnect (void) {}

// Model management stubs
void Mod_ResetAll (void) {}

// Sky management stubs
void Sky_ClearAll (void) {}

// Menu stubs (additional)
void M_CheckMods (void) {}
void M_Menu_Main_f (void) {}
void M_ToggleMenu_f (void) {}
qboolean M_WantsConsole (float *alpha) { if (alpha) *alpha = 0.0f; return false; }
qboolean M_WaitingForKeyBinding (void) { return false; }
void M_Keydown (int key) {}
void M_Charinput (int key) {}
enum textmode_t M_TextEntry (void) { return TEXTMODE_OFF; }
void M_PrintWhite (int cx, int cy, const char *str) {}

// Video management stubs (additional)
void VID_Lock (void) {}
void VID_Toggle (void) {}
void VID_SetMouseCursor (mousecursor_t cursor) {}

// Texture manager stubs (additional)
void TexMgr_NewGame (void) {}
void TexMgr_FreeTexturesForOwner (qmodel_t *owner) {}

// Drawing stubs (additional)
void Draw_NewGame (void) {}
void Draw_Character (int x, int y, int num) {}
void Draw_String (int x, int y, const char *str) {}
void Draw_Fill (int x, int y, int w, int h, int c, float alpha) {}
void Draw_Pic (int x, int y, qpic_t *pic) {}
void Draw_ConsoleBackground (void) {}
void Draw_GetCanvasTransform (canvastype canvas, drawtransform_t *transform) {}

// Renderer stubs (additional)
void R_NewGame (void) {}

// Screen stubs (additional)
void SCR_UpdateScreen (void) {}
void SCR_EndLoadingPlaque (void) {}

// Sound stubs (additional)
void S_LocalSound (const char *sound) {}

// OpenGL canvas stubs
void GL_SetCanvas (canvastype canvas) {}
void GL_SetCanvasColor (float r, float g, float b, float a) {}
void GL_PushCanvasColor (float r, float g, float b, float a) {}
void GL_PopCanvasColor (void) {}

// Input stubs (additional)
void IN_Activate (void) {}
void IN_DeactivateForConsole (void) {}
void IN_UpdateInputMode (void) {}
qboolean IN_EmulatedCharEvents (void) { return false; }
gamepadtype_t IN_GetGamepadType (void) { return GAMEPAD_NONE; }

// Platform stubs (additional)
char *PL_GetClipboardData (void) { return NULL; }

// Config variables (declared in client.h as cvar_t, defined here)
cvar_t cfg_unbindall = {"cfg_unbindall","1",CVAR_ARCHIVE};

// Global variables - rendering/screen related
int glwidth = 640;
int glheight = 480;
int glx = 0;
int gly = 0;
int clearnotify = 0;
int scr_tileclear_updates = 0;

// Video definition (declared in vid.h, defined here)
viddef_t vid;

// Renderer definition (declared in render.h/glquake.h, defined here)
refdef_t r_refdef;

// Pics (qpic_t pointers declared in draw.h, defined here as NULL)
qpic_t *pic_ins = NULL;
qpic_t *pic_ovr = NULL;

// SIMD flag
qboolean use_simd = false;
