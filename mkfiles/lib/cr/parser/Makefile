TOP = ../../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
PARSER_PATH = $(TOP_SRC)/lib/cr/parser
PARSER_SRC = $(PARSER_PATH)
PARSER_HEADER = $(TOP_INC)/cr
PARSER_LIB = $(TOP_LIB)
PARSER_BIN = $(TOP_BIN)

TARGET := $(PARSER_LIB)/libparser.so

SRC_PATH := $(PARSER_SRC)
HEADER_PATH := $(PARSER_HEADER)
LIB_PATH := $(PARSER_LIB)
BIN_PATH := $(PARSER_BIN)

SRC = \
	CRtokenIze.cc \
	CRparmv.cc \
	CRblockList.cc \
	CRmmlParm.cc \
	CRcmdEntry.cc \
	CRcmdTbl.cc \
	CRctBlockList.cc \
	CRctParm.cc \
	CRcepArgv.cc \
	CRinvokeCmd.cc \
#	CRintParser.cc 

LIBS := $(LIBS) -lpthread -ldl
CFLAGS := $(CFLAGS) -fPIC

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

