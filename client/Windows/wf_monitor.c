/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * X11 Monitor Handling
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freerdp/log.h>

#define TAG CLIENT_TAG("windows")

#include "wf_monitor.h"

/* See MSDN Section on Multiple Display Monitors: http://msdn.microsoft.com/en-us/library/dd145071 */

struct _MonitorDevice {
    //wstring cardName;
    //wstring deviceName;
    //wstring cardType;
    //wstring deviceType;
    int x;
    int y;
    int width;
    int height;
    BOOL isPrimary;
    BOOL isDisabled;
    BOOL isSLM;
};

typedef struct _MonitorDevice MonitorDevice;

int getMonitorInfo( MONITOR_INFO * monitorInfo, int * primaryMonitor )
{
    DISPLAY_DEVICE dd;
    dd.cb = sizeof( dd );

    int monitorCount = 0;

    DWORD dev = 0; // device index

    int id = 1; // monitor number, as used by Display Properties > Settings

    if (primaryMonitor)
    {
        * primaryMonitor = 0;
    }

    while ( EnumDisplayDevices( 0, dev, &dd, 0 ) )
    {
        if ( ( dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP ) && ! ( dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER ) )
        {
            DISPLAY_DEVICE ddMon;
            ZeroMemory( &ddMon, sizeof( ddMon ) );
            ddMon.cb = sizeof( ddMon );
            DWORD devMon = 0;

            while (EnumDisplayDevices( dd.DeviceName, devMon, &ddMon, 0 ))
            {
                if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE)
                {
                    break;
                }

                devMon++;
            }

            //if (!*ddMon.DeviceString)
            //{
            //    EnumDisplayDevices( dd.DeviceName, NULL, &ddMon, NULL );

            //    if (!*ddMon.DeviceString)
            //    {
            //        lstrcpy( ddMon.DeviceString, _T( "Default Monitor" ) );
            //    }
            //}

            // get information about the display's position and the current display mode
            DEVMODE dm;
            ZeroMemory( &dm, sizeof( dm ) );
            dm.dmSize = sizeof( dm );

            if (EnumDisplaySettingsEx( dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0 ) == FALSE)
            {
                EnumDisplaySettingsEx( dd.DeviceName, ENUM_REGISTRY_SETTINGS, &dm, 0 );
            }

            // get the monitor handle and workspace
            HMONITOR hm = 0;
            MONITORINFO mi;

            ZeroMemory( &mi, sizeof( mi ) );
            mi.cbSize = sizeof( mi );

            POINT pt = { dm.dmPosition.x, dm.dmPosition.y };
            hm = MonitorFromPoint( pt, MONITOR_DEFAULTTONULL );
            if (hm)
            {
                GetMonitorInfo( hm, &mi );
            }

            // width x height @ x,y - bpp - refresh rate
            // note that refresh rate information is not available on Win9x
            //wsprintf( buf, _T( "%d x %d @ %d,%d - %d-bit - %d Hz\r\n" ), dm.dmPelsWidth, dm.dmPelsHeight,
            //    dm.dmPosition.x, dm.dmPosition.y, dm.dmBitsPerPel, dm.dmDisplayFrequency );

            if (hm)
            {
                // workspace and monitor handle

                // workspace: x,y - x,y HMONITOR: handle
                //wsprintf( buf, _T( "workspace: %d,%d - %d,%d HMONITOR: 0x%X\r\n" ), mi.rcWork.left,
                //    mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom, hm );

                //lstrcat( msg, buf );
            }

            id++;

            monitorInfo[monitorCount].area.left = (UINT16) dm.dmPosition.x;
            monitorInfo[monitorCount].area.top = (UINT16) dm.dmPosition.y;
            monitorInfo[monitorCount].area.right = (UINT16) ( dm.dmPosition.x + dm.dmPelsWidth );
            monitorInfo[monitorCount].area.bottom = (UINT16) ( dm.dmPosition.y + dm.dmPelsHeight );
            monitorInfo[monitorCount].workarea.left = (UINT16) mi.rcWork.left;
            monitorInfo[monitorCount].workarea.top = (UINT16) mi.rcWork.top;
            monitorInfo[monitorCount].workarea.right = (UINT16) mi.rcWork.right;
            monitorInfo[monitorCount].workarea.bottom = (UINT16) mi.rcWork.bottom;
            monitorInfo[monitorCount].primary = ( dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE ) != 0;

            if (primaryMonitor && monitorInfo[monitorCount].primary )
            {
                *primaryMonitor = monitorCount;
            }

            monitorCount++;
        }

        //if (!( thisDev.deviceType.length() == 0 )) // Don't list a nonexistent device.
        //    displays.push_back( thisDev );

        dev++;
    }

    return monitorCount;
}

