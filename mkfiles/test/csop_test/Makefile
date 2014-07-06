TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
CSOP_TEST_PATH = $(TOP_SRC)/test/csop_test
CSOP_TEST_SRC = $(CSOP_TEST_PATH)
CSOP_TEST_HEADER = $(TOP_INC)
CSOP_TEST_BIN = $(TOP_BIN)
CSOP_TEST_LIB = $(TOP_LIB)

TARGET := $(CSOP_TEST_BIN)/csop_test

SRC_PATH := $(CSOP_TEST_SRC)
HEADER_PATH := $(CSOP_TEST_HEADER)
LIB_PATH := $(CSOP_TEST_LIB)
BIN_PATH := $(CSOP_TEST_BIN)

SRC := \
	csop_test.cc

INC := $(INC) -I$(3rd_INC_BASE)
LIBS := $(LIBS) $(LIB_PATH)/INmain.a -lpq -lom -lmisc -lmsgh -ldbi -lehhandler -ltimer -lutils -lactivemq-cpp -lapr-1 -laprutil-1 -lssl

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
