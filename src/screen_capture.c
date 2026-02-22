#include "circle2search.h"

// Wrapper for g_free to match GdkPixbufDestroyNotify signature
static void pixbuf_free_wrapper(guchar *pixels, gpointer data) {
    (void)data;
    g_free(pixels);
}

// Capture the entire screen using X11
gboolean capture_screen(AppState *state) {
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
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);
            int offset = (y * width + x) * 4;
            
            // Extract RGB (assuming 24-bit or 32-bit display)
            pixels[offset + 0] = (pixel >> 16) & 0xFF; // R
            pixels[offset + 1] = (pixel >> 8) & 0xFF;  // G
            pixels[offset + 2] = pixel & 0xFF;         // B
            pixels[offset + 3] = 0xFF;                 // A (fully opaque)
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
    
    // Clean up
    XDestroyImage(image);
    XCloseDisplay(display);
    
    return TRUE;
}
