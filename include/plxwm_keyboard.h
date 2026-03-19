#pragma once
#include "common.h"
#include "plxwm_signal.h"

namespace PlxWM {

class Server;

class Keyboard {
public:
    Keyboard() {}
    Keyboard(Server *server, wlr_input_device *device);

    void onModifiers(wl_listener *listener, void *data);
    void onKey(wl_listener *listener, wlr_keyboard_key_event *event);
    void onDestroy(wl_listener *listener, void *data);

    bool handleKeyBinding(xkb_keysym_t sym);
private:
    Server *server;
    wlr_input_device *device;

	wlr_keyboard *keyboard;

    unique_ptr<Signal<&Keyboard::onModifiers>> modifiers;
    unique_ptr<Signal<&Keyboard::onKey>> key;
    unique_ptr<Signal<&Keyboard::onDestroy>> destroy;
};

}