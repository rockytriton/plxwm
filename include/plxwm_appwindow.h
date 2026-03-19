#pragma once
#include "common.h"

namespace PlxWM {
class Server;

class AppWindow {
public:
    AppWindow() {}
    AppWindow(Server *server, wlr_xdg_toplevel *xdg_toplevel);

    wlr_xdg_toplevel *getXdgTopLevel() { return xdg_toplevel; }
    wlr_scene_tree *getSceneTree() { return scene_tree; }

    void onCommit(wl_listener *listener);
    void onMap(wl_listener *listener);
    void onUnmap(wl_listener *listener);

    const char *getName() { return "name"; }

    wlr_surface *getSurface() { return xdg_toplevel->base->surface; }

    wl_list *getLink() { return &link; }
private:
    Server *server;

	wl_list link;
	wlr_xdg_toplevel *xdg_toplevel;
	wlr_scene_tree *scene_tree;
	Listener<AppWindow> map;
	Listener<AppWindow> unmap;
	Listener<AppWindow> commit;
	Listener<AppWindow> destroy;
	Listener<AppWindow> request_move;
	Listener<AppWindow> request_resize;
	Listener<AppWindow> request_maximize;
	Listener<AppWindow> request_fullscreen;
};

}
