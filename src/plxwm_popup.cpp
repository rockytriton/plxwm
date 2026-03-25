#include "plxwm_popup.h"
#include "plxwm_server.h"
#include "plxwm_appwindow.h"

namespace PlxWM {

Popup::Popup(Server *server, wlr_xdg_popup *popup, wlr_scene_tree *parent_tree) {
    this->server = server;
    this->popup = popup;

    // Fallback if we can't find anything (puts it on the top layer)
    if (!parent_tree) {
        
        if (popup->parent == nullptr) {
            parent_tree = (wlr_scene_tree *)&server->getScene()->tree; 
        } else {
            wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(popup->parent);

            if (parent != nullptr) {
                parent_tree = (wlr_scene_tree *)parent->data;
            }
        }
    }

    // Create the scene object
    popup_tree = wlr_scene_xdg_surface_create(parent_tree, popup->base);
    
    popup->base->data = popup_tree;

    wlr_scene_node_set_position(&popup_tree->node, 
                                 popup->pending.geometry.x, 
                                 popup->pending.geometry.y);

	commit = make_unique<Signal<&Popup::onCommit>>(this, &popup->base->surface->events.commit);
	destroy = make_unique<Signal<&Popup::onDestroy>>(this, &popup->events.destroy);
}

void Popup::onCommit(wl_listener *listener, void *data) {

    if (popup->base->initial_commit) {
		wlr_xdg_surface_schedule_configure(popup->base);
	}

    if (popup->base->surface->mapped) {
        wlr_scene_node *node = &((wlr_scene_tree *)popup->base->data)->node;
        
        int x = popup->pending.geometry.x;
        int y = popup->pending.geometry.y;

        if (popup->parent != nullptr && 
            popup->parent->role != nullptr && 
            strcmp(popup->parent->role->name, "zwlr_layer_surface_v1") == 0) {
            
            wlr_scene_node_set_position(node, x, y + 125);
        } else {
            wlr_scene_node_set_position(node, x, y);
        }
                                     
        // Explicitly ensure the node is enabled
        wlr_scene_node_set_enabled(node, true);
    }
}

void Popup::onDestroy(wl_listener *listener, void *data) {
    commit->cleanup();
    destroy->cleanup();
}

}