int wf_list_monitors( wfContext* wfc )
{
    MONITOR_INFO mi[WF_MAX_MONITORS];

    int monitorCount = getMonitorInfo( mi, NULL );

    for (int i = 0; i < monitorCount; i++)
    {
        printf( "      %s [%d] %dx%d\t+%d+%d\n", mi[i].primary ? "*" : " ", i, mi[i].area.right - mi[i].area.left, mi[i].area.bottom - mi[i].area.top, mi[i].area.top, mi[i].area.left );
    }

	return 0;
}

/* Was monitor requested on command line using /monitors:<0,1,2...> */
static BOOL wf_is_monitor_id_active(wfContext* wfc, UINT32 id)
{
	rdpSettings* settings = wfc->settings;

    BOOL useThisMonitor = FALSE;

    if (settings->NumMonitorIds == 0)  // switch not specified, use all monitors
    {
        useThisMonitor = TRUE;
    }
    else
    {
        for (UINT32 index = 0; index < settings->NumMonitorIds; index++)
        {
            if (settings->MonitorIds[index] == id)
            {
                useThisMonitor = TRUE;
                break;
            }
        }
    }

	return useThisMonitor;
}

BOOL wf_detect_monitors(wfContext* wfc, UINT32* pMaxWidth, UINT32* pMaxHeight)
{
	int i;
	int nmonitors = 0;
	int primaryMonitorFound = FALSE;
	VIRTUAL_SCREEN* vscreen;
	rdpSettings* settings = wfc->settings;

    int mouse_x, mouse_y; // , _dummy_i;
	//Window _dummy_w;
	int current_monitor = 0;

	vscreen = &wfc->vscreen;
	*pMaxWidth = settings->DesktopWidth;
	*pMaxHeight = settings->DesktopHeight;

	/* get mouse location */

#if 0
	if (!XQueryPointer(wfc->display, DefaultRootWindow(wfc->display), &_dummy_w, &_dummy_w, &mouse_x, &mouse_y,	&_dummy_i, &_dummy_i, (void *) &_dummy_i))
		mouse_x = mouse_y = 0;
#endif

    POINT cursorPos;

    if (GetCursorPos( &cursorPos ) != 0)
    {
        mouse_x = cursorPos.x;
        mouse_y = cursorPos.y;
    }
    else
    {
        mouse_x = mouse_y = 0;
    }

    int primaryMonitor = 0;
    vscreen->nmonitors = getMonitorInfo( vscreen->monitors, &primaryMonitor );

    if (vscreen->nmonitors > WF_MAX_MONITORS)  // unlikely, but...
    {
        // TODO:
        // This is somewhat crappy.  In the unlikely event there's more than 16 monitors, this
        // will fail silently.  Should add a log message here.
        vscreen->nmonitors = 0;
    }

	if (vscreen->nmonitors)
	{
		for (i = 0; i < vscreen->nmonitors; i++)
		{
			/* Determine which monitor that the mouse cursor is on */
            if (( mouse_x >= vscreen->monitors[i].area.left ) &&
                ( mouse_x <= vscreen->monitors[i].area.right ) &&
                ( mouse_y >= vscreen->monitors[i].area.top ) &&
                ( mouse_y <= vscreen->monitors[i].area.bottom ))
            {
                current_monitor = i;
                break;
            }
		}
	}

	wfc->fullscreenMonitors.top = wfc->fullscreenMonitors.bottom =
	wfc->fullscreenMonitors.left = wfc->fullscreenMonitors.right = 0;

	/* WORKAROUND: With Remote Application Mode - using NET_WM_WORKAREA
 	 * causes issues with the ability to fully size the window vertically
 	 * (the bottom of the window area is never updated). So, we just set
 	 * the workArea to match the full Screen width/height.
	 */
    // Linux code...
	//if (settings->RemoteApplicationMode || !wf_GetWorkArea(wfc))
	//{
	//	wfc->workArea.x = 0;
	//	wfc->workArea.y = 0;
	//	wfc->workArea.width = abs( vscreen->monitors[primaryMonitor].area.right - vscreen->monitors[primaryMonitor].area.left );
	//	wfc->workArea.height = abs( vscreen->monitors[primaryMonitor].area.bottom - vscreen->monitors[primaryMonitor].area.top );
	//}

    // The Linux code seems to get the workarea for the primary monitor and if it can't
    //if (settings->RemoteApplicationMode || !wf_GetWorkArea(wfc))
    //{
    //xfc->workArea.x = 0;
    //xfc->workArea.y = 0;
    //xfc->workArea.width = WidthOfScreen( xfc->screen );
    //xfc->workArea.height = HeightOfScreen( xfc->screen );
    //}

    wfc->workArea.x = vscreen->monitors[primaryMonitor].workarea.right;
    wfc->workArea.y = vscreen->monitors[primaryMonitor].workarea.top;
    wfc->workArea.width = abs( vscreen->monitors[primaryMonitor].workarea.right - vscreen->monitors[primaryMonitor].workarea.left );
    wfc->workArea.height = abs( vscreen->monitors[primaryMonitor].workarea.bottom - vscreen->monitors[primaryMonitor].workarea.top );


	if (settings->Fullscreen)
	{
		*pMaxWidth = abs( vscreen->monitors[primaryMonitor].area.right - vscreen->monitors[primaryMonitor].area.left );
		*pMaxHeight = abs( vscreen->monitors[primaryMonitor].area.bottom - vscreen->monitors[primaryMonitor].area.top );
	}
	else if (settings->Workarea)
	{
		*pMaxWidth = wfc->workArea.width;
		*pMaxHeight = wfc->workArea.height;
	}
	else if (settings->PercentScreen)
	{
		*pMaxWidth = (wfc->workArea.width * settings->PercentScreen) / 100;
		*pMaxHeight = (wfc->workArea.height * settings->PercentScreen) / 100;

		/* If we have specific monitor information then limit the PercentScreen value
		 * to only affect the current monitor vs. the entire desktop
		 */
		if (vscreen->nmonitors > 0)
		{
			*pMaxWidth = ((vscreen->monitors[current_monitor].area.right - vscreen->monitors[current_monitor].area.left + 1) * settings->PercentScreen) / 100;
			*pMaxHeight = ((vscreen->monitors[current_monitor].area.bottom - vscreen->monitors[current_monitor].area.top + 1) * settings->PercentScreen) / 100;
		}
	}

	if (!settings->Fullscreen && !settings->Workarea && !settings->UseMultimon)
		return TRUE;

	/* If single monitor fullscreen OR workarea without remote app */
	if ((settings->Fullscreen && !settings->UseMultimon && !settings->SpanMonitors) ||
			(settings->Workarea && !settings->RemoteApplicationMode))
	{
		/* If no monitors were specified on the command-line then set the current monitor as active */
		if (!settings->NumMonitorIds)
		{
			settings->MonitorIds[0] = current_monitor;
		}

		/* Always sets number of monitors from command-line to just 1.
		 * If the monitor is invalid then we will default back to current monitor
		 * later as a fallback. So, there is no need to validate command-line entry here.
		 */
		settings->NumMonitorIds = 1;
	}

	/* Create array of all active monitors by taking into account monitors requested on the command-line */
	for (i = 0; i < vscreen->nmonitors; i++)
	{
		if (!wf_is_monitor_id_active(wfc, i))
			continue;

		settings->MonitorDefArray[nmonitors].x = vscreen->monitors[i].area.left;
		settings->MonitorDefArray[nmonitors].y = vscreen->monitors[i].area.top;
        settings->MonitorDefArray[nmonitors].width = MIN(vscreen->monitors[i].area.right - vscreen->monitors[i].area.left + 1, (INT32) *pMaxWidth);
		settings->MonitorDefArray[nmonitors].height = MIN(vscreen->monitors[i].area.bottom - vscreen->monitors[i].area.top + 1, (INT32) *pMaxHeight);
		settings->MonitorDefArray[nmonitors].orig_screen = i;

		nmonitors++;
	}

	/* If no monitor is active(bogus command-line monitor specification) - then lets try to fallback to go fullscreen on the current monitor only */
	if (nmonitors == 0 && vscreen->nmonitors > 0)
	{
		settings->MonitorDefArray[0].x = vscreen->monitors[current_monitor].area.left;
		settings->MonitorDefArray[0].y = vscreen->monitors[current_monitor].area.top;
		settings->MonitorDefArray[0].width = MIN(vscreen->monitors[current_monitor].area.right - vscreen->monitors[current_monitor].area.left + 1, (INT32) *pMaxWidth);
		settings->MonitorDefArray[0].height = MIN(vscreen->monitors[current_monitor].area.bottom - vscreen->monitors[current_monitor].area.top + 1, (INT32) *pMaxHeight);
		settings->MonitorDefArray[0].orig_screen = current_monitor;

		nmonitors = 1;
	}

	settings->MonitorCount = nmonitors;

	/* If we have specific monitor information */
	if (settings->MonitorCount)
	{
		/* Initialize bounding rectangle for all monitors */
		int vX = settings->MonitorDefArray[0].x;
		int vY = settings->MonitorDefArray[0].y;
		int vR = vX + settings->MonitorDefArray[0].width;
		int vB = vY + settings->MonitorDefArray[0].height;
		wfc->fullscreenMonitors.top = wfc->fullscreenMonitors.bottom =
		wfc->fullscreenMonitors.left = wfc->fullscreenMonitors.right = settings->MonitorDefArray[0].orig_screen;

		/* Calculate bounding rectangle around all monitors to be used AND
		 * also set the Xinerama indices which define left/top/right/bottom monitors.
		 */
		for (i = 1; i < settings->MonitorCount; i++)
		{
			/* does the same as gdk_rectangle_union */
			int destX = MIN(vX, settings->MonitorDefArray[i].x);
			int destY = MIN(vY, settings->MonitorDefArray[i].y);
			int destR = MAX(vR, settings->MonitorDefArray[i].x + settings->MonitorDefArray[i].width);
			int destB = MAX(vB, settings->MonitorDefArray[i].y + settings->MonitorDefArray[i].height);

			if (vX != destX)
				wfc->fullscreenMonitors.left = settings->MonitorDefArray[i].orig_screen;
			if (vY != destY)
				wfc->fullscreenMonitors.top = settings->MonitorDefArray[i].orig_screen;
			if (vR != destR)
				wfc->fullscreenMonitors.right = settings->MonitorDefArray[i].orig_screen;
			if (vB != destB)
				wfc->fullscreenMonitors.bottom = settings->MonitorDefArray[i].orig_screen;

			vX = destX;
			vY = destY;
			vR = destR;
			vB = destB;
		}

		settings->DesktopPosX = vX;
		settings->DesktopPosY = vY;

		vscreen->area.left = 0;
		vscreen->area.right = vR - vX - 1;
		vscreen->area.top = 0;
		vscreen->area.bottom = vB - vY - 1;

		if (settings->Workarea)
		{
			vscreen->area.top = wfc->workArea.y;
			vscreen->area.bottom = wfc->workArea.height + wfc->workArea.y - 1;
		}

		/* If there are multiple monitors and we have not selected a primary */
		if (!primaryMonitorFound)
		{       
			/* First lets try to see if there is a monitor with a 0,0 coordinate */
			for (i=0; i<settings->MonitorCount; i++)
			{
				if (!primaryMonitorFound && settings->MonitorDefArray[i].x == 0 && settings->MonitorDefArray[i].y == 0)
				{
					settings->MonitorDefArray[i].is_primary = TRUE;
					settings->MonitorLocalShiftX = settings->MonitorDefArray[i].x;
					settings->MonitorLocalShiftY = settings->MonitorDefArray[i].y;
					primaryMonitorFound = TRUE;
				}
			}

			/* If we still do not have a primary monitor then just arbitrarily choose first monitor */
			if (!primaryMonitorFound)
			{
				settings->MonitorDefArray[0].is_primary = TRUE;
				settings->MonitorLocalShiftX = settings->MonitorDefArray[0].x;
				settings->MonitorLocalShiftY = settings->MonitorDefArray[0].y;
				primaryMonitorFound = TRUE;
			}
		}

		/* Subtract monitor shift from monitor variables for server-side use. 
		 * We maintain monitor shift value as Window requires the primary monitor to have a coordinate of 0,0
		 * In some X configurations, no monitor may have a coordinate of 0,0. This can also be happen if the user
		 * requests specific monitors from the command-line as well. So, we make sure to translate our primary monitor's
		 * upper-left corner to 0,0 on the server.
		 */
		for (i=0; i < settings->MonitorCount; i++)
		{
			settings->MonitorDefArray[i].x = settings->MonitorDefArray[i].x - settings->MonitorLocalShiftX;
			settings->MonitorDefArray[i].y = settings->MonitorDefArray[i].y - settings->MonitorLocalShiftY;
		}

		/* Set the desktop width and height according to the bounding rectangle around the active monitors */
		*pMaxWidth = vscreen->area.right - vscreen->area.left + 1;
		*pMaxHeight = vscreen->area.bottom - vscreen->area.top + 1;
	}

    return TRUE;
}
