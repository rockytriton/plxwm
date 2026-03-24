#include "plxwm_layerwindow.h"
#include "plxwm_server.h"
#include "plxwm_appwindow.h"

namespace PlxWM {

LayerWindow::LayerWindow(Server *server, wlr_layer_surface_v1 *surface) {
    this->server = server;
    this->surface = surface;

    struct wlr_scene_tree *parent_tree = (wlr_scene_tree *)&server->getScene()->tree;

	printf("1: %p\n", parent_tree);

    // 2. Create the scene graph node for the panel
    auto *scene_layer_surface = wlr_scene_layer_surface_v1_create(parent_tree, surface);
    
	printf("2: %p\n", scene_layer_surface);

    // 3. THE KEY: Save the scene tree into the surface's data pointer
    // This is what your Popup constructor Case 2 is looking for!
    surface->data = scene_layer_surface->tree;

	commit = make_unique<Signal<&LayerWindow::onCommit>>(this, &surface->surface->events.commit);
	destroy = make_unique<Signal<&LayerWindow::onDestroy>>(this, &surface->events.destroy);
}

void LayerWindow::onCommit(wl_listener *listener, void *data) {
    printf("ON LayerWindow COMMIT\n");

    if (surface->initial_commit) {
        // Now it's safe to configure!
        wlr_layer_surface_v1_configure(surface, 
                                       surface->pending.desired_width, 
                                       surface->pending.desired_height);
    }
}

void LayerWindow::onDestroy(wl_listener *listener, void *data) {
    printf("ON LayerWindow DESTROY\n");

    commit->cleanup();
    destroy->cleanup();
}

}

