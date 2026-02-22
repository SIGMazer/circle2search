#ifndef CIRCLE2SEARCH_H
#define CIRCLE2SEARCH_H

#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <tesseract/capi.h>
#include <leptonica/allheaders.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

// Selection mode
typedef enum {
    MODE_TEXT,
    MODE_IMAGE
} SelectionMode;

// Application state
typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GdkPixbuf *screenshot;
    cairo_surface_t *surface;
    
    // Selection coordinates
    int start_x;
    int start_y;
    int end_x;
    int end_y;
    gboolean selecting;
    gboolean selection_done;
    gboolean force_image_mode;  // True when Ctrl is held
    
    // Mode (auto-detected)
    SelectionMode mode;
    
    // Detected text preview
    char *detected_text;
    
    // UI elements
    GtkWidget *search_button;
} AppState;

// Function declarations
// screen_capture.c
gboolean capture_screen(AppState *state);

// overlay.c
void create_overlay_window(AppState *state);
gboolean on_draw(GtkWidget *widget, cairo_t *cr, AppState *state);
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, AppState *state);
gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, AppState *state);
gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, AppState *state);
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, AppState *state);

// ocr.c
char* extract_text_from_region(AppState *state);

// search.c
void open_browser_with_search(const char *query, SelectionMode mode);
void on_search_clicked(GtkWidget *widget, AppState *state);
gboolean is_text_meaningful(const char *text);

#endif
