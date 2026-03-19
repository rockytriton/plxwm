#include "plxwm_appwindow.h"
#include "plxwm_server.h"

namespace PlxWM {



static void xdg_toplevel_map(struct wl_listener *listener, void *data) {
	/* Called when the surface is mapped, or ready to display on-screen. */
	//struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, map);

    AppWindow *kb = ((Listener<AppWindow> *)listener)->owner;
	kb->onMap(&((Listener<AppWindow> *)listener)->listener);
}

static void xdg_toplevel_unmap(struct wl_listener *listener, void *data) {
    printf("xdg_toplevel_unmap\n");
	/* Called when the surface is unmapped, and should no longer be shown. */
	//struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

	/* Reset the cursor mode if the grabbed toplevel was unmapped. */
	//if (toplevel == toplevel->server->grabbed_toplevel) {
	//	reset_cursor_mode(toplevel->server);
	//}

	//wl_list_remove(&toplevel->link);
}

void AppWindow::onMap(wl_listener *listener) {
    printf("ON onMap: %p\n", server);
    wl_list_insert(server->getAppWindows(), &link);
    server->focus(this);
    printf("Focus\n");
}

void AppWindow::onUnmap(wl_listener *listener) {

}

void AppWindow::onCommit(wl_listener *listener) {
    printf("ON onCommit\n");

    if (xdg_toplevel->base->initial_commit) {
		//When an xdg_surface performs an initial commit, the compositor must
		// * reply with a configure so the client can map the surface. tinywl
		// * configures the xdg_toplevel with 0,0 size to let the client pick the
		// * dimensions itself. 
        wlr_xdg_toplevel_set_size(xdg_toplevel, 0, 0);
    }
}

static void xdg_toplevel_commit(struct wl_listener *listener, void *data) {
    AppWindow *kb = ((Listener<AppWindow> *)listener)->owner;
	kb->onCommit(&((Listener<AppWindow> *)listener)->listener);

}

static void xdg_toplevel_destroy(struct wl_listener *listener, void *data) {
    printf("xdg_toplevel_destroy\n");
	/* Called when the xdg_toplevel is destroyed. 
	struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, destroy);

	wl_list_remove(&toplevel->map.link);
	wl_list_remove(&toplevel->unmap.link);
	wl_list_remove(&toplevel->commit.link);
	wl_list_remove(&toplevel->destroy.link);
	wl_list_remove(&toplevel->request_move.link);
	wl_list_remove(&toplevel->request_resize.link);
	wl_list_remove(&toplevel->request_maximize.link);
	wl_list_remove(&toplevel->request_fullscreen.link);

	free(toplevel);*/
}



static void xdg_toplevel_request_move(
		struct wl_listener *listener, void *data) {
    printf("xdg_toplevel_request_move\n");
	/* This event is raised when a client would like to begin an interactive
	 * move, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want. 
	struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, request_move);
	begin_interactive(toplevel, TINYWL_CURSOR_MOVE, 0);*/
}

static void xdg_toplevel_request_resize(
		struct wl_listener *listener, void *data) {
    printf("xdg_toplevel_request_resize\n");
	/* This event is raised when a client would like to begin an interactive
	 * resize, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want.
	struct wlr_xdg_toplevel_resize_event *event = data;
	struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, request_resize);
	begin_interactive(toplevel, TINYWL_CURSOR_RESIZE, event->edges); */
}

static void xdg_toplevel_request_maximize(
		struct wl_listener *listener, void *data) {
    printf("xdg_toplevel_request_maximize\n");
	/* This event is raised when a client would like to maximize itself,
	 * typically because the user clicked on the maximize button on client-side
	 * decorations. tinywl doesn't support maximization, but to conform to
	 * xdg-shell protocol we still must send a configure.
	 * wlr_xdg_surface_schedule_configure() is used to send an empty reply.
	 * However, if the request was sent before an initial commit, we don't do
	 * anything and let the client finish the initial surface setup. 
	struct tinywl_toplevel *toplevel =
		wl_container_of(listener, toplevel, request_maximize);
	if (toplevel->xdg_toplevel->base->initialized) {
		wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
	}*/
}

static void xdg_toplevel_request_fullscreen(
		struct wl_listener *listener, void *data) {
    printf("xdg_toplevel_request_fullscreen\n");
	/* Just as with request_maximize, we must send a configure here.
	struct tinywl_toplevel *toplevel =
		wl_container_of(listener, toplevel, request_fullscreen);
	if (toplevel->xdg_toplevel->base->initialized) {
		wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
	} */
}

AppWindow::AppWindow(Server *server, wlr_xdg_toplevel *xdg_toplevel) {
    this->server = server;
    this->xdg_toplevel = xdg_toplevel;

    map.owner = this;
    unmap.owner = this;
    commit.owner = this;
    destroy.owner = this;
    request_move.owner = this;
    request_resize.owner = this;
    request_maximize.owner = this;
    request_fullscreen.owner = this;

    scene_tree = wlr_scene_xdg_surface_create(&server->getScene()->tree, xdg_toplevel->base);
    scene_tree->node.data = this;
    xdg_toplevel->base->data = scene_tree;

    map.listener.notify = xdg_toplevel_map;
	wl_signal_add(&xdg_toplevel->base->surface->events.map, &map.listener);
	unmap.listener.notify = xdg_toplevel_unmap;
	wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &unmap.listener);
	commit.listener.notify = xdg_toplevel_commit;
	wl_signal_add(&xdg_toplevel->base->surface->events.commit, &commit.listener);

	destroy.listener.notify = xdg_toplevel_destroy;
	wl_signal_add(&xdg_toplevel->events.destroy, &destroy.listener);


	// cotd 
	request_move.listener.notify = xdg_toplevel_request_move;
	wl_signal_add(&xdg_toplevel->events.request_move, &request_move.listener);
	request_resize.listener.notify = xdg_toplevel_request_resize;
	wl_signal_add(&xdg_toplevel->events.request_resize, &request_resize.listener);
	request_maximize.listener.notify = xdg_toplevel_request_maximize;
	wl_signal_add(&xdg_toplevel->events.request_maximize, &request_maximize.listener);
	request_fullscreen.listener.notify = xdg_toplevel_request_fullscreen;
	wl_signal_add(&xdg_toplevel->events.request_fullscreen, &request_fullscreen.listener);

}

}
