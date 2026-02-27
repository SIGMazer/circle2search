#include "circle2search.h"
#include <signal.h>

#ifndef VERSION
#define VERSION "unknown"
#endif

int main(int argc, char *argv[]) {
    // Handle version flag
    if (argc > 1) {
        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
            printf("circle2search version %s\n", VERSION);
            return 0;
        }
    }
    
    // Check if another instance is already running
    FILE *check = popen("pgrep -c -x circle2search", "r");
    if (check) {
        int count = 0;
        fscanf(check, "%d", &count);
        pclose(check);
        
        // If more than 1 instance (current process is already counted)
        if (count > 1) {
            fprintf(stderr, "circle2search is already running\n");
            return 0;
        }
    }
    
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
    if (state.original_screenshot) {
        g_object_unref(state.original_screenshot);
    }
    if (state.detected_text) {
        free(state.detected_text);
    }
    
    return 0;
}
