#pragma once
#include "common.h"
#include "plxwm_signal.h"

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

    void beginInteractive(enum tinywl_cursor_mode mode, uint32_t edges);

    const char *getName() { return "name"; }

    wlr_surface *getSurface() { return xdg_toplevel->base->surface; }

private:
    Server *server;

	wlr_xdg_toplevel *xdg_toplevel;
	wlr_scene_tree *scene_tree;

    std::unique_ptr<Signal<&AppWindow::onRequestMove>> move;
    std::unique_ptr<Signal<&AppWindow::onRequestResize>> resize;
    std::unique_ptr<Signal<&AppWindow::onRequestMaximize>> maximize;
    std::unique_ptr<Signal<&AppWindow::onRequestFullScreen>> fullscreen;

    std::unique_ptr<Signal<&AppWindow::onMap>> map;
    std::unique_ptr<Signal<&AppWindow::onUnmap>> unmap;
    std::unique_ptr<Signal<&AppWindow::onCommit>> commit;
    std::unique_ptr<Signal<&AppWindow::onDestroy>> destroy;

};

}
