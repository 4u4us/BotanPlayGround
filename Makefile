# Paths to relevant programs

CXX            = g++
LINKER         = $(CXX)
AR             = ar
AR_OPTIONS     = crs
PYTHON_EXE     = python

# Compiler Flags

ABI_FLAGS      =  -fstack-protector -pthread
LANG_FLAGS     = -std=c++11 -D_REENTRANT
CXXFLAGS       = -O3 -momit-leaf-frame-pointer
WARN_FLAGS     = -Wall -Wextra -Wpedantic -Wstrict-aliasing -Wcast-align -Wmissing-declarations -Wpointer-arith -Wcast-qual -Wzero-as-null-pointer-constant -Wnon-virtual-dtor
SO_OBJ_FLAGS   = -fPIC -fvisibility=hidden
LDFLAGS        = 

EXE_LINK_CMD   = $(LINKER) -Wl,-rpath=\$$ORIGIN
POST_LINK_CMD  = 

LIB_LINKS_TO   = -ldl -lrt
EXE_LINKS_TO   = -L. -lbotan-2 $(LIB_LINKS_TO)

BUILD_FLAGS    = $(ABI_FLAGS) $(LANG_FLAGS) $(CXXFLAGS) $(WARN_FLAGS)

SCRIPTS_DIR    = src/scripts
INSTALLED_LIB_DIR = /usr/local/lib

# The primary target
all: cli

# Executable targets
CLI           = ./botanTestApp
#LIBRARIES     = ./libbotan-2.a ./libbotan-2.so.8
LIBRARIES     = ./libbotan-2.a

cli: $(CLI)


# Misc targets

.PHONY = all

# Object Files

CLIOBJS = main.o

# Executable targets

$(CLI): $(CLIOBJS)
	$(EXE_LINK_CMD) $(ABI_FLAGS) $(LDFLAGS) $(CLIOBJS) $(EXE_LINKS_TO) -o $@
	$(POST_LINK_CMD)

# Build Commands

main.o: main.cpp
	$(CXX) $(BUILD_FLAGS)  -I../../botan/build/include -I../../build/include/external -c main.cpp -o $@



