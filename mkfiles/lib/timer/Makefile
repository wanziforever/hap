TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
TIMER_PATH = $(TOP_SRC)/lib/timer
TIMER_SRC = $(TIMER_PATH)
TIMER_HEADER = $(TOP_INC)/timer
TIMER_LIB = $(TOP_LIB)
TIMER_BIN = $(TOP_BIN)

TARGET := $(TIMER_LIB)/libtimer.so

SRC_PATH := $(TIMER_SRC)
HEADER_PATH := $(TIMER_HEADER)
LIB_PATH := $(TIMER_LIB)
BIN_PATH := $(TIMER_BIN)

SRC = \
	TMprivate.cc \
	TMtimers.cc \
	TMtimes.cc

LIBS := $(LIBS) -lpthread -ldl
CFLAGS := $(CFLAGS) -fPIC
INC := $(INC) 

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

