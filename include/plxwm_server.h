#include "common.h"

#include "plxwm_cursor.h"

namespace PlxWM {

class AppWindow;
class Keyboard;
class ServerOutput;

class Server {
public:
    Server();

    void init();
    void onNewOutput(wl_listener *listener, wlr_output *output);
    void onNewInput(wl_listener *listener, wlr_input_device *device);
	void onNewAppWindow(wl_listener *listener, wlr_xdg_toplevel *xdg_toplevel);
	void onRequestCursor(wl_listener *listener, wlr_seat_pointer_request_set_cursor_event *event);

	void onNewPopup(wl_listener *listener, wlr_xdg_popup *event);
	void onSetSelection(struct wl_listener *listener, wlr_seat_request_set_selection_event *data);

    //wl_list *getOutputs() { return &outputs; }
    wlr_scene *getScene() { return scene; }

    void newKeyboard(wlr_input_device *device);

    wlr_seat *getSeat() { return seat; }
    //wl_list *getKeyboards() { return &keyboards; }

    wl_display *getDisplay() { return display; }

    wlr_output_layout *getOutputLayout() { return output_layout; }

    wlr_cursor *getCursor();

	AppWindow *getGrabbedWindow() { return grabbedWindow; }

	AppWindow *getWindowAt(double lx, double ly, double *sx, double *sy);

	void setGrabbedWindow(AppWindow *wnd) { grabbedWindow = wnd; }

	void focus(AppWindow *wnd);

	//wl_list *getAppWindows() { return &appWindows; }


	wlr_box getGrabGeoBox() { return grab_geobox; }
	void setGrabGeoBox(wlr_box g) { grab_geobox = g; }
	void setResizeEdgets(uint32_t e) { resize_edges = e; }

	void processResize();


    double getGrabX() { return grab_x; }
    double getGrabY() { return grab_y; }

    void setGrabX(double x) { grab_x = x; }
    void setGrabY(double y) { grab_y = y; }

    void setCursorMode(CursorMode mode) { cursor->setCursorMode(mode); }

private:
	wl_display *display;
	wlr_backend *backend;
	wlr_renderer *renderer;
	wlr_allocator *allocator;
	wlr_scene *scene;
	wlr_scene_output_layout *scene_layout;

    wlr_output_layout *output_layout;
	//wl_list outputs;

    Cursor *cursor;

	AppWindow *grabbedWindow;
	//wl_list appWindows;

	vector<std::unique_ptr<AppWindow>> windows;

    Listener<Server> new_output;

	wlr_xdg_shell *xdg_shell;
	Listener<Server> new_xdg_toplevel;
	Listener<Server> new_xdg_popup;

	wlr_seat *seat;
	Listener<Server> new_input;
	Listener<Server> request_cursor;
	Listener<Server> request_set_selection;
	//wl_list keyboards;

	vector<Keyboard *> keyboards;
	vector<ServerOutput *> outputs;

	wlr_box grab_geobox;
	uint32_t resize_edges;

	double grab_x, grab_y;
};

};
