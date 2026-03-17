#include "common.h"

namespace PlxWM {
    
class Cursor;

class Server {
public:
    Server();

    void init();
    void onNewOutput(wl_listener *listener, wlr_output *output);
    void onNewInput(wl_listener *listener, wlr_input_device *device);

    static void server_new_output(wl_listener *listener, void *data);
    static void server_new_input(struct wl_listener *listener, void *data);

    wl_list *getOutputs() { return &outputs; }
    wlr_scene *getScene() { return scene; }

    void newKeyboard(wlr_input_device *device);

    wlr_seat *getSeat() { return seat; }
    wl_list *getKeyboards() { return &keyboards; }

    wl_display *getDisplay() { return display; }

    wlr_output_layout *getOutputLayout() { return output_layout; }

    Cursor *getCursor() { return cursor; }

    tinywl_toplevel *getTopLevel() { return grabbed_toplevel; }

private:
	wl_display *display;
	wlr_backend *backend;
	wlr_renderer *renderer;
	wlr_allocator *allocator;
	wlr_scene *scene;
	wlr_scene_output_layout *scene_layout;

    wlr_output_layout *output_layout;
	wl_list outputs;

    Cursor *cursor;

    Listener<Server> new_output;

	wlr_xdg_shell *xdg_shell;
	Listener<Server> new_xdg_toplevel;
	Listener<Server> new_xdg_popup;
	wl_list toplevels;

	wlr_seat *seat;
	Listener<Server> new_input;
	Listener<Server> request_cursor;
	Listener<Server> request_set_selection;
	wl_list keyboards;
	tinywl_toplevel *grabbed_toplevel;
	wlr_box grab_geobox;
	uint32_t resize_edges;

};

};
