#pragma once
#include "common.h"
#include "plxwm_signal.h"
#include "plxwm_cursor.h"

namespace PlxWM {
class Server;

class AppWindow {
public:
    AppWindow(Server *server, wlr_xdg_toplevel *xdg_toplevel);

    wlr_xdg_toplevel *getXdgTopLevel() { return xdg_toplevel; }
    wlr_scene_tree *getSceneTree() { return scene_tree; }

    void onCommit(wl_listener *listener, void *data);
    void onMap(wl_listener *listener, void *data);
    void onUnmap(wl_listener *listener, void *data);
    void onDestroy(struct wl_listener *listener, void *data);
    void onRequestMove(wl_listener *listener, void *data);
    void onRequestResize(wl_listener *listener, wlr_xdg_toplevel_resize_event *event);
    void onRequestMaximize(wl_listener *listener, void *data);
    void onRequestFullScreen(wl_listener *listener, void *data);

    void beginInteractive(CursorMode mode, uint32_t edges);

    const char *getName() { return "name"; }

    wlr_surface *getSurface() { return xdg_toplevel->base->surface; }

private:
    Server *server;

	wlr_xdg_toplevel *xdg_toplevel;
	wlr_scene_tree *scene_tree;

    unique_ptr<Signal<&AppWindow::onRequestMove>> move;
    unique_ptr<Signal<&AppWindow::onRequestResize>> resize;
    unique_ptr<Signal<&AppWindow::onRequestMaximize>> maximize;
    unique_ptr<Signal<&AppWindow::onRequestFullScreen>> fullscreen;

    unique_ptr<Signal<&AppWindow::onMap>> map;
    unique_ptr<Signal<&AppWindow::onUnmap>> unmap;
    unique_ptr<Signal<&AppWindow::onCommit>> commit;
    unique_ptr<Signal<&AppWindow::onDestroy>> destroy;

};

}
