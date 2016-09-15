/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * MobiKey Virtual Channel Extension
 *
 * Copyright 2016 Route1 Inc.
 */

#ifndef __MOBIKEY_MAIN_H
#define __MOBIKEY_MAIN_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <freerdp/dvc.h>
#include <freerdp/types.h>
#include <freerdp/addin.h>
#include <freerdp/channels/log.h>

#define DVC_TAG CHANNELS_TAG("mobikey.client")
#ifdef WITH_DEBUG_DVC
#define DEBUG_DVC(fmt, ...) WLog_DBG(DVC_TAG, fmt, ## __VA_ARGS__)
#else
#define DEBUG_DVC(fmt, ...) do { } while (0)
#endif

// Caution! Don't change this unless it is also changed
// in the host service code.  So don't change it.
#define MOBI_DVC_CHANNEL_NAME "MobiRDPSession"

#endif /* __MOBIKEY_MAIN_H */

