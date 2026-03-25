#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

// Action callback for menu items
static void on_menu_action_old(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    const char *name = g_action_get_name(G_ACTION(action));
    printf("Selected Menu Item Action: %s\n", name);
}

static void on_menu_action(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    GtkWindow *parent_window = GTK_WINDOW(user_data);
    const char *name = g_action_get_name(G_ACTION(action));
    printf("Selected Menu Item Action: %s\n", name);

    // 1. Create the Alert Dialog
    GtkAlertDialog *dialog = gtk_alert_dialog_new("Menu Selection");
    
    // 2. Set the descriptive text (the "body" of the message)
    char *message = g_strdup_printf("You selected the %s option.", name);
    gtk_alert_dialog_set_detail(dialog, message);
    
    // 3. Show the dialog
    // This is non-blocking (async), so the rest of your panel stays responsive
    gtk_alert_dialog_choose(dialog, parent_window, NULL, NULL, NULL);

    // Cleanup the temporary string
    g_free(message);
    g_object_unref(dialog);
}

static void on_launch_clicked(GtkButton *button, gpointer user_data) {
    const char *command = (const char *)user_data;
    GError *error = NULL;
    if (!g_spawn_command_line_async(command, &error)) {
        g_printerr("Error launching %s: %s\n", command, error->message);
        g_error_free(error);
    }
}

static void activate(GtkApplication* app, gpointer user_data) {
    // 1. Create the Window
    GtkWindow *window = GTK_WINDOW(gtk_application_window_new(app));

    // Layer Shell Setup (GTK4 version)
    gtk_layer_init_for_window(window);
    //gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_TOP);
    //gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    //gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    //gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

    gtk_layer_auto_exclusive_zone_enable(window);
    // Required for menus to work in most compositors
    // This allows the panel to take focus when needed (like for menus)
    gtk_layer_set_keyboard_mode(window, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);


    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_window_set_child(window, box);

    // --- 2. Create the Menu Model ---
    GMenu *menu_model = g_menu_new();
    const char *items[] = {"Settings", "Terminal", "Files", "Browser", "A", "B"};
    const char *actions[] = {"settings", "terminal", "files", "browser", "a", "b"};

    // Update your loop to use the "win." prefix in the menu model
    for (int i = 0; i < 6; i++) {
        // This tells the menu to look for the action on the Window object
        char *detailed_action = g_strdup_printf("win.%s", actions[i]);
        g_menu_append(menu_model, items[i], detailed_action);
        g_free(detailed_action);

        GSimpleAction *action = g_simple_action_new(actions[i], NULL);
        g_simple_action_set_enabled(action, TRUE);
        g_signal_connect(action, "activate", G_CALLBACK(on_menu_action), window);
        g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(action));
    }

    // --- 3. Create the Menu Button ---
    // In GTK4, GtkMenuButton handles the popover logic for you automatically
    GtkWidget *menu_btn = gtk_menu_button_new();
    gtk_menu_button_set_label(GTK_MENU_BUTTON(menu_btn), "Menu");
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_btn), G_MENU_MODEL(menu_model));
    gtk_box_append(GTK_BOX(box), menu_btn);

    // Launch Button
    GtkWidget *button = gtk_button_new_with_label("Launch Konsole");
    g_signal_connect(button, "clicked", G_CALLBACK(on_launch_clicked), (gpointer)"konsole");
    gtk_box_append(GTK_BOX(box), button);

    // CSS Styling
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "button { min-height: 30px; padding: 2px 10px; margin: 2px; }"
        "window { background-color: #222; color: white; }");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_window_present(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.plxwm.panel", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}