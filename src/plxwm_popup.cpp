#include "plxwm_popup.h"
#include "plxwm_server.h"
#include "plxwm_appwindow.h"
#include "plxwm_layerwindow.h"

namespace PlxWM {

Popup::Popup(Server *server, wlr_xdg_popup *popup, wlr_scene_tree *parent_tree, LayerWindow *LayerWindow) {
    this->server = server;
    this->popup = popup;

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

    popup_tree = wlr_scene_xdg_surface_create(parent_tree, popup->base);
    popup->base->data = popup_tree;
    
	map = make_unique<Signal<&Popup::onMap>>(this, &popup->base->surface->events.map);
	commit = make_unique<Signal<&Popup::onCommit>>(this, &popup->base->surface->events.commit);
	destroy = make_unique<Signal<&Popup::onDestroy>>(this, &popup->events.destroy);
}

void Popup::onMap(wl_listener *listener, void *data) {
    
}

void Popup::onCommit(wl_listener *listener, void *data) {

    if (popup->base->initial_commit) {
		wlr_xdg_surface_schedule_configure(popup->base);
	}


    if (popup->base->surface->mapped) {
        wlr_scene_node *node = &((wlr_scene_tree *)popup->base->data)->node;
        
        wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(popup->parent);

        wlr_box output_box = {0};
        output_box.width = 1024;
        output_box.height = 768;
        wlr_output_effective_resolution(server->getActiveOutput(), &output_box.width, &output_box.height);

        int lx, ly;
        wlr_xdg_popup_get_toplevel_coords(popup, 
                                        popup->current.geometry.x, 
                                        popup->current.geometry.y, 
                                        &lx, &ly);

        int x = popup->pending.geometry.x;
        int y = popup->pending.geometry.y;

        int w = popup->pending.geometry.width;
        int h = popup->pending.geometry.height;

        int diffY = output_box.height - (ly + h);
        int diffX = output_box.width - (lx + w);

        //printf("GEOM: %d,%d - %d,%d - %d,%d\n", w, h, x, y, lx, ly);

        //printf("DIFF: %d, %d\n", diffX, diffY);

        if (diffY < 0) {
            y += diffY;
        }
        if (diffX < 0) {
            x += diffX;
        }

        if (popup->parent != nullptr && 
            popup->parent->role != nullptr && 
            strcmp(popup->parent->role->name, "zwlr_layer_surface_v1") == 0) {
            
            wlr_scene_node_set_position(node, x, y);
            //printf("\tPOPUP ON_COMMIT: zwlr_layer_surface_v1\n");
        } else {
            wlr_scene_node_set_position(node, x, y);
            //printf("\tPOPUP ON_COMMIT: %s\n", popup->parent->role->name);
        }
                                        
        wlr_scene_node_set_enabled(node, true);
    }
}

void Popup::onDestroy(wl_listener *listener, void *data) {
    commit->cleanup();
    destroy->cleanup();
    map->cleanup();
}

}

