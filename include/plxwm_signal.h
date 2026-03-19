#pragma once

#include "common.h"

namespace PlxWM {

template<auto Func>
class Signal {
public:
    using C = typename member_func_traits<decltype(Func)>::class_type;
    using T = typename member_func_traits<decltype(Func)>::arg_type;

    Signal(C *instance, wl_signal *signal) {
        listenerWrapper.owner = instance;

        // Use traits to find what 'Arg' is for this specific 'Func'
        using T = typename member_func_traits<decltype(Func)>::arg_type;

        listenerWrapper.listener.notify = [](struct wl_listener *l, void *data) {
            auto *wrapper = reinterpret_cast<Listener<C> *>(l);
            
            // Now we cast void* to T* (e.g., wlr_xdg_toplevel_resize_event*)
            (wrapper->owner->*Func)(l, static_cast<T*>(data));
        };

        wl_signal_add(signal, &listenerWrapper.listener);
    }

    static std::unique_ptr<Signal<Func>>  create(C *instance, wl_signal *signal) {
        return std::make_unique<Signal<Func>>(instance, signal);
    }

    ~Signal() {
        cleanup();
    }

    void cleanup() {
        if (listenerWrapper.listener.link.next != nullptr) {
            wl_list_remove(&listenerWrapper.listener.link);
        }
    }

private:
    Listener<C> listenerWrapper;
};

}
