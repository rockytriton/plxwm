#include "plxwm_layerwindow.h"
#include "plxwm_server.h"
#include "plxwm_appwindow.h"
#include "plxwm_popup.h"

namespace PlxWM {

static int nnn = 0;

LayerWindow::LayerWindow(Server *server, wlr_layer_surface_v1 *surface) {
    this->server = server;
    this->surface = surface;

    struct wlr_scene_tree *parent_tree = (wlr_scene_tree *)&server->getScene()->tree;

    scene_layer_surface = wlr_scene_layer_surface_v1_create(parent_tree, surface);
    surface->data = scene_layer_surface->tree;

	commit = make_unique<Signal<&LayerWindow::onCommit>>(this, &surface->surface->events.commit);
	popup = make_unique<Signal<&LayerWindow::onPopup>>(this, &surface->events.new_popup);
	destroy = make_unique<Signal<&LayerWindow::onDestroy>>(this, &surface->events.destroy);
}

void LayerWindow::onPopup(wl_listener *listener, wlr_xdg_popup *popup) {
    printf("ON LayerWindow onPopup\n");

    wlr_scene_tree *my_tree = (wlr_scene_tree *)surface->data;

    new Popup(server, popup, my_tree, this);
}

void LayerWindow::onCommit(wl_listener *listener, void *data) {
    if (surface->initial_commit) {
        // Now it's safe to configure!
        wlr_layer_surface_v1_configure(surface, 
                                       surface->pending.desired_width, 
                                       surface->pending.desired_height);

        server->arrangeLayers();
    }

    bool changed = (surface->current.anchor != surface->pending.anchor) ||
                   (surface->current.exclusive_zone != surface->pending.exclusive_zone) ||
                   (surface->current.actual_width != surface->pending.actual_width) ||
                   (surface->current.actual_height != surface->pending.actual_height);

    if (changed) {
        printf("State changed, rearranging...\n");
        server->arrangeLayers();
    }
}

void LayerWindow::onDestroy(wl_listener *listener, void *data) {
    printf("ON LayerWindow DESTROY\n");

    commit->cleanup();
    destroy->cleanup();
    popup->cleanup();
}

}

