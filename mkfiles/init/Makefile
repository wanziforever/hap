TOP = ..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
INIT_PATH = $(TOP_SRC)/init
INIT_SRC = $(INIT_PATH)/proc
INIT_HEADER = $(TOP_INC)/init
INIT_BIN = $(TOP_BIN)
INIT_LIB = $(TOP_LIB)

TARGET := $(INIT_BIN)/init

SRC_PATH := $(INIT_SRC)
HEADER_PATH := $(INIT_HEADER)
LIB_PATH := $(INIT_LIB)
BIN_PATH := $(INIT_BIN)

SRC := \
	init.cc \
	inlocal.cc \
	initlist.cc \
	insync.cc \
	increate.cc \
	intimerprc.cc \
	inrcvmsg.cc  \
	inrestart.cc


LIBS := $(LIBS) -lmsgh -lom -ltimer -lutils -lactivemq-cpp  -laprutil-1 -lehhandler -lactivemq-cpp -lapr-1 -lssl

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
	@echo "SRC_PATH: $(SRC_PATH)"
	@echo "HEADER_PATH: $(HEADER_PATH)"
	@echo "LIB_PATH: $(LIB_PATH)"
	@echo "BIN_PATH: $(BIN_PATH)"
	@echo "TARGET: $(TARGET)"
	@echo "VPATH: $(VPATH)"
	@echo "SRC: $(SRC)"
	@echo "$\OBJ: $(OBJS)"

Prepare:
	@$(PREPARE)

##########################

$(TARGET): $(OBJS)
	$(LINK) $^ -o $@ $(LIBS)


clean:
	rm -rf $(TARGET)
	rm -rf *.o
	rm -rf .*.d
