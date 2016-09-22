UNAME := $(shell uname -s)
ifeq ($(UNAME),Linux)
  OS = Linux
endif
ifeq ($(UNAME),Darwin)
  OS = OSX
endif

CXX = clang++-3.6
CXXFLAGS = -std=c++11 -Wno-deprecated-register

SRCDIR = src
OUTDIR = out
OBJDIR = $(OUTDIR)/obj
BINDIR = $(OUTDIR)/bin
PWD = $(shell pwd)
ifeq ($(OS), Linux)
  PKG_CONFIG_PATH="third_party/opencv3/lib/pkgconfig"
else
  PKG_CONFIG_PATH=
endif
PACKAGES = "libglog goocanvas"

INCLUDE = -I$(SRCDIR) $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config $(PACKAGES) --cflags-only-I)
LDFLAGS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config $(PACKAGES) --libs)
ifeq ($(OS), OSX)
  INCLUDE += -I/opt/local/include
endif


CC_FILES = $(shell find $(SRCDIR) -name "*.cc")
OBJ_FILES = $(addprefix $(OBJDIR)/, $(patsubst %.cc, %.o, $(CC_FILES)))

$(BINDIR)/fonttest: $(OBJ_FILES)
	@if [ ! -d $(dir $@) ]; then \
		echo "MKDIR $(dir $@)"; mkdir -p $(dir $@); \
	fi
	@echo "LINK $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: %.cc
	@if [ ! -d $(dir $@) ]; then \
		echo "MKDIR $(dir $@)"; mkdir -p $(dir $@); \
	fi
	@echo "CXX $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

clean:
	@rm -fr $(OUTDIR)
