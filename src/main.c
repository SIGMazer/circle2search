#include "circle2search.h"

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Create application state
    AppState state = {0};
    state.mode = MODE_TEXT;
    state.selecting = FALSE;
    state.selection_done = FALSE;
    state.force_image_mode = FALSE;
    state.detected_text = NULL;
    
    // Capture screen
    printf("Capturing screen...\n");
    if (!capture_screen(&state)) {
        fprintf(stderr, "Failed to capture screen\n");
        return 1;
    }
    printf("Screen captured successfully\n");
    
    // Create overlay window
    create_overlay_window(&state);
    
    // Run GTK main loop
    gtk_main();
    
    // Cleanup
    if (state.screenshot) {
        g_object_unref(state.screenshot);
    }
    if (state.detected_text) {
        free(state.detected_text);
    }
    
    return 0;
}
