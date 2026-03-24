#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>

// Callback for menu item selection
static void on_menu_item_selected(GtkMenuItem *item, gpointer user_data) {
    const char *label = gtk_menu_item_get_label(item);
    printf("Selected Menu Item: %s\n", label);
}

// Callback to show the menu when the button is clicked
static void on_menu_button_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *menu = GTK_WIDGET(user_data);
    
    // Position and show the menu
    gtk_menu_popup_at_widget(GTK_MENU(menu), 
                             GTK_WIDGET(button), 
                             GDK_GRAVITY_NORTH_WEST, 
                             GDK_GRAVITY_SOUTH_WEST, 
                             NULL);
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
    // Styling
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "button { min-height: 30px; padding: 2px 10px; margin: 2px; }"
        "window { background-color: #222; color: white; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkWindow *window = GTK_WINDOW(gtk_application_window_new(app));

    // Layer Shell Setup
    gtk_layer_init_for_window(window);
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_auto_exclusive_zone_enable(window);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(window), box);

    // --- 1. Create the Menu ---
    GtkWidget *menu = gtk_menu_new();
    const char *items[] = {"System Settings", "Terminal", "File Manager", "Web Browser"};
    
    for (int i = 0; i < 4; i++) {
        GtkWidget *menu_item = gtk_menu_item_new_with_label(items[i]);
        g_signal_connect(menu_item, "activate", G_CALLBACK(on_menu_item_selected), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    }
    gtk_widget_show_all(menu);

    // --- 2. Create the Menu Button ---
    GtkWidget *menu_btn = gtk_button_new_with_label("Menu");
    g_signal_connect(menu_btn, "clicked", G_CALLBACK(on_menu_button_clicked), menu);
    gtk_box_pack_start(GTK_BOX(box), menu_btn, FALSE, FALSE, 5);

    // Existing Konsole Button
    GtkWidget *button = gtk_button_new_with_label("Launch Konsole");
    g_signal_connect(button, "clicked", G_CALLBACK(on_launch_clicked), (gpointer)"gtk3-demo-application");
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 5);

    gtk_widget_show_all(GTK_WIDGET(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.plxwm.panel", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}