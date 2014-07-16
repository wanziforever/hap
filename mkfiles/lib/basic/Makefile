TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
BASIC_PATH = $(TOP_SRC)/lib/basic
BASIC_SRC = $(BASIC_PATH)
BASIC_HEADER = $(TOP_INC)
BASIC_LIB = $(TOP_LIB)
BASIC_BIN = $(TOP_BIN)

TARGET := $(BASIC_LIB)/libbasic.so

SRC_PATH := $(BASIC_SRC)
HEADER_PATH := $(BASIC_HEADER)
LIB_PATH := $(BASIC_LIB)
BIN_PATH := $(BASIC_BIN)

SRC = \
	atoi.c

src_dirs := $(sort $(dir $(addprefix $(SRC_PATH)/, $(SRC))))
VPATH := $(subst " ", ":", $(src_dirs))

OBJS := $(addsuffix .o, $(basename $(notdir $(SRC))))
.PHONY : clean info Prepare

all: info Prepare $(TARGET)

$(OBJS):%.o:%.c
	@$(C) -MM $(CFLAGS) $(INC) -o .$*.d $<
	@mv -f .$*.d .$*.d.tmp
	@sed -e 's|.*:|$@:|' < .$*.d.tmp > .$*.d
#	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
#	  sed -e 's/^ *//' -e 's/$$/:/' >> .$*.d
	@rm -f .$*.d.tmp
	$(C) $(CFLAGS) $(INC) -c $< -o $@

sinclude $(SRC:%.c=.%.d)

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
	$(LINK) -shared  $^ -o $@

clean:
	rm -rf $(TARGET); rm -rf *.o; rm -rf .*.d

