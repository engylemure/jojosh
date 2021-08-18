COMPILER = clang
#  Directory for binary file
TARGET_PATH = target
# Build files directory
BUILD_PATH = build
# Source files directories
SRC_PATH = src
COMPILER_CMD = $(COMPILER) $(DBG_FLAG)

SOURCES := $(shell find $(SRC_PATH) -name '*.c')
SOURCES_PATH := $(sort $(dir $(SOURCES)))
OBJECTS := $(addprefix $(BUILD_PATH)/,$(SOURCES:%.c=%.o))

NAME = jojosh
BINARY = $(NAME)
BINARY_PATH = $(TARGET_PATH)/$(BINARY)
ECHO = echo
RM = rm -rf
MKDIR = mkdir

ifeq ($(DEBUG), 1) 
	DBG_FLAG = -g
endif

initial_setup:
	echo $(OBJECTS)
	$(MKDIR) -p $(BUILD_PATH) $(addprefix $(BUILD_PATH)/,$(SOURCES_PATH:./=)) $(TARGET_PATH)

all: initial_setup $(BINARY)

$(BINARY): $(OBJECTS)
	$(COMPILER_CMD) $(OBJECTS) -o $(BINARY_PATH)

$(BUILD_PATH)/%.o: %.c
	$(COMPILER_CMD) -c $<  -o $@

build_cleanup:
	$(RM) -f $(BUILD_PATH)

clean:
	$(RM) -f $(TARGET_PATH) $(BUILD_PATH)

help:
	@$(ECHO) "Targets:"
	@$(ECHO) "all - compile and build whatever is necessary"
	@$(ECHO) "build_cleanup - remove build files"
	@$(ECHO) "clean - cleanup build and binary"