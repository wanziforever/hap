TOP_ROOT = ..
TOP_SRC = $(ROOT)/cc
TOP_INC = $(ROOT)/hdr
TOP_BIN = $(ROOT)/bin
TOP_LIB = $(ROOT)/lib

CC = g++
LINK = g++
AR = ar

# third party headers
3rd_INC_BASE = $(ROOT)/include/3rdparty
#third party libraries
3rd_LIB_BASE = $(ROOT)/lib/3rdparty

# some of the variable has default value
CFLAGS = -Wall -g -Wno-unused
INC = -I$(TOP_INC) -I$(ROOT)
LIBS = -L. -L$(TOP_LIB) -L$(3rd_LIB_BASE)

