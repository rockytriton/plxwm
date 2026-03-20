#pragma once

#include "common.h"
#include "plxwm_signal.h"

namespace PlxWM {

class Server;

class Popup {
public:
    Popup(Server *server, wlr_xdg_popup *popup);

    void onCommit(wl_listener *listener, void *data);
    void onDestroy(wl_listener *listener, void *data);

private:
    Server *server;

	wlr_xdg_popup *popup;

    unique_ptr<Signal<&Popup::onCommit>> commit;
    unique_ptr<Signal<&Popup::onDestroy>> destroy;

};

}
