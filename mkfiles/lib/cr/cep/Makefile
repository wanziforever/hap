TOP = ../../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
CEP_PATH = $(TOP_SRC)/lib/cr/cep
CEP_SRC = $(CEP_PATH)
CEP_HEADER = $(TOP_INC)/cr
CEP_LIB = $(TOP_LIB)
CEP_BIN = $(TOP_BIN)

TARGET := $(CEP_LIB)/libcep.so

SRC_PATH := $(CEP_SRC)
HEADER_PATH := $(CEP_HEADER)
LIB_PATH := $(CEP_LIB)
BIN_PATH := $(CEP_BIN)

SRC = \
	CRmmlUtil.cc \
	CRcmdLine.cc



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

