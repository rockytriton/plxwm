#include "plxwm_appwindow.h"
#include "plxwm_server.h"
#include "plxwm_cursor.h"

namespace PlxWM {

void AppWindow::onMap(wl_listener *listener, void *data) {
    printf("onMap\n");
    //wl_list_insert(server->getAppWindows(), &link);
    server->focus(this);
}

void AppWindow::onUnmap(wl_listener *listener, void *data) {
    printf("onUnmap\n");
	/* Called when the surface is unmapped, and should no longer be shown. */
	//struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

	/* Reset the cursor mode if the grabbed toplevel was unmapped. */
	
	if (this == server->getGrabbedWindow()) {
		cursor_mode = TINYWL_CURSOR_PASSTHROUGH;
		server->setGrabbedWindow(NULL);
	}

	wl_list_remove(&link);
    printf("onUnmap - done\n");
}

void AppWindow::onCommit(wl_listener *listener, void *data) {
    printf("onCommit\n");

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

	map->cleanup();
	unmap->cleanup();
	commit->cleanup();
	destroy->cleanup();

	move->cleanup();
	resize->cleanup();
	maximize->cleanup();
	fullscreen->cleanup();

	printf("On destroy done\n");
}

void AppWindow::beginInteractive(enum tinywl_cursor_mode mode, uint32_t edges) {
	/* This function sets up an interactive move or resize operation, where the
	 * compositor stops propegating pointer events to clients and instead
	 * consumes them itself, to move or resize windows. */

	server->setGrabbedWindow(this);
	cursor_mode = mode;

	if (mode == TINYWL_CURSOR_MOVE) {
		server->setGrabX(server->getCursor()->x - getSceneTree()->node.x);
		server->setGrabY(server->getCursor()->y - getSceneTree()->node.y);
	} else {
		struct wlr_box *geo_box = &getXdgTopLevel()->base->geometry;

		double border_x = (getSceneTree()->node.x + geo_box->x) +
			((edges & WLR_EDGE_RIGHT) ? geo_box->width : 0);
		double border_y = (getSceneTree()->node.y + geo_box->y) +
			((edges & WLR_EDGE_BOTTOM) ? geo_box->height : 0);
		
		server->setGrabX(server->getCursor()->x - border_x);
		server->setGrabY(server->getCursor()->y - border_y);

		wlr_box gb = *geo_box;
		gb.x += getSceneTree()->node.x;
		gb.y += getSceneTree()->node.y;
		server->setGrabGeoBox(gb);

		server->setResizeEdgets(edges);
	}
}

void AppWindow::onRequestMove(wl_listener *listener, void *data) {
    printf("onRequestMove: %p - %p\n", this, listener);
	/* This event is raised when a client would like to begin an interactive
	 * move, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want.  */
	beginInteractive(TINYWL_CURSOR_MOVE, 0);
}

void AppWindow::onRequestResize(wl_listener *listener, wlr_xdg_toplevel_resize_event *event) {
    printf("onRequestResize\n");
	/* This event is raised when a client would like to begin an interactive
	 * resize, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want.
	struct wlr_xdg_toplevel_resize_event *event = data;
	struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, request_resize);
	begin_interactive(toplevel, TINYWL_CURSOR_RESIZE, event->edges); */

	beginInteractive(TINYWL_CURSOR_RESIZE, event->edges);
}

void AppWindow::onRequestMaximize(wl_listener *listener, void *data) {
    printf("onRequestMaximize\n");
	/* This event is raised when a client would like to maximize itself,
	 * typically because the user clicked on the maximize button on client-side
	 * decorations. tinywl doesn't support maximization, but to conform to
	 * xdg-shell protocol we still must send a configure.
	 * wlr_xdg_surface_schedule_configure() is used to send an empty reply.
	 * However, if the request was sent before an initial commit, we don't do
	 * anything and let the client finish the initial surface setup. 
	struct tinywl_toplevel *toplevel =
		wl_container_of(listener, toplevel, request_maximize); */

	if (getXdgTopLevel()->base->initialized) {
		wlr_xdg_surface_schedule_configure(getXdgTopLevel()->base);
	}
}

void AppWindow::onRequestFullScreen(wl_listener *listener, void *data) {
    printf("onRequestFullScreen\n");
	/* Just as with request_maximize, we must send a configure here.
	struct tinywl_toplevel *toplevel =
		wl_container_of(listener, toplevel, request_fullscreen);
	if (toplevel->xdg_toplevel->base->initialized) {
		wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
	} */

	if (getXdgTopLevel()->base->initialized) {
		wlr_xdg_surface_schedule_configure(getXdgTopLevel()->base);
	}
}

AppWindow::AppWindow(Server *server, wlr_xdg_toplevel *xdg_toplevel) {
    this->server = server;
    this->xdg_toplevel = xdg_toplevel;

    scene_tree = wlr_scene_xdg_surface_create(&server->getScene()->tree, xdg_toplevel->base);
    scene_tree->node.data = this;
    xdg_toplevel->base->data = scene_tree;

	map = std::make_unique<Signal<&AppWindow::onMap>>(this, &xdg_toplevel->base->surface->events.map);
	unmap = std::make_unique<Signal<&AppWindow::onUnmap>>(this, &xdg_toplevel->base->surface->events.unmap);
	commit = std::make_unique<Signal<&AppWindow::onCommit>>(this, &xdg_toplevel->base->surface->events.commit);
	destroy = std::make_unique<Signal<&AppWindow::onDestroy>>(this, &xdg_toplevel->base->surface->events.destroy);

	move = std::make_unique<Signal<&AppWindow::onRequestMove>>(this, &xdg_toplevel->events.request_move);
	resize = std::make_unique<Signal<&AppWindow::onRequestResize>>(this, &xdg_toplevel->events.request_resize);
	maximize = std::make_unique<Signal<&AppWindow::onRequestMaximize>>(this, &xdg_toplevel->events.request_maximize);
	fullscreen = std::make_unique<Signal<&AppWindow::onRequestFullScreen>>(this, &xdg_toplevel->events.request_fullscreen);

}

}
