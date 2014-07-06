TOP = ..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
CEP_PATH = $(TOP_SRC)/
CEP_SRC = $(CEP_PATH)/cep
CEP_HEADER = $(TOP_INC)/init
CEP_BIN = $(TOP_BIN)/cep
CEP_LIB = $(TOP_LIB)

SRC_PATH := $(CEP_SRC)
HEADER_PATH := $(CEP_HEADER)
LIB_PATH := $(CEP_LIB)
BIN_PATH := $(CEP_BIN)

SRC := \
	CRtrace.cc

TARGETS := \
	TRACE_ADM

LIBS := $(LIBS) -lmsgh -lom -ltimer -lcep -lparser -lmisc -lutils -lactivemq-cpp -laprutil-1 -lehhandler -lactivemq-cpp -lapr-1 -lssl

src_dirs := $(sort $(dir $(addprefix $(SRC_PATH)/, $(SRC))))
VPATH := $(subst " ", ":", $(src_dirs))

###########################
OBJS := $(addsuffix .o, $(basename $(notdir $(SRC))))
#TARGETS := $(addprefix $(BIN_PATH)/, $(basename $(notdir $(SRC))))
#TARGETS := $(basename $(notdir $(SRC)))
.PHONY : clean info Prepare

all: info Prepare $(TARGETS)

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
	@echo "TARGET: $(TARGETS)"
	@echo "VPATH: $(VPATH)"
	@echo "SRC: $(SRC)"
	@echo "$\OBJ: $(OBJS)"

Prepare:
	@$(PREPARE)


TRACE_ADM: CRtrace.o
	$(LINK) $^ -o $(BIN_PATH)/$@ $(LIBS)

##########################


clean:
	rm -rf $(addprefix $(BIN_PATH)/, $(TARGETS))
	rm -rf *.o
	rm -rf .*.d
