TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
TEXTLMT_PATH = $(TOP_SRC)/cr/termproc
TEXTLMT_SRC = $(TEXTLMT_PATH)
TEXTLMT_HEADER = $(TOP_INC)
TEXTLMT_BIN = $(TOP_BIN)
TEXTLMT_LIB = $(TOP_LIB)

TARGET := $(TEXTLMT_BIN)/lmt

SRC_PATH := $(TEXTLMT_SRC)
HEADER_PATH := $(TEXTLMT_HEADER)
LIB_PATH := $(TEXTLMT_LIB)
BIN_PATH := $(TEXTLMT_BIN)

SRC := \
	CRtextlmt.cc \
	CRtextlmtMsg.cc \
	CRsop.cc \
	CRintParser.cc
	

LIBS := $(LIBS) $(LIB_PATH)/INmain.a -lmsgh -lom -lcep -lmisc -lutils -lterm -lparser -ltimer -lehhandler -lactivemq-cpp -lapr-1 -laprutil-1 -lssl

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
