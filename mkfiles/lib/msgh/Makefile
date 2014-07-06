TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
MSGH_PATH = $(TOP_SRC)/lib/msgh
MSGH_SRC = $(MSGH_PATH)
MSGH_HEADER = $(TOP_INC)/msgh
MSGH_LIB = $(TOP_LIB)
MSGH_BIN = $(TOP_BIN)

TARGET := $(MSGH_LIB)/libmsgh.so

SRC_PATH := $(MSGH_SRC)
HEADER_PATH := $(MSGH_HEADER)
LIB_PATH := $(MSGH_LIB)
BIN_PATH := $(MSGH_BIN)

SRC = \
	MHcontext.cc \
	MHqsender.cc \
	MHqreceiver.cc \
	MHgenname.cc \
	MHmsgBase.cc

LIBS := $(LIBS) -lpthread -ldl
CFLAGS := $(CFLAGS) -fPIC
INC := $(INC) -I$(3rd_INC_BASE)/activemq-cpp-3.6.0 -I$(3rd_INC_BASE)/apr/apr-1 -I$(3rd_INC_BASE)/apr-util/apr-1

src_dirs := $(sort $(dir $(addprefix $(SRC_PATH)/, $(SRC))))
VPATH := $(subst " ", ":", $(src_dirs))

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
	@echo "SRC_PATH=$(SRC_PATH);" \
	"HEADER_PATH=$(HEADER_PATH);" \
	"LIB_PATH=$(LIB_PATH);" \
	"BIN_PATH=$(BIN_PATH);" \
	"TARGET=$(TARGET);" \
	"VPATH=$(VPATH)"
	@echo "SRC=$(SRC)" 
	@echo "$\OBJ=$(OBJS)"

Prepare:
	@$(PREPARE)

$(TARGET): $(OBJS)
	$(LINK) -shared  $^ -o $@ $(LIBS)

clean:
	rm -rf $(TARGET); rm -rf *.o; rm -rf .*.d

