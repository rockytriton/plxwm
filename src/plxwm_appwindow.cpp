#include "plxwm_appwindow.h"
#include "plxwm_server.h"
#include "plxwm_cursor.h"

namespace PlxWM {

void AppWindow::onMap(wl_listener *listener, void *data) {
	wlr_output *output = wlr_output_layout_output_at(server->getOutputLayout(), 
                                                       server->getCursor()->x, 
                                                       server->getCursor()->y);

	// 2. If no output is under the cursor, fallback to the first one
	if (!output) {
		output = wlr_output_layout_get_center_output(server->getOutputLayout());
	}

	int width = getSurface()->current.width;
    int height = getSurface()->current.height;

	// 3. Extract the width and height
	int output_width = output->width;
	int output_height = output->height;
	
	wlr_output_effective_resolution(output, &output_width, &output_height);

	// In plxwm_appwindow.cpp
	if (xdg_toplevel->pending.width < 600 && xdg_toplevel->pending.height < 400) {
		// Center the window on the output
		
		printf("%d, %d, %d, %d\n", output_width, output_height, width, height);
		
		int center_x = (output_width / 2) - (width / 2);
		int center_y = (output_height / 2) - (height / 2);

		wlr_scene_node_set_position(&scene_tree->node, center_x, center_y);
	}

    server->focus(this);
}

void AppWindow::onUnmap(wl_listener *listener, void *data) {
	if (this == server->getGrabbedWindow()) {
		server->setCursorMode(TINYWL_CURSOR_PASSTHROUGH);
		server->setGrabbedWindow(NULL);
	}
}

void AppWindow::onCommit(wl_listener *listener, void *data) {
    if (xdg_toplevel->base->initial_commit) {
        wlr_xdg_toplevel_set_size(xdg_toplevel, 0, 0);
    }
}

void AppWindow::onDestroy(struct wl_listener *listener, void *data) {
	map->cleanup();
	unmap->cleanup();
	commit->cleanup();
	destroy->cleanup();
	move->cleanup();
	resize->cleanup();
	maximize->cleanup();
	fullscreen->cleanup();
}

void AppWindow::beginInteractive(CursorMode mode, uint32_t edges) {
	server->setGrabbedWindow(this);
	server->setCursorMode(mode);

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
	beginInteractive(TINYWL_CURSOR_MOVE, 0);
}

void AppWindow::onRequestResize(wl_listener *listener, wlr_xdg_toplevel_resize_event *event) {
	beginInteractive(TINYWL_CURSOR_RESIZE, event->edges);
}

void AppWindow::onRequestMaximize(wl_listener *listener, void *data) {
	//not yet supported...

	if (getXdgTopLevel()->base->initialized) {
		wlr_xdg_surface_schedule_configure(getXdgTopLevel()->base);
	}
}

void AppWindow::onRequestFullScreen(wl_listener *listener, void *data) {
    //not yet supported...

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

	map = make_unique<Signal<&AppWindow::onMap>>(this, &xdg_toplevel->base->surface->events.map);
	unmap = make_unique<Signal<&AppWindow::onUnmap>>(this, &xdg_toplevel->base->surface->events.unmap);
	commit = make_unique<Signal<&AppWindow::onCommit>>(this, &xdg_toplevel->base->surface->events.commit);
	destroy = make_unique<Signal<&AppWindow::onDestroy>>(this, &xdg_toplevel->events.destroy);

	move = make_unique<Signal<&AppWindow::onRequestMove>>(this, &xdg_toplevel->events.request_move);
	resize = make_unique<Signal<&AppWindow::onRequestResize>>(this, &xdg_toplevel->events.request_resize);
	maximize = make_unique<Signal<&AppWindow::onRequestMaximize>>(this, &xdg_toplevel->events.request_maximize);
	fullscreen = make_unique<Signal<&AppWindow::onRequestFullScreen>>(this, &xdg_toplevel->events.request_fullscreen);

}

}
