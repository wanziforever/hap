TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
PROCA_PATH = $(TOP_SRC)/test/procA
PROCA_SRC = $(PROCA_PATH)
PROCA_HEADER = $(TOP_INC)
PROCA_BIN = $(TOP_BIN)
PROCA_LIB = $(TOP_LIB)

TARGET := $(PROCA_BIN)/proca

SRC_PATH := $(PROCA_SRC)
HEADER_PATH := $(PROCA_HEADER)
LIB_PATH := $(PROCA_LIB)
BIN_PATH := $(PROCA_BIN)

SRC := \
	proca.cc


LIBS := $(LIBS) $(LIB_PATH)/INmain.a -lom -lmisc -lmsgh -lehhandler -ltimer -lutils -lactivemq-cpp -lapr-1 -laprutil-1 -lssl

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
