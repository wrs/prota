# Thanks https://spin.atomicobject.com/2016/08/26/makefile-c-projects/

TARGET_EXEC ?= nstest

BUILD_DIR ?= ./build
SRC_DIRS ?= ./runtime ./test

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := ./include ./gc/include
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CC := cc
CXX := cc

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -g -m32 -stdlib=libc++ -std=c++11 -Wno-c++11-compat-deprecated-writable-strings
LDFLAGS ?= -m32 -stdlib=libc++ -lc++ bdwgc/gc.a

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
