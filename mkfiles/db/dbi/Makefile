TOP = ../..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
DBI_PATH = $(TOP_SRC)/proc/db/dbi
DBI_SRC = $(DBI_PATH)
DBI_HEADER = $(TOP_INC)
DBI_BIN = $(TOP_BIN)
DBI_LIB = $(TOP_LIB)

TARGET := $(DBI_BIN)/dbi

SRC_PATH := $(DBI_SRC)
HEADER_PATH := $(DBI_HEADER)
LIB_PATH := $(DBI_LIB)
BIN_PATH := $(DBI_BIN)

SRC := \
	DBI.cc \
	DBIhpr.cc \
	DBIprocessMsg.cc \
	DBIregister.cc \
	DBIrtnMsg.cc \
	DBIutil.cc \
	DBIqryStat.cc \
	DBIsyncMsg.cc

#LIBS := $(LIBS) $(LIB_PATH)/INmain.a -lpq -lom -ldbi -lmisc -lmsgh -lehhandler -ltimer -lutils

src_dirs := $(sort $(dir $(addprefix $(SRC_PATH)/, $(SRC))))
VPATH := $(subst " ", ":", $(src_dirs))


###########################
OBJS := $(addsuffix .o, $(basename $(notdir $(SRC))))
.PHONY : clean info Prepare
LINUX_HDR = ${TOP_SRC}/hdr/linux
INC := ${INC} -I${LINUX_HDR}

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
	@mkdir -p ${DBI_BIN}
	$(LINK) $^ -o $@ $(LIBS) ${TOP_LIB}/INmain.a -lmsgh -ltimer -llinux -lehhandler -ldb


clean:
	rm -rf $(TARGET); rm -rf *.o; rm -rf .*.d
