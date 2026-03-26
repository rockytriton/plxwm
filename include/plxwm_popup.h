#pragma once

#include "common.h"
#include "plxwm_signal.h"

namespace PlxWM {

class Server;
class LayerWindow;

class Popup {
public:
    Popup(Server *server, wlr_xdg_popup *popup, wlr_scene_tree *parent_tree, LayerWindow *layerWindow);

    void onMap(wl_listener *listener, void *data);
    void onCommit(wl_listener *listener, void *data);
    void onDestroy(wl_listener *listener, void *data);

private:
    Server *server;

	wlr_xdg_popup *popup;
    wlr_scene_tree *popup_tree;
    LayerWindow *layerWindow;
    bool positioned;

    unique_ptr<Signal<&Popup::onCommit>> commit;
    unique_ptr<Signal<&Popup::onMap>> map;
    unique_ptr<Signal<&Popup::onDestroy>> destroy;

};

}
