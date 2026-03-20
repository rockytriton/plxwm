#include "plxwm_popup.h"
#include "plxwm_server.h"
#include "plxwm_appwindow.h"

namespace PlxWM {

Popup::Popup(Server *server, wlr_xdg_popup *popup) {
    this->server = server;
    this->popup = popup;

	wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(popup->parent);
	assert(parent != NULL);

	wlr_scene_tree *parent_tree = (wlr_scene_tree *)parent->data;
	popup->base->data = wlr_scene_xdg_surface_create(parent_tree, popup->base);

	commit = make_unique<Signal<&Popup::onCommit>>(this, &popup->base->surface->events.commit);
	destroy = make_unique<Signal<&Popup::onDestroy>>(this, &popup->events.destroy);
}

void Popup::onCommit(wl_listener *listener, void *data) {
    printf("ON POPUP COMMIT\n");

    if (popup->base->initial_commit) {
		/* When an xdg_surface performs an initial commit, the compositor must
		 * reply with a configure so the client can map the surface.
		 * tinywl sends an empty configure. A more sophisticated compositor
		 * might change an xdg_popup's geometry to ensure it's not positioned
		 * off-screen, for example. */
		wlr_xdg_surface_schedule_configure(popup->base);
	}
}

void Popup::onDestroy(wl_listener *listener, void *data) {
    printf("ON POPUP DESTROY\n");

    commit->cleanup();
    destroy->cleanup();
}
}

