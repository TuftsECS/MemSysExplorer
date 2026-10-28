# operating system being built on, Windows, Linux or Darwin (macOS)
ifeq ($(OS),Windows_NT)
    PLATFORM := Windows
else
    PLATFORM := $(shell uname -s)
endif

TARGETNAME := msxac
# output executable name, Windows executables end in .exe
TARGET := $(TARGETNAME)$(if $(filter Windows,$(PLATFORM)),.exe)

# directory of source files (.cpp) for compilation
SRCDIR := src
# directory of intermediate object files (.o) for linking and dependency files (.d) for compilation
OBJDIR := build
# list of directories to be added to compiler header search path
incdirs := include

# C++ tool to use comes from make's CXX, g++ unless set in the environment or on the command line (make CXX=clang++)
# C++ version to compile for
CXXVERSION := c++23
# list of include path flags to feed to compiler
INC := $(addprefix -I, $(incdirs))
# list of warning flags to feed to compiler
CXXWARN := -Wall -Wpedantic -Wextra -Werror -Wno-unknown-pragmas
# final list of flags to feed to compiler, -MMD and -MP generate dependencies
CXXFLAGS := -std=$(CXXVERSION) $(CXXWARN) $(INC) -MMD -MP -O1
# list of flags to feed to linker
LDFLAGS :=

# OpenMP flags, Apple clang needs libomp from Homebrew (brew install libomp) and takes OpenMP through the preprocessor
OMPFLAGS := -fopenmp
OMPLIBS := -fopenmp
ifeq ($(PLATFORM),Darwin)
    ifneq ($(findstring clang,$(shell $(CXX) --version)),)
        LIBOMP := $(shell brew --prefix libomp 2>/dev/null)
        OMPFLAGS := -Xpreprocessor -fopenmp -I$(LIBOMP)/include
        OMPLIBS := -L$(LIBOMP)/lib -lomp
    endif
endif

# commands to make and remove directories, Windows without a Unix shell (sh) uses cmd commands instead
ifeq ($(shell echo "cmd"),"cmd")
    MKDIR = if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
    RMFILE = if exist $(subst /,\,$(1)) del /q $(subst /,\,$(1))
    RMDIR = if exist $(subst /,\,$(1)) rmdir /s /q $(subst /,\,$(1))
else
    MKDIR = mkdir -p $(1)
    RMFILE = rm -f $(1)
    RMDIR = rm -rf $(1)
endif

# recursively finds files in directory $(1) matching pattern $(2) without needing the find command
rwildcard = $(foreach entry,$(wildcard $(1)/*),$(call rwildcard,$(entry),$(2)) $(filter $(subst *,%,$(2)),$(entry)))

# lists of files required by the target to compile (SRCS and DEPS) and link (OBJS)
SRCS = $(call rwildcard,$(SRCDIR),*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: all debug openmp clean

all: $(TARGET)

debug: CXXFLAGS += -g -O0
debug: LDFLAGS += -g
debug: clean $(TARGET)

openmp: CXXFLAGS += $(OMPFLAGS)
openmp: LDFLAGS += $(OMPLIBS)
openmp: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(call RMFILE,$(TARGET))
	$(call RMDIR,$(OBJDIR))

-include $(DEPS)
