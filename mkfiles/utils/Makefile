TOP = ..
include $(TOP)/top.mk
ROOT = $(TOP)/$(TOP_ROOT)
UTILS_PATH = $(TOP_SRC)/utils
UTILS_SRC = $(UTILS_PATH)
UTILS_HEADER = $(TOP_INC)
UTILS_BIN = $(TOP_BIN)
UTILS_LIB = $(TOP_LIB)
UTILS_OBJ = .

TARGET = $(UTILS_LIB)/libutils.so

SRC_PATH := $(UTILS_SRC)
HEADER_PATH := $(UTILS_HEADER)
OBJ_PATH := $(UTILS_OBJ)
LIB_PATH := $(UTILS_BIN)
BIN_PATH := $(UTILS_BIN)

SRC = \
	stringutils.cc


INC := -I$(TOP_INC)
LIBS := -L. -L$(TOP_LIB)
CFLAGS := $(CFLAGS) -fPIC

#############################################
src_dirs := $(addprefix $(SRC_PATH)/, $(sort $(dir $(SRC))))
VPATH := $(subst " ", ":", $(src_dirs))

OBJS := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(basename $(notdir $(SRC)))))

all: info Prepare $(TARGET)

info:
	@echo "SRC_PATH: $(SRC_PATH)"
	@echo "HEADER_PATH: $(HEADER_PATH)"
	@echo "OBJ_PATH: $(OBJ_PATH)"
	@echo "LIB_PATH: $(LIB_PATH)"
	@echo "BIN_PATH: $(BIN_PATH)"
	@echo "TARGET: $(TARGET)"
	@echo "VPATH: $(VPATH)"

Prepare:
	@mkdir -p $(OBJ_PATH)


$(OBJ_PATH)/%.o:%.cc
	@$(CC) -MM $(CFLAGS) $(INC) -o .$*.d $<
	@mv -f .$*.d .$*.d.tmp
	@sed -e 's|.*:|$@:|' < .$*.d.tmp > .$*.d
#	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
#	  sed -e 's/^ *//' -e 's/$$/:/' >> .$*.d
	@rm -f .$*.d.tmp
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

sinclude $(SRC:%.cc=.%.d)

$(TARGET): $(OBJS)
	$(LINK) -shared $^ -o $@ $(LIBS)

clean:
	rm -rf $(TARGET)
	rm -rf *.o
	rm -rf .*.d
