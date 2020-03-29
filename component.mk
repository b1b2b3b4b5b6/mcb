#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_ADD_INCLUDEDIRS += debug
COMPONENT_SRCDIRS += debug

COMPONENT_ADD_INCLUDEDIRS += func
COMPONENT_SRCDIRS += func

COMPONENT_ADD_INCLUDEDIRS += func/ota
COMPONENT_SRCDIRS += func/ota

COMPONENT_ADD_INCLUDEDIRS += func/report
COMPONENT_SRCDIRS += func/report

COMPONENT_ADD_INCLUDEDIRS += func/device_config
COMPONENT_SRCDIRS += func/device_config

COMPONENT_ADD_INCLUDEDIRS += base_info
COMPONENT_SRCDIRS += base_info

COMPONENT_ADD_INCLUDEDIRS += base_protocol
COMPONENT_SRCDIRS += base_protocol

COMPONENT_ADD_INCLUDEDIRS += network
COMPONENT_SRCDIRS += network

COMPONENT_ADD_INCLUDEDIRS += cmd_protocol
COMPONENT_SRCDIRS += cmd_protocol

COMPONENT_ADD_INCLUDEDIRS += out_interface
COMPONENT_SRCDIRS += out_interface

COMPONENT_ADD_INCLUDEDIRS += out_port
COMPONENT_SRCDIRS += out_port


