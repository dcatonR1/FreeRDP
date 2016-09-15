# FreeRDP: A Remote Desktop Protocol Implementation
# FreeRDP cmake build script
#
# Copyright 2016 Route1 Inc.
#

define_channel_client("mobikey")

set(${MODULE_PREFIX}_SRCS
	mobikey_main.c
	mobikey_main.h)

include_directories(..)

add_channel_client_library(${MODULE_PREFIX} ${MODULE_NAME} ${CHANNEL_NAME} TRUE "DVCPluginEntry")

if (WITH_DEBUG_SYMBOLS AND MSVC AND NOT BUILTIN_CHANNELS AND BUILD_SHARED_LIBS)
	install(FILES ${CMAKE_BINARY_DIR}/${MODULE_NAME}.pdb DESTINATION ${FREERDP_ADDIN_PATH} COMPONENT symbols)
endif()

target_link_libraries(${MODULE_NAME} winpr)
set_property(TARGET ${MODULE_NAME} PROPERTY FOLDER "Channels/${CHANNEL_NAME}/Client")
