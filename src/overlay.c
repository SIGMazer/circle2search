#include "circle2search.h"

// Draw callback - renders the screenshot and selection rectangle
gboolean on_draw(GtkWidget *widget, cairo_t *cr, AppState *state) {
    // Draw the screenshot
    if (state->screenshot) {
        gdk_cairo_set_source_pixbuf(cr, state->screenshot, 0, 0);
        cairo_paint(cr);
    }
    
    // Draw selection rectangle if selecting
    if (state->selecting || state->selection_done) {
        int x = MIN(state->start_x, state->end_x);
        int y = MIN(state->start_y, state->end_y);
        int w = abs(state->end_x - state->start_x);
        int h = abs(state->end_y - state->start_y);
        
        // Semi-transparent overlay
        cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
        cairo_rectangle(cr, 0, 0, gtk_widget_get_allocated_width(widget), y);
        cairo_fill(cr);
        cairo_rectangle(cr, 0, y, x, h);
        cairo_fill(cr);
        cairo_rectangle(cr, x + w, y, gtk_widget_get_allocated_width(widget) - x - w, h);
        cairo_fill(cr);
        cairo_rectangle(cr, 0, y + h, gtk_widget_get_allocated_width(widget), 
                       gtk_widget_get_allocated_height(widget) - y - h);
        cairo_fill(cr);
        
        // Selection border - color based on detected mode
        if (state->selection_done) {
            if (state->mode == MODE_TEXT) {
                cairo_set_source_rgb(cr, 0.2, 0.6, 1.0); // Blue for text
            } else {
                cairo_set_source_rgb(cr, 1.0, 0.4, 0.2); // Orange for image
            }
        } else {
            cairo_set_source_rgb(cr, 0.2, 0.6, 1.0); // Blue while selecting
        }
        cairo_set_line_width(cr, 3.0);
        cairo_rectangle(cr, x, y, w, h);
        cairo_stroke(cr);
        
        // Draw detected text preview (Android style)
        if (state->selection_done && state->detected_text && state->mode == MODE_TEXT) {
            // Create semi-transparent background for text
            int text_y = (y > 60) ? y - 50 : y + h + 10;
            
            // Measure text
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 16);
            cairo_text_extents_t extents;
            
            // Truncate long text
            char preview[100];
            strncpy(preview, state->detected_text, 95);
            preview[95] = '\0';
            if (strlen(state->detected_text) > 95) {
                strcat(preview, "...");
            }
            
            // Replace newlines with spaces
            for (char *p = preview; *p; p++) {
                if (*p == '\n' || *p == '\r') *p = ' ';
            }
            
            cairo_text_extents(cr, preview, &extents);
            
            int text_width = extents.width + 20;
            int text_height = extents.height + 16;
            int text_x = x + (w - text_width) / 2;
            
            // Draw background
            cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.9);
            cairo_rectangle(cr, text_x, text_y, text_width, text_height);
            cairo_fill(cr);
            
            // Draw text
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, text_x + 10, text_y + text_height - 8);
            cairo_show_text(cr, preview);
        } else if (state->selection_done && state->mode == MODE_IMAGE) {
            // Show "Image" indicator
            int label_y = (y > 40) ? y - 30 : y + h + 10;
            
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 14);
            
            const char *label = "📷 Image";
            cairo_text_extents_t extents;
            cairo_text_extents(cr, label, &extents);
            
            int label_width = extents.width + 20;
            int label_height = extents.height + 12;
            int label_x = x + (w - label_width) / 2;
            
            // Draw background
            cairo_set_source_rgba(cr, 1.0, 0.4, 0.2, 0.9);
            cairo_rectangle(cr, label_x, label_y, label_width, label_height);
            cairo_fill(cr);
            
            // Draw text
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, label_x + 10, label_y + label_height - 6);
            cairo_show_text(cr, label);
        }
    }
    
    return FALSE;
}

// Mouse button press - start selection
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, AppState *state) {
    if (event->button == 1) { // Left click
        state->start_x = event->x;
        state->start_y = event->y;
        state->end_x = event->x;
        state->end_y = event->y;
        state->selecting = TRUE;
        state->selection_done = FALSE;
        state->force_image_mode = (event->state & GDK_CONTROL_MASK) != 0;
        
        // Clear previous detected text
        if (state->detected_text) {
            free(state->detected_text);
            state->detected_text = NULL;
        }
        
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

// Mouse button release - end selection
gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, AppState *state) {
    if (event->button == 1 && state->selecting) {
        state->end_x = event->x;
        state->end_y = event->y;
        state->selecting = FALSE;
        state->selection_done = TRUE;
        
        // If not forcing image mode, try to detect text
        if (!state->force_image_mode) {
            char *text = extract_text_from_region(state);
            if (text && is_text_meaningful(text)) {
                state->detected_text = text;
                state->mode = MODE_TEXT;
            } else {
                if (text) free(text);
                state->mode = MODE_IMAGE;
            }
        } else {
            state->mode = MODE_IMAGE;
        }
        
        gtk_widget_queue_draw(widget);
        
        // Show search button
        gtk_widget_set_visible(state->search_button, TRUE);
    }
    return TRUE;
}

// Mouse motion - update selection
gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, AppState *state) {
    if (state->selecting) {
        state->end_x = event->x;
        state->end_y = event->y;
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

// Key press - handle ESC
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, AppState *state) {
    (void)widget;
    (void)state;
    
    if (event->keyval == GDK_KEY_Escape) {
        gtk_main_quit();
        return TRUE;
    }
    return FALSE;
}

// Create the overlay window
void create_overlay_window(AppState *state) {
    // Create fullscreen window
    state->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_fullscreen(GTK_WINDOW(state->window));
    gtk_window_set_decorated(GTK_WINDOW(state->window), FALSE);
    
    // Create main container with overlay
    GtkWidget *overlay_container = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(state->window), overlay_container);
    
    // Drawing area for screenshot
    state->drawing_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(overlay_container), state->drawing_area);
    
    // Search button as overlay (initially hidden)
    state->search_button = gtk_button_new_with_label("🔍 Search");
    gtk_widget_set_halign(state->search_button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(state->search_button, GTK_ALIGN_END);
    gtk_widget_set_margin_bottom(state->search_button, 50);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay_container), state->search_button);
    gtk_widget_set_visible(state->search_button, FALSE);
    
    // Enable mouse events
    gtk_widget_add_events(state->drawing_area,
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | 
        GDK_POINTER_MOTION_MASK);
    
    // Connect signals
    g_signal_connect(state->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(state->window, "key-press-event", G_CALLBACK(on_key_press), state);
    g_signal_connect(state->drawing_area, "draw", G_CALLBACK(on_draw), state);
    g_signal_connect(state->drawing_area, "button-press-event", 
                     G_CALLBACK(on_button_press), state);
    g_signal_connect(state->drawing_area, "button-release-event", 
                     G_CALLBACK(on_button_release), state);
    g_signal_connect(state->drawing_area, "motion-notify-event", 
                     G_CALLBACK(on_motion_notify), state);
    g_signal_connect(state->search_button, "clicked", G_CALLBACK(on_search_clicked), state);
    
    // Show window
    gtk_widget_show_all(state->window);
    gtk_widget_set_visible(state->search_button, FALSE);
}
