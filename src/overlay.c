#include "circle2search.h"

// Draw callback - renders the screenshot and selection rectangle
gboolean on_draw(GtkWidget *widget, cairo_t *cr, AppState *state) {
    (void)widget;
    
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
        
        // Draw the selected area with original brightness
        if (state->original_screenshot) {
            cairo_save(cr);
            cairo_rectangle(cr, x, y, w, h);
            cairo_clip(cr);
            gdk_cairo_set_source_pixbuf(cr, state->original_screenshot, 0, 0);
            cairo_paint(cr);
            cairo_restore(cr);
        }
        
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
            int text_y = (y > 60) ? y - 50 : y + h + 10;
            
            char preview[600];
            g_utf8_strncpy(preview, state->detected_text, 190);
            if (g_utf8_strlen(state->detected_text, -1) > 190) {
                strcat(preview, "...");
            }
            
            for (char *p = preview; *p; p++) {
                if (*p == '\n' || *p == '\r') *p = ' ';
            }
            
            PangoLayout *layout = pango_cairo_create_layout(cr);
            PangoFontDescription *font_desc = pango_font_description_from_string("Sans Bold 16");
            pango_layout_set_font_description(layout, font_desc);
            pango_layout_set_text(layout, preview, -1);
            
            int text_width, text_height;
            pango_layout_get_pixel_size(layout, &text_width, &text_height);
            
            text_width += 20;
            text_height += 16;
            int text_x = x + (w - text_width) / 2;
            
            cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.9);
            cairo_rectangle(cr, text_x, text_y, text_width, text_height);
            cairo_fill(cr);
            
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, text_x + 10, text_y + 8);
            pango_cairo_show_layout(cr, layout);
            
            pango_font_description_free(font_desc);
            g_object_unref(layout);
        } else if (state->selection_done && state->mode == MODE_IMAGE) {
            int label_y = (y > 40) ? y - 30 : y + h + 10;
            
            const char *label = "📷 Image";
            
            PangoLayout *layout = pango_cairo_create_layout(cr);
            PangoFontDescription *font_desc = pango_font_description_from_string("Sans Bold 14");
            pango_layout_set_font_description(layout, font_desc);
            pango_layout_set_text(layout, label, -1);
            
            int label_width, label_height;
            pango_layout_get_pixel_size(layout, &label_width, &label_height);
            
            label_width += 20;
            label_height += 12;
            int label_x = x + (w - label_width) / 2;
            
            cairo_set_source_rgba(cr, 1.0, 0.4, 0.2, 0.9);
            cairo_rectangle(cr, label_x, label_y, label_width, label_height);
            cairo_fill(cr);
            
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, label_x + 10, label_y + 6);
            pango_cairo_show_layout(cr, layout);
            
            pango_font_description_free(font_desc);
            g_object_unref(layout);
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
        
        gtk_widget_set_visible(state->search_button, TRUE);
        if (state->mode == MODE_TEXT) {
            gtk_widget_set_visible(state->translate_button, TRUE);
        } else {
            gtk_widget_set_visible(state->translate_button, FALSE);
        }
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
    
    if (event->keyval == GDK_KEY_Escape) {
        gtk_main_quit();
        return TRUE;
    }
    
    // Ctrl+C to copy detected text
    if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_c || event->keyval == GDK_KEY_C)) {
        if (state->detected_text && state->mode == MODE_TEXT) {
            // Use xclip to copy to clipboard
            FILE *pipe = popen("xclip -selection clipboard", "w");
            if (pipe) {
                fprintf(pipe, "%s", state->detected_text);
                pclose(pipe);
                printf("Text copied to clipboard: %s\n", state->detected_text);
            } else {
                printf("Failed to copy: xclip not available\n");
            }
            return TRUE;
        } else {
            printf("Cannot copy: detected_text=%p, mode=%d\n", 
                   (void*)state->detected_text, state->mode);
        }
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
    
    state->drawing_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(overlay_container), state->drawing_area);
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(button_box, GTK_ALIGN_END);
    gtk_widget_set_margin_bottom(button_box, 50);
    
    state->translate_button = gtk_button_new_with_label("🌐 Translate");
    state->search_button = gtk_button_new_with_label("🔍 Search");
    
    gtk_box_pack_start(GTK_BOX(button_box), state->translate_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), state->search_button, FALSE, FALSE, 0);
    
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay_container), button_box);
    gtk_widget_set_visible(state->search_button, FALSE);
    gtk_widget_set_visible(state->translate_button, FALSE);
    
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
    g_signal_connect(state->translate_button, "clicked", G_CALLBACK(on_translate_clicked), state);
    
    gtk_widget_show_all(state->window);
    gtk_widget_set_visible(state->search_button, FALSE);
    gtk_widget_set_visible(state->translate_button, FALSE);
    
    gtk_widget_queue_draw(state->drawing_area);
}
