PKG_CONFIG ?= pkg-config
WAYLAND_PROTOCOLS != $(PKG_CONFIG) --variable=pkgdatadir wayland-protocols
WAYLAND_SCANNER != $(PKG_CONFIG) --variable=wayland_scanner wayland-scanner

# --- Compositor Settings ---
PKGS = "wlroots-0.19" wayland-server xkbcommon
CFLAGS_PKG_CONFIG != $(PKG_CONFIG) --cflags $(PKGS)
LIBS != $(PKG_CONFIG) --libs $(PKGS)

# --- External: Panel Settings ---
# Added wayland-client and gdk-wayland-4.0
# Added -DGDK_WINDOWING_WAYLAND to CFLAGS
PANEL_PKGS = gtk4 gtk4-layer-shell-0 glib-2.0 wayland-client
PANEL_CFLAGS != $(PKG_CONFIG) --cflags $(PANEL_PKGS) 
PANEL_LIBS != $(PKG_CONFIG) --libs $(PANEL_PKGS)

CXX = g++
CC = gcc
BUILD_DIR = build
SRC_DIR = src

# Compositor Sources
SRCS_FILES = plxwm.cpp plxwm_server.cpp plxwm_server_output.cpp plxwm_keyboard.cpp \
             plxwm_cursor.cpp plxwm_appwindow.cpp plxwm_popup.cpp plxwm_layerwindow.cpp plxwm_wayland.cpp
# Add the protocol object to compositor
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS_FILES:.cpp=.o)) $(BUILD_DIR)/plxwm-ctrl-protocol.o

# Panel Sources
PANEL_DIR = ext/panel
PANEL_SRC_DIR = $(PANEL_DIR)/src
PANEL_BUILD_DIR = $(BUILD_DIR)/panel
PANEL_SRCS = $(wildcard $(PANEL_SRC_DIR)/*.cpp)
# Add the protocol object to panel
PANEL_OBJS = $(patsubst $(PANEL_SRC_DIR)/%.cpp, $(PANEL_BUILD_DIR)/%.o, $(PANEL_SRCS)) \
             $(BUILD_DIR)/plxwm-ctrl-protocol.o

# --- Rules ---

all: plxwm plxwm-panel

# 1. Protocols
$(BUILD_DIR)/xdg-shell-protocol.h: | $(BUILD_DIR)
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

$(BUILD_DIR)/wlr-layer-shell-unstable-v1-protocol.h: protocols/wlr-layer-shell-unstable-v1.xml | $(BUILD_DIR)
	$(WAYLAND_SCANNER) server-header $< $@

# Common protocol glue code
$(BUILD_DIR)/plxwm-ctrl-protocol.h: protocols/plxwm-ctrl.xml | $(BUILD_DIR)
	$(WAYLAND_SCANNER) server-header $< $@

$(BUILD_DIR)/plxwm-ctrl-protocol.c: protocols/plxwm-ctrl.xml | $(BUILD_DIR)
	$(WAYLAND_SCANNER) private-code  $< $@

$(BUILD_DIR)/plxwm-ctrl-client-protocol.h: protocols/plxwm-ctrl.xml | $(BUILD_DIR)
	$(WAYLAND_SCANNER) client-header $< $@

# 2. Build Directories
$(BUILD_DIR) $(PANEL_BUILD_DIR):
	mkdir -p $@

# 3. Protocol Implementation Object (The binary C glue)
$(BUILD_DIR)/plxwm-ctrl-protocol.o: $(BUILD_DIR)/plxwm-ctrl-protocol.c | $(BUILD_DIR)
	$(CC) -c $< $(CFLAGS_PKG_CONFIG) -fPIC -o $@

# 4. Compositor Object Pattern Rule
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(BUILD_DIR)/xdg-shell-protocol.h $(BUILD_DIR)/wlr-layer-shell-unstable-v1-protocol.h $(BUILD_DIR)/plxwm-ctrl-protocol.h | $(BUILD_DIR)
	$(CXX) -c $< -g -Werror $(CFLAGS) $(CFLAGS_PKG_CONFIG) -I$(BUILD_DIR) -Iinclude -I$(SRC_DIR) -DWLR_USE_UNSTABLE -o $@

# 5. Panel Object Pattern Rule
# Ensure it depends on the client-header
$(PANEL_BUILD_DIR)/%.o: $(PANEL_SRC_DIR)/%.cpp $(BUILD_DIR)/plxwm-ctrl-client-protocol.h | $(PANEL_BUILD_DIR)
	$(CXX) -c $< -g -Werror $(PANEL_CFLAGS) -I$(PANEL_DIR)/include -I$(BUILD_DIR) -o $@

# 6. Link Compositor
plxwm: $(OBJS)
	$(CXX) $^ -g -Werror $(CFLAGS) $(CFLAGS_PKG_CONFIG) $(LDFLAGS) $(LIBS) -o $@

# 7. Link Panel
plxwm-panel: $(PANEL_OBJS)
	$(CXX) $^ -g -Werror $(PANEL_LIBS) -o $@

clean:
	rm -rf $(BUILD_DIR) plxwm plxwm-panel

.PHONY: all clean