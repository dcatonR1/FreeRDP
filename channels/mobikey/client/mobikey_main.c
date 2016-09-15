/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * MobiKey Virtual Channel Extension
 *
 * Copyright 2016 Route1 Inc.
 *
 * The current purpose of this channel is simply to allow the Mobi host service to identify an RDP
 * session that is connected to a MobiKEY remote, as opposed to a session created by some other RDP client
 * (or a session with no connected user).
 *
 * We need to identify an MobiKEY-created RDP session in the host service because we need to know the name of the
 * session's logged-in user, so we can create scan files in the correct user directory.  WTSEnumerateSessions()
 * doesn't help because it can't differentiate a session created by MobiKEY vs. one created by something else.
 *
 * The solution is for the Mobi host service to try and open this virtual channel when it receives a WTS_REMOTE_CONNECT
 * notification from the service control manager. If the channel open is successful, then we know this it an RDP session
 * created by the Mobi version of FreeRDP, and the host can then obtain the username for that session.
 *
 * If at some point in the future we need to send data across the RDP connection rather than through our MSA channel,
 * this virtual channel can be enhanced to actually do something more substantial.  For now, it's sufficient that it can
 * be opened.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <winpr/crt.h>
#include <winpr/stream.h>

#include "mobikey_main.h"
#include <freerdp/channels/log.h>

#define TAG CHANNELS_TAG("mobikey.client")

IWTSVirtualChannel* _spChannel;

typedef struct _MOBIKEY_LISTENER_CALLBACK MOBIKEY_LISTENER_CALLBACK;

struct _MOBIKEY_LISTENER_CALLBACK
{
    IWTSListenerCallback iface;

    IWTSPlugin* plugin;
    IWTSVirtualChannelManager* channel_mgr;
};

typedef struct _MOBIKEY_CHANNEL_CALLBACK MOBIKEY_CHANNEL_CALLBACK;

struct _MOBIKEY_CHANNEL_CALLBACK
{
    IWTSVirtualChannelCallback iface;

    IWTSPlugin* plugin;
    IWTSVirtualChannelManager* channel_mgr;
    IWTSVirtualChannel* channel;
};

typedef struct _mobikey_PLUGIN mobikey_PLUGIN;

struct _mobikey_PLUGIN
{
    IWTSPlugin iface;

    MOBIKEY_LISTENER_CALLBACK* listener_callback;
};

/**
* Function description
*
* @return 0 on success, otherwise a Win32 error code
*/
static UINT mobikey_on_data_received(IWTSVirtualChannelCallback* pChannelCallback, wStream *data)
{
    // Note: presently this never gets called as we are not sending/receiving any data over this channel.
    // This implementation simply echos back whatever it received, which can be used for testing purposes.

    MOBIKEY_CHANNEL_CALLBACK* callback = (MOBIKEY_CHANNEL_CALLBACK*) pChannelCallback;
    BYTE* pBuffer = Stream_Pointer(data);
    UINT32 cbSize = Stream_GetRemainingLength(data);

    /* echo back what we have received */
    return callback->channel->Write(callback->channel, cbSize, pBuffer, NULL);
}

/**
* Function description
*
* @return 0 on success, otherwise a Win32 error code
*/
static UINT mobikey_on_close(IWTSVirtualChannelCallback* pChannelCallback)
{
    MOBIKEY_CHANNEL_CALLBACK* callback = (MOBIKEY_CHANNEL_CALLBACK*) pChannelCallback;

    free(callback);

    return CHANNEL_RC_OK;
}

