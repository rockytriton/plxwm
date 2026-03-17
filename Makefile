PKG_CONFIG ?= pkg-config
WAYLAND_PROTOCOLS != $(PKG_CONFIG) --variable=pkgdatadir wayland-protocols
WAYLAND_SCANNER != $(PKG_CONFIG) --variable=wayland_scanner wayland-scanner

PKGS = "wlroots-0.19" wayland-server xkbcommon
CFLAGS_PKG_CONFIG != $(PKG_CONFIG) --cflags $(PKGS)
CFLAGS += $(CFLAGS_PKG_CONFIG)
LIBS != $(PKG_CONFIG) --libs $(PKGS)

CXX = g++
BUILD_DIR = build
SRC_DIR = src

# List your source files (filenames only)
SRCS_FILES = plxwm.cpp plxwm_server.cpp plxwm_server_output.cpp plxwm_keyboard.cpp plxwm_cursor.cpp

# Map the source filenames to their actual location in src/
SRCS = $(addprefix $(SRC_DIR)/, $(SRCS_FILES))

# Map the source files to the build directory for object files
# This turns "src/plxwm.cpp" into "build/plxwm.o"
OBJS = $(SRCS_FILES:%.cpp=$(BUILD_DIR)/%.o)

all: plxwm

# 1. Generate the protocol header
$(BUILD_DIR)/xdg-shell-protocol.h: | $(BUILD_DIR)
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

# 2. Pattern Rule for .cpp files
# This tells Make: "To build build/foo.o, look for src/foo.cpp"
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(BUILD_DIR)/xdg-shell-protocol.h | $(BUILD_DIR)
	$(CXX) -c $< -g -Werror $(CFLAGS) -I$(BUILD_DIR) -Iinclude -I$(SRC_DIR) -DWLR_USE_UNSTABLE -o $@

# 3. Create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 4. Link everything together
plxwm: $(OBJS)
	$(CXX) $^ -g -Werror $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -rf $(BUILD_DIR) plxwm

.PHONY: all clean