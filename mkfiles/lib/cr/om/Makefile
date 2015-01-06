TOP = ../../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
OM_PATH = $(TOP_SRC)/lib/cr/om
OM_SRC = $(OM_PATH)
OM_HEADER = $(TOP_INC)/cr
OM_LIB = $(TOP_LIB)
OM_BIN = $(TOP_BIN)

TARGET := $(OM_LIB)/libom.so

SRC_PATH := $(OM_SRC)
HEADER_PATH := $(OM_HEADER)
LIB_PATH := $(OM_LIB)
BIN_PATH := $(OM_BIN)

SRC = \
	CRdebugMsg.cc \
	CRmsg.cc \
	CRomInfo.cc \
	CRspoolMsg.cc \
	CRmsgClass.cc \
	CRalarmLevel.cc \
	CRomHdrFtr.cc \
	CRofficeNm.cc \
	CRlogFile.cc \
	CRloadName.cc \
	CRmofMsg.cc \
	CRlogLoc.cc \
	CRlogFileSwitchUpd.cc \
	CRprmMsg.cc \
	CRprmSpool.cc \
	CRalarmMsg.cc \
	CRdbCmdMsg.cc


LIBS := $(LIBS) -lpthread -ldl
CFLAGS := $(CFLAGS) -fPIC

src_dirs := $(sort $(dir $(addprefix $(SRC_PATH)/, $(SRC))))
VPATH := $(subst " ", ":", $(src_dirs))

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

$(TARGET): $(OBJS)
	$(LINK) -shared  $^ -o $@ $(LIBS)

clean:
	rm -rf $(TARGET); rm -rf *.o; rm -rf .*.d

