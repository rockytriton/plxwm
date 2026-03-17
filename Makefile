PKG_CONFIG ?= pkg-config
WAYLAND_PROTOCOLS != $(PKG_CONFIG) --variable=pkgdatadir wayland-protocols
WAYLAND_SCANNER != $(PKG_CONFIG) --variable=wayland_scanner wayland-scanner

PKGS = "wlroots-0.19" wayland-server xkbcommon
CFLAGS_PKG_CONFIG != $(PKG_CONFIG) --cflags $(PKGS)
CFLAGS += $(CFLAGS_PKG_CONFIG)
LIBS != $(PKG_CONFIG) --libs $(PKGS)

CXX = g++
BUILD_DIR = build

# List your source files here
SRCS = plxwm.cpp plxwm_server.cpp plxwm_server_output.cpp plxwm_keyboard.cpp plxwm_cursor.cpp

# Map source files to the build directory
OBJS = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

all: plxwm

# 1. Generate the protocol header ONLY
$(BUILD_DIR)/xdg-shell-protocol.h: | $(BUILD_DIR)
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

# 2. Pattern Rule for .cpp files
# We include $(BUILD_DIR) in the include path so the header is found
$(BUILD_DIR)/%.o: %.cpp $(BUILD_DIR)/xdg-shell-protocol.h | $(BUILD_DIR)
	$(CXX) -c $< -g -Werror $(CFLAGS) -I$(BUILD_DIR) -I. -DWLR_USE_UNSTABLE -o $@

# 3. Create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 4. Link everything together
plxwm: $(OBJS)
	$(CXX) $^ -g -Werror $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -rf $(BUILD_DIR) plxwm

.PHONY: all clean