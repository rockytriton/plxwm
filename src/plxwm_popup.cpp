#include "plxwm_popup.h"
#include "plxwm_server.h"
#include "plxwm_appwindow.h"

namespace PlxWM {

Popup::Popup(Server *server, wlr_xdg_popup *popup) {
    this->server = server;
    this->popup = popup;

	wlr_scene_tree *parent_tree = nullptr;
	
	printf("1: %p\n", popup);
	printf("1aa: %p\n", popup->parent);

    // 1. Try standard XDG Parent (like Konsole's menus)
    wlr_xdg_surface *xdg_parent = wlr_xdg_surface_try_from_wlr_surface(popup->parent);
    
	printf("1a: %p - %p\n", xdg_parent);

    if (xdg_parent != nullptr) {
        parent_tree = (wlr_scene_tree *)xdg_parent->data;
		printf("1b: %p - %p\n", xdg_parent->data, parent_tree);
    } 
    // 2. The "Cheat": Check for the Layer Shell role by string name
    else if (popup->parent->role != nullptr && 
             strcmp(popup->parent->role->name, "wlr_layer_surface_v1") == 0) {
        
		printf("2\n");
        // If it's a layer surface, we find the tree we hid in the .data pointer
        // (Assuming you stored it there in your Panel class)
        parent_tree = (wlr_scene_tree *)popup->parent->data;
    } else {

		printf("3\n");
		printf("%p\n", popup->parent->role);
	}

    // Fallback if we can't find anything (puts it on the top layer)
    if (!parent_tree) {
        parent_tree = (wlr_scene_tree *)&server->getScene()->tree; 
    }

    // Create the scene object
    wlr_scene_tree *popup_tree = wlr_scene_xdg_surface_create(parent_tree, popup->base);
	printf("PT: %p\n", popup_tree);
    popup->base->data = popup_tree;

	commit = make_unique<Signal<&Popup::onCommit>>(this, &popup->base->surface->events.commit);
	destroy = make_unique<Signal<&Popup::onDestroy>>(this, &popup->events.destroy);
}

void Popup::onCommit(wl_listener *listener, void *data) {
    printf("ON POPUP COMMIT\n");

    if (popup->base->initial_commit) {
		wlr_xdg_surface_schedule_configure(popup->base);
	}
}

void Popup::onDestroy(wl_listener *listener, void *data) {
    printf("ON POPUP DESTROY\n");

    commit->cleanup();
    destroy->cleanup();
}

}

