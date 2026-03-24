#pragma once

#include "common.h"
#include "plxwm_signal.h"

namespace PlxWM {

class Server;

class LayerWindow {
public:
    LayerWindow(Server *server, wlr_layer_surface_v1 *surface);

    void onCommit(wl_listener *listener, void *data);
    void onDestroy(wl_listener *listener, void *data);

private:
    Server *server;

	wlr_layer_surface_v1 *surface;

    unique_ptr<Signal<&LayerWindow::onCommit>> commit;
    unique_ptr<Signal<&LayerWindow::onDestroy>> destroy;

};

}
