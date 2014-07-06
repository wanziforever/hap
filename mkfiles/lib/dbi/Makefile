TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
DBI_PATH = $(TOP_SRC)/lib/dbi
DBI_SRC = $(DBI_PATH)
DBI_HEADER = $(TOP_INC)/dbi
DBI_LIB = $(TOP_LIB)
DBI_BIN = $(TOP_BIN)

TARGET := $(DBI_LIB)/libdbi.so

SRC_PATH := $(DBI_SRC)
HEADER_PATH := $(DBI_HEADER)
LIB_PATH := $(DBI_LIB)
BIN_PATH := $(DBI_BIN)

SRC = \
	postgresql_operater.cc


LIBS := $(LIBS) -lpthread -ldl
CFLAGS := $(CFLAGS) -fPIC
INC := $(INC) -I$(3rd_INC_BASE)

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

