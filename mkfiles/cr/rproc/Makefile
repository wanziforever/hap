TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
RPROC_PATH = $(TOP_SRC)/cr/rproc
RPROC_SRC = $(RPROC_PATH)
RPROC_HEADER = $(TOP_INC)
RPROC_BIN = $(TOP_BIN)
RPROC_LIB = $(TOP_LIB)

TARGET := $(RPROC_BIN)/rproc

SRC_PATH := $(RPROC_SRC)
HEADER_PATH := $(RPROC_HEADER)
LIB_PATH := $(RPROC_LIB)
BIN_PATH := $(RPROC_BIN)

SRC := \
	CRrprocinit.cc \
	CRrproc.cc \
	CRrpInitMsg.cc \
	CRrpInitExt.cc 

LIBS := $(LIBS) -lmsgh -lom -ltimer -lterm -lehhandler -lactivemq-cpp -lapr-1 -laprutil-1 -lssl

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
