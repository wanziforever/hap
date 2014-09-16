TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
LINUX_PATH = $(TOP_SRC)/lib/linux
LINUX_SRC = $(LINUX_PATH)
LINUX_HEADER = $(TOP_INC)
LINUX_LIB = $(TOP_LIB)
LINUX_BIN = $(TOP_BIN)

TARGET := $(LINUX_LIB)/liblinux.so

SRC_PATH := $(LINUX_SRC)
HEADER_PATH := $(LINUX_HEADER)
LIB_PATH := $(LINUX_LIB)
BIN_PATH := $(LINUX_BIN)

SRC = \
	thread/mutex.c \
	thread/thread.c \
	thread/cond.c \
	thread/fcond.c \
	thread/usersem.c

src_dirs := $(sort $(dir $(addprefix $(SRC_PATH)/, $(SRC))))
VPATH := $(subst " ", ":", $(src_dirs))
LINUX_HDR = ${TOP_SRC}/hdr/linux
INC := ${INC} -I${LINUX_HDR}

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
	$(C) $(CFLAGS) $(INC) -fPIC -c $< -o $@

sinclude $(SRC:%.c=.%.d)

info:
p	@echo "SRC_PATH=$(SRC_PATH);" \
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
	$(LINK) -shared $^ -o $@ 

clean:
	rm -rf $(TARGET); rm -rf *.o; rm -rf .*.d

