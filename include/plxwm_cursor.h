#pragma once

#include "common.h"
#include "plxwm_signal.h"

namespace PlxWM {

class Server;

enum CursorMode {
	TINYWL_CURSOR_PASSTHROUGH,
	TINYWL_CURSOR_MOVE,
	TINYWL_CURSOR_RESIZE,
};

class Cursor {
public:
    Cursor(Server *server);

    wlr_cursor *getCursor() { return cursor; }

    void onMove(uint32_t time);
    void onMotion(wl_listener *listener, wlr_pointer_motion_event *event);
    void onMotionAbsolute(wl_listener *listener, wlr_pointer_motion_absolute_event *event);
    void onButton(wl_listener *listener, wlr_pointer_button_event *event);
    void onCursorFrame(wl_listener *listener, void *data);
    void onCursorAxis(wl_listener *listener, wlr_pointer_axis_event *event);

    void setCursorMode(CursorMode mode) { cursor_mode = mode; }

private:
    Server *server;

	wlr_cursor *cursor;
	wlr_xcursor_manager *cursor_mgr;

    unique_ptr<Signal<&Cursor::onMotion>> motion;
    unique_ptr<Signal<&Cursor::onMotionAbsolute>> motionAbsolute;
    unique_ptr<Signal<&Cursor::onButton>> button;
    unique_ptr<Signal<&Cursor::onCursorAxis>> axis;
    unique_ptr<Signal<&Cursor::onCursorFrame>> frame;

    CursorMode cursor_mode;
};

}
