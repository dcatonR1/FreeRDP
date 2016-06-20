/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Windows Monitor Handling
 *
 * Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2016 Don Caton <dcaton1220@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __WF_MONITOR_H
#define __WF_MONITOR_H

#include <freerdp/api.h>
#include <freerdp/freerdp.h>

struct _MONITOR_INFO
{
	RECTANGLE_16 area;
	RECTANGLE_16 workarea;
	BOOL primary;
};
typedef struct _MONITOR_INFO MONITOR_INFO;

// No particular reason for this, it's just what the Linix version uses.
// Windows XP had a limit of nine.  Later versions don't specify a maximum.
#define WF_MAX_MONITORS 16

struct _VIRTUAL_SCREEN
{
	int nmonitors;
	RECTANGLE_16 area;
	RECTANGLE_16 workarea;
	MONITOR_INFO monitors[WF_MAX_MONITORS];
};
typedef struct _VIRTUAL_SCREEN VIRTUAL_SCREEN;

#include "wf_client.h"

FREERDP_API int wf_list_monitors(wfContext* wfc);
FREERDP_API BOOL wf_detect_monitors(wfContext* wfc, UINT32* pWidth, UINT32* pHeight);
FREERDP_API void wf_monitors_free(wfContext* wfc);

#endif /* __WF_MONITOR_H */
