#include "plxwm_appwindow.h"
#include "plxwm_server.h"
#include "plxwm_cursor.h"

namespace PlxWM {

void AppWindow::onMap(wl_listener *listener) {
    wl_list_insert(server->getAppWindows(), &link);
    server->focus(this);
}

void AppWindow::onUnmap(wl_listener *listener) {
	/* Called when the surface is unmapped, and should no longer be shown. */
	//struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

	/* Reset the cursor mode if the grabbed toplevel was unmapped. */
	
	if (this == server->getGrabbedWindow()) {
		cursor_mode = TINYWL_CURSOR_PASSTHROUGH;
		server->setGrabbedWindow(NULL);
	}

	wl_list_remove(&link);
}

void AppWindow::onCommit(wl_listener *listener) {

    if (xdg_toplevel->base->initial_commit) {
		//When an xdg_surface performs an initial commit, the compositor must
		// * reply with a configure so the client can map the surface. tinywl
		// * configures the xdg_toplevel with 0,0 size to let the client pick the
		// * dimensions itself. 
        wlr_xdg_toplevel_set_size(xdg_toplevel, 0, 0);
    }
}

void AppWindow::onDestroy(struct wl_listener *listener, void *data) {
    printf("onDestroy\n");
	/* Called when the xdg_toplevel is destroyed. 
	struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, destroy); */

	wl_list_remove(&map.listener.link);
	wl_list_remove(&unmap.listener.link);
	wl_list_remove(&commit.listener.link);
	wl_list_remove(&destroy.listener.link);
	wl_list_remove(&request_move.listener.link);
	wl_list_remove(&request_resize.listener.link);
	wl_list_remove(&request_maximize.listener.link);
	wl_list_remove(&request_fullscreen.listener.link);

}

void AppWindow::beginInteractive(enum tinywl_cursor_mode mode, uint32_t edges) {
	/* This function sets up an interactive move or resize operation, where the
	 * compositor stops propegating pointer events to clients and instead
	 * consumes them itself, to move or resize windows. */

	server->setGrabbedWindow(this);
	cursor_mode = mode;

	if (mode == TINYWL_CURSOR_MOVE) {
		server->getCursor()->setGrabX(server->getCursor()->getCursor()->x - getSceneTree()->node.x);
		server->getCursor()->setGrabY(server->getCursor()->getCursor()->y - getSceneTree()->node.y);
	} else {
		struct wlr_box *geo_box = &getXdgTopLevel()->base->geometry;

		double border_x = (getSceneTree()->node.x + geo_box->x) +
			((edges & WLR_EDGE_RIGHT) ? geo_box->width : 0);
		double border_y = (getSceneTree()->node.y + geo_box->y) +
			((edges & WLR_EDGE_BOTTOM) ? geo_box->height : 0);
		server->getCursor()->setGrabX(server->getCursor()->getCursor()->x - border_x);
		server->getCursor()->setGrabY(server->getCursor()->getCursor()->y - border_y);

		wlr_box gb = *geo_box;
		gb.x += getSceneTree()->node.x;
		gb.y += getSceneTree()->node.y;
		server->setGrabGeoBox(gb);

		server->setResizeEdgets(edges);
	}
}

void AppWindow::onRequestMove(wl_listener *listener, void *data) {
    printf("onRequestMove\n");
	/* This event is raised when a client would like to begin an interactive
	 * move, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want. 
	struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, request_move); */
	beginInteractive(TINYWL_CURSOR_MOVE, 0);
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

    map.listener.notify = NOTIFIER_ND(AppWindow, onMap);
	wl_signal_add(&xdg_toplevel->base->surface->events.map, &map.listener);
	unmap.listener.notify = NOTIFIER_ND(AppWindow, onUnmap);
	wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &unmap.listener);
	commit.listener.notify = NOTIFIER_ND(AppWindow, onCommit);
	wl_signal_add(&xdg_toplevel->base->surface->events.commit, &commit.listener);

	destroy.listener.notify = NOTIFIER(AppWindow, void, onDestroy);
	wl_signal_add(&xdg_toplevel->events.destroy, &destroy.listener);


	// cotd 
	request_move.listener.notify = NOTIFIER(AppWindow, void, onRequestMove);
	wl_signal_add(&xdg_toplevel->events.request_move, &request_move.listener);
	request_resize.listener.notify = xdg_toplevel_request_resize;
	wl_signal_add(&xdg_toplevel->events.request_resize, &request_resize.listener);
	request_maximize.listener.notify = xdg_toplevel_request_maximize;
	wl_signal_add(&xdg_toplevel->events.request_maximize, &request_maximize.listener);
	request_fullscreen.listener.notify = xdg_toplevel_request_fullscreen;
	wl_signal_add(&xdg_toplevel->events.request_fullscreen, &request_fullscreen.listener);


}

}
