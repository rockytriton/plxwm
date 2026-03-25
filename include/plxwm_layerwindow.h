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
    void onPopup(wl_listener *listener, wlr_xdg_popup *popup);

    wlr_layer_surface_v1 *getSurface() { return surface; }
    wlr_scene_layer_surface_v1 *getSceneSurface() { return scene_layer_surface; }
    
private:
    Server *server;

	wlr_layer_surface_v1 *surface;
    wlr_scene_layer_surface_v1 *scene_layer_surface;

    unique_ptr<Signal<&LayerWindow::onCommit>> commit;
    unique_ptr<Signal<&LayerWindow::onPopup>> popup;
    unique_ptr<Signal<&LayerWindow::onDestroy>> destroy;

};

}
