/*
Copyright (C) 2014 Jeffrey Quesnelle

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef _RSP_HLE_STATIC_H
#define _RSP_HLE_STATIC_H 

#include "m64p_types.h"

#ifdef __cplusplus
extern "C" {
#endif

EXPORT m64p_error CALL PluginGetVersionRSP(m64p_plugin_type *PluginType, int *PluginVersion, int *APIVersion, const char **PluginNamePtr, int *Capabilities);
EXPORT m64p_error CALL PluginStartupRSP(m64p_dynlib_handle, void *, void (*)(void *, int, const char *));
EXPORT m64p_error CALL PluginShutdownRSP(void);
EXPORT void CALL RomClosedRSP(void);

#ifdef __cplusplus
}
#endif

#endif