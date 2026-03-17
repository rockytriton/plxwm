#pragma once
#include "common.h"

namespace PlxWM {

class Server;

class Keyboard {
public:
    Keyboard() {}
    Keyboard(Server *server, wlr_input_device *device);
    void init();

    void onModifiers(wl_listener *listener);
    void onKey(wl_listener *listener, wlr_keyboard_key_event *event);
    void onDestroy(wl_listener *listener);

    bool handleKeyBinding(xkb_keysym_t sym);
private:
    Server *server;
    wlr_input_device *device;

	wl_list link;
	wlr_keyboard *keyboard;

	Listener<Keyboard> modifiers;
	Listener<Keyboard> key;
	Listener<Keyboard> destroy;
};

}