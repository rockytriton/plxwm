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
    void onMotion(wl_listener *listener, wlr_pointer_motion_event *event);
    void onMotionAbsolute(wl_listener *listener, wlr_pointer_motion_absolute_event *event);
    void onButton(wl_listener *listener, wlr_pointer_button_event *event);
    void onCursorFrame(wl_listener *listener, void *data);
    void onCursorAxis(wl_listener *listener, wlr_pointer_axis_event *event);

private:
    Server *server;

	wlr_cursor *cursor;
	wlr_xcursor_manager *cursor_mgr;
	Listener<Cursor> cursor_motion;
	Listener<Cursor> cursor_motion_absolute;
	Listener<Cursor> cursor_button;
	Listener<Cursor> cursor_axis;
	Listener<Cursor> cursor_frame;
};

}
