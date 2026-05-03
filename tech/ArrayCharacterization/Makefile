# output executable name
TARGET := msxac

# directory of source files (.cpp) for compilation
SRCDIR := src
# directory of intermediate object files (.o) for linking and dependency files (.d) for compilation
OBJDIR := build
# list of directories to be added to compiler header search path
incdirs := include

# C++ tool to use
CXX := g++
# C++ version to compile for
CXXVERSION := c++17
# list of include path flags to feed to compiler
INC := $(addprefix -I, $(incdirs))
# list of warning flags to feed to compiler
CXXWARN := -Wall -Wpedantic -Wextra -Werror
# final list of flags to feed to compiler, -MMD and -MP generate dependencies
CXXFLAGS := -std=$(CXXVERSION) $(CXXWARN) $(INC) -MMD -MP
# list of flags to feed to linker
LDFLAGS :=

# lists of files required by the target to compile (SRCS and DEPS) and link (OBJS)
SRCS = $(shell find $(SRCDIR) -name "*.cpp")
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: all debug clean

all: $(TARGET)

debug: CXXFLAGS += -g -O0
debug: LDFLAGS += -g
debug: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET)
	rm -rf $(OBJDIR)

-include $(DEPS)
