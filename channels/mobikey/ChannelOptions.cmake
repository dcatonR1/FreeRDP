
set(OPTION_DEFAULT OFF)
set(OPTION_CLIENT_DEFAULT ON)

define_channel_options(NAME "mobikey" TYPE "dynamic"
	DESCRIPTION "MobiKey Virtual Channel Extension"
	SPECIFICATIONS "[None]"
	DEFAULT ${OPTION_DEFAULT})

define_channel_client_options(${OPTION_CLIENT_DEFAULT})


