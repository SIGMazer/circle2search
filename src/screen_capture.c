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
