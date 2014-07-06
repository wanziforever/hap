TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
CSPOP_PATH = $(TOP_SRC)/cr/csop
CSPOP_SRC = $(CSPOP_PATH)
CSPOP_HEADER = $(TOP_INC)
CSPOP_BIN = $(TOP_BIN)
CSPOP_LIB = $(TOP_LIB)

TARGET := $(CSPOP_BIN)/csop

SRC_PATH := $(CSPOP_SRC)
HEADER_PATH := $(CSPOP_HEADER)
LIB_PATH := $(CSPOP_LIB)
BIN_PATH := $(CSPOP_BIN)

SRC := \
	crcsop.cc \
	CRcsopMsg.cc \
	CRcsopOMs.cc \
	CRcsopTrace.cc \
	CRdestEnt.cc \
	CRomBuffers.cc \
	CRomClEnt.cc \
	CRomdb.cc \
	CRomdbEnt.cc \
	CRomdbSim.cc \
	CRrcvOmdb.cc \
	CRdoprint.cc \

LIBS := $(LIBS) $(LIB_PATH)/INmain.a -lpq -lom -ldbi -lmisc -lmsgh -lehhandler -ltimer -lutils -lactivemq-cpp -lapr-1 -laprutil-1 -lssl

src_dirs := $(sort $(dir $(addprefix $(SRC_PATH)/, $(SRC))))
VPATH := $(subst " ", ":", $(src_dirs))


###########################
OBJS := $(addsuffix .o, $(basename $(notdir $(SRC))))
.PHONY : clean info Prepare

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
	$(LINK) $^ -o $@ $(LIBS)


clean:
	rm -rf $(TARGET); rm -rf *.o; rm -rf .*.d
