#include "circle2search.h"

// Wrapper for g_free to match GdkPixbufDestroyNotify signature
static void pixbuf_free_wrapper(guchar *pixels, gpointer data) {
    (void)data;
    g_free(pixels);
}

// Check if running on Wayland
static gboolean is_wayland() {
    const char *session_type = getenv("XDG_SESSION_TYPE");
    return ((session_type && strcmp(session_type, "wayland") == 0) || getenv("WAYLAND_DISPLAY"));
}

// Capture screen using Wayland (grim)
static gboolean capture_screen_wayland(AppState *state) {
    const char *temp_file = "/tmp/circle2search_wayland_capture.png";
    
    // Use grim to capture the screen
    char command[256];
    snprintf(command, sizeof(command), "grim '%s' 2>/dev/null", temp_file);
    
    if (system(command) != 0) {
        fprintf(stderr, "Failed to capture screen with grim. Is it installed?\n");
        return FALSE;
    }
    
    // Load the captured image
    GError *error = NULL;
    GdkPixbuf *original = gdk_pixbuf_new_from_file(temp_file, &error);
    
    if (!original || error) {
        fprintf(stderr, "Failed to load captured screen: %s\n", error ? error->message : "unknown error");
        if (error) g_error_free(error);
        return FALSE;
    }
    
    // Get dimensions
    int width = gdk_pixbuf_get_width(original);
    int height = gdk_pixbuf_get_height(original);
    
    // Create dimmed version (50% darker)
    guchar *pixels = g_malloc(width * height * 4);
    guchar *original_pixels = g_malloc(width * height * 4);
    
    const guchar *src = gdk_pixbuf_get_pixels(original);
    int rowstride = gdk_pixbuf_get_rowstride(original);
    int n_channels = gdk_pixbuf_get_n_channels(original);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int src_offset = y * rowstride + x * n_channels;
            int dst_offset = (y * width + x) * 4;
            
            guchar r = src[src_offset + 0];
            guchar g = src[src_offset + 1];
            guchar b = src[src_offset + 2];
            
            // Store original
            original_pixels[dst_offset + 0] = r;
            original_pixels[dst_offset + 1] = g;
            original_pixels[dst_offset + 2] = b;
            original_pixels[dst_offset + 3] = 0xFF;
            
            // Store dimmed version (50% darker)
            pixels[dst_offset + 0] = r / 2;
            pixels[dst_offset + 1] = g / 2;
            pixels[dst_offset + 2] = b / 2;
            pixels[dst_offset + 3] = 0xFF;
        }
    }
    
    g_object_unref(original);
    remove(temp_file);
    
    state->screenshot = gdk_pixbuf_new_from_data(
        pixels,
        GDK_COLORSPACE_RGB,
        TRUE,
        8,
        width,
        height,
        width * 4,
        pixbuf_free_wrapper,
        NULL
    );
    
    state->original_screenshot = gdk_pixbuf_new_from_data(
        original_pixels,
        GDK_COLORSPACE_RGB,
        TRUE,
        8,
        width,
        height,
        width * 4,
        pixbuf_free_wrapper,
        NULL
    );
    
    return TRUE;
}

// Capture the entire screen using X11
static gboolean capture_screen_x11(AppState *state) {
    Display *display;
    Window root;
    XImage *image;
    int screen;
    int width, height;
    
    // Open display
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Failed to open X display\n");
        return FALSE;
    }
    
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);
    
    // Get screen dimensions
    width = DisplayWidth(display, screen);
    height = DisplayHeight(display, screen);
    
    // Capture screen
    image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
    if (!image) {
        fprintf(stderr, "Failed to capture screen\n");
        XCloseDisplay(display);
        return FALSE;
    }
    
    // Convert XImage to GdkPixbuf
    guchar *pixels = g_malloc(width * height * 4);
    guchar *original_pixels = g_malloc(width * height * 4);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);
            int offset = (y * width + x) * 4;
            
            // Extract RGB
            guchar r = (pixel >> 16) & 0xFF;
            guchar g = (pixel >> 8) & 0xFF;
            guchar b = pixel & 0xFF;
            
            // Store original
            original_pixels[offset + 0] = r;
            original_pixels[offset + 1] = g;
            original_pixels[offset + 2] = b;
            original_pixels[offset + 3] = 0xFF;
            
            // Store dimmed version (50% darker)
            pixels[offset + 0] = r / 2;
            pixels[offset + 1] = g / 2;
            pixels[offset + 2] = b / 2;
            pixels[offset + 3] = 0xFF;
        }
    }
    
    state->screenshot = gdk_pixbuf_new_from_data(
        pixels,
        GDK_COLORSPACE_RGB,
        TRUE,  // has_alpha
        8,     // bits_per_sample
        width,
        height,
        width * 4,  // rowstride
        pixbuf_free_wrapper,
        NULL
    );
    
    state->original_screenshot = gdk_pixbuf_new_from_data(
        original_pixels,
        GDK_COLORSPACE_RGB,
        TRUE,
        8,
        width,
        height,
        width * 4,
        pixbuf_free_wrapper,
        NULL
    );
    
    // Clean up
    XDestroyImage(image);
    XCloseDisplay(display);
    
    return TRUE;
}

// Capture screen - auto-detects Wayland or X11
gboolean capture_screen(AppState *state) {
    if (is_wayland()) {
        printf("Wayland session detected, using grim for screen capture\n");
        return capture_screen_wayland(state);
    } else {
        printf("X11 session detected, using X11 for screen capture\n");
        return capture_screen_x11(state);
    }
}