/**
* Function description
*
* @return 0 on success, otherwise a Win32 error code
*/
static UINT mobikey_on_new_channel_connection(IWTSListenerCallback* pListenerCallback,
    IWTSVirtualChannel* pChannel, BYTE* Data, BOOL* pbAccept,
    IWTSVirtualChannelCallback** ppCallback)
{
    // apparently Data is always NULL
    // pbAccept defaults to TRUE but set it anyway to be safe

    UINT ret = CHANNEL_RC_OK;

    MOBIKEY_CHANNEL_CALLBACK* callback;
    MOBIKEY_LISTENER_CALLBACK* listener_callback = (MOBIKEY_LISTENER_CALLBACK*) pListenerCallback;

    callback = (MOBIKEY_CHANNEL_CALLBACK*) calloc(1, sizeof(MOBIKEY_CHANNEL_CALLBACK));

    if (!callback)
    {
        WLog_ERR(TAG, "calloc failed!");
        ret = CHANNEL_RC_NO_MEMORY;
    }
    else
    {

        callback->iface.OnDataReceived = mobikey_on_data_received;
        callback->iface.OnClose = mobikey_on_close;
        callback->plugin = listener_callback->plugin;
        callback->channel_mgr = listener_callback->channel_mgr;
        callback->channel = pChannel;

        *ppCallback = (IWTSVirtualChannelCallback*) callback;

        _spChannel = pChannel;
        *pbAccept = TRUE;
    }

    return ret;
}

/**
* Function description
*
* @return 0 on success, otherwise a Win32 error code
*/
static UINT mobikey_plugin_initialize(IWTSPlugin* pPlugin, IWTSVirtualChannelManager* pChannelMgr)
{
    UINT ret;

    mobikey_PLUGIN* mobikey = (mobikey_PLUGIN*) pPlugin;

    mobikey->listener_callback = (MOBIKEY_LISTENER_CALLBACK*) calloc(1, sizeof(MOBIKEY_LISTENER_CALLBACK));

    if (!mobikey->listener_callback)
    {
        WLog_ERR(TAG, "calloc failed!");
        return CHANNEL_RC_NO_MEMORY;
    }

    mobikey->listener_callback->iface.OnNewChannelConnection = mobikey_on_new_channel_connection;
    mobikey->listener_callback->plugin = pPlugin;
    mobikey->listener_callback->channel_mgr = pChannelMgr;

    ret = pChannelMgr->CreateListener(pChannelMgr, MOBI_DVC_CHANNEL_NAME, 0, (IWTSListenerCallback*) mobikey->listener_callback, NULL);

    return ret;
}

/**
* Function description
*
* @return 0 on success, otherwise a Win32 error code
*/
static UINT mobikey_plugin_terminated(IWTSPlugin* pPlugin)
{
    mobikey_PLUGIN* mobikey = (mobikey_PLUGIN*) pPlugin;

    free(mobikey);

    return CHANNEL_RC_OK;
}

#ifdef BUILTIN_CHANNELS
#define DVCPluginEntry		mobikey_DVCPluginEntry
#else
#define DVCPluginEntry		FREERDP_API DVCPluginEntry
#endif

/**
* Function description
*
* @return 0 on success, otherwise a Win32 error code
*/
UINT DVCPluginEntry(IDRDYNVC_ENTRY_POINTS* pEntryPoints)
{
    UINT status = CHANNEL_RC_OK;
    mobikey_PLUGIN* mobikey;

    mobikey = (mobikey_PLUGIN*) pEntryPoints->GetPlugin(pEntryPoints, "mobikey");

    if (!mobikey)
    {
        mobikey = (mobikey_PLUGIN*) calloc(1, sizeof(mobikey_PLUGIN));

        if (!mobikey)
        {
            WLog_ERR(TAG, "calloc failed!");
            return CHANNEL_RC_NO_MEMORY;
        }

        mobikey->iface.Initialize = mobikey_plugin_initialize;
        mobikey->iface.Connected = NULL;
        mobikey->iface.Disconnected = NULL;
        mobikey->iface.Terminated = mobikey_plugin_terminated;

        status = pEntryPoints->RegisterPlugin(pEntryPoints, "mobikey", (IWTSPlugin*) mobikey);
    }

    return status;
}
