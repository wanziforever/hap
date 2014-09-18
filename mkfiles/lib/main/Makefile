TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
INMAIN_PATH = $(TOP_SRC)/lib/init
INMAIN_SRC = $(INMAIN_PATH)
INMAIN_HEADER = $(TOP_INC)
INMAIN_BIN = $(TOP_BIN)
INMAIN_LIB = $(TOP_LIB)

TARGET := ${INMAIN_LIB}/INmain.a

SRC_PATH := $(INMAIN_SRC)
HEADER_PATH := $(INMAIN_HEADER)
LIB_PATH := $(INMAIN_LIB)
BIN_PATH := $(INMAIN_BIN)

SRC := \
	INreq.cc \
	INapi.cc \
	INargchk.cc \
	INcommon.cc \
	INdata.cc \
	INdist.cc \
	INgetindx.cc \
	INsbt.cc \
	INsanipeg.cc \
	INsharedMem.cc \
	INmain.cc \
	INsync.cc

LIBS := $(LIBS) -lmsgh -lbasic -llinux

src_dirs := $(sort $(dir $(addprefix $(SRC_PATH)/, $(SRC))))
VPATH := $(subst " ", ":", $(src_dirs))

###########################
OBJS := $(addsuffix .o, $(basename $(notdir $(SRC))))
.PHONY : clean info Prepare
LINUX_HDR = ${TOP_SRC}/hdr/linux
INC := ${INC} -I${LINUX_HDR}

all: info Prepare $(TARGET)

$(OBJS):%.o:%.cc
	@$(CC) -MM $(CFLAGS) $(INC) -o .$*.d $<
	@mv -f .$*.d .$*.d.tmp
	@sed -e 's|.*:|$@:|' < .$*.d.tmp > .$*.d
#	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
#	  sed -e 's/^ *//' -e 's/$$/:/' >> .$*.d
	@rm -f .$*.d.tmp
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

sinclude $(SRC:%.cc=.%.d)

info:
#	@echo "SRC_PATH=$(SRC_PATH);" \
#	"HEADER_PATH=$(HEADER_PATH);" \
#	"LIB_PATH=$(LIB_PATH);" \
#	"BIN_PATH=$(BIN_PATH);" \
#	"TARGET=$(TARGET);" \
#	"VPATH=$(VPATH)"
#	@echo "SRC=$(SRC)" 
#	@echo "$\OBJ=$(OBJS)"

Prepare:
	@$(PREPARE)

##########################

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

clean:
	rm -rf $(TARGET); rm -rf *.o; rm -rf .*.d
