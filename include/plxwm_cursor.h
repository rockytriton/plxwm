#pragma once

#include "common.h"

namespace PlxWM {

class Server;

class Cursor {
public:
    Cursor() {}
    Cursor(Server *server);

    void init();

    wlr_cursor *getCursor() { return cursor; }

    void onMove(uint32_t time);

    static void server_cursor_frame(struct wl_listener *listener, void *data);
    static void server_cursor_motion(struct wl_listener *listener, void *data);
    static void server_cursor_motion_absolute(wl_listener *listener, void *data);
    static void server_cursor_button(struct wl_listener *listener, void *data);
    
private:
    Server *server;

	wlr_cursor *cursor;
	wlr_xcursor_manager *cursor_mgr;
	Listener<Cursor> cursor_motion;
	Listener<Cursor> cursor_motion_absolute;
	Listener<Cursor> cursor_button;
	Listener<Cursor> cursor_axis;
	Listener<Cursor> cursor_frame;
	double grab_x, grab_y;
};

}
