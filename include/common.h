#pragma once

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <type_traits>

#include <memory>

#include <vector>
using std::vector;

#ifdef __cplusplus
extern "C" {
#define static
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#undef static
}
#endif

#define NOTIFIER(X,Y,Z) [](wl_listener *listener, void *data) { \
	X *obj = ((Listener<X> *)listener)->owner; \
	Y *val = (Y *)data; \
	obj->Z(&((Listener<X> *)listener)->listener, val); \
}

#define NOTIFIER_ND(X,Z) [](wl_listener *listener, void *data) { \
	X *obj = ((Listener<X> *)listener)->owner; \
	obj->Z(&((Listener<X> *)listener)->listener); \
}


struct tinywl_output {
	struct wl_list link;
	struct tinywl_server *server;
	struct wlr_output *wlr_output;
	struct wl_listener frame;
	struct wl_listener request_state;
	struct wl_listener destroy;
};

struct tinywl_toplevel {
	struct wl_list link;
	struct tinywl_server *server;
	struct wlr_xdg_toplevel *xdg_toplevel;
	struct wlr_scene_tree *scene_tree;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener commit;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;
};

struct tinywl_popup {
	struct wlr_xdg_popup *xdg_popup;
	struct wl_listener commit;
	struct wl_listener destroy;
};

struct tinywl_keyboard {
	struct wl_list link;
	struct tinywl_server *server;
	struct wlr_keyboard *wlr_keyboard;

	struct wl_listener modifiers;
	struct wl_listener key;
	struct wl_listener destroy;
};

template<typename T>
struct Listener {
    wl_listener listener;
    T *owner;
};

#include <xkbcommon/xkbcommon.h>


enum tinywl_cursor_mode {
	TINYWL_CURSOR_PASSTHROUGH,
	TINYWL_CURSOR_MOVE,
	TINYWL_CURSOR_RESIZE,
};

extern tinywl_cursor_mode cursor_mode;

// 1. The base template (empty)
template <typename T>
struct member_func_traits;

// 2. The specialization that "extracts" the types
// Note: We use 'struct wl_listener' to match Wayland's C headers
template <typename Class, typename Arg>
struct member_func_traits<void (Class::*)(struct wl_listener*, Arg*)> {
    using class_type = Class;
    using arg_type = Arg;
};

// 3. Optional: Add a specialization for 'const' member functions just in case
template <typename Class, typename Arg>
struct member_func_traits<void (Class::*)(struct wl_listener*, Arg*) const> {
    using class_type = Class;
    using arg_type = Arg;
};

template<auto Func, typename C>
void add_signal(C *instance, Listener<C> &listener, struct wl_signal *signal) {
    listener.owner = instance;

    // Use traits to find what 'Arg' is for this specific 'Func'
    using T = typename member_func_traits<decltype(Func)>::arg_type;

    listener.listener.notify = [](struct wl_listener *l, void *data) {
        auto *wrapper = reinterpret_cast<Listener<C> *>(l);
        
        // Now we cast void* to T* (e.g., wlr_xdg_toplevel_resize_event*)
        (wrapper->owner->*Func)(l, static_cast<T*>(data));
    };

    wl_signal_add(signal, &listener.listener);
}

template<auto Func, typename C>
void add_signal_noparam(C *instance, Listener<C> &listener, struct wl_signal *signal) {
    listener.owner = instance;

    listener.listener.notify = [](struct wl_listener *l, void *data) {
        auto *wrapper = reinterpret_cast<Listener<C> *>(l);
        (wrapper->owner->*Func)(l);
    };

    wl_signal_add(signal, &listener.listener);
}

