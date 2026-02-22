#include "circle2search.h"

// Extract text from the selected region using Tesseract OCR
char* extract_text_from_region(AppState *state) {
    if (!state->original_screenshot || !state->selection_done) {
        return NULL;
    }
    
    // Get selection bounds
    int x = MIN(state->start_x, state->end_x);
    int y = MIN(state->start_y, state->end_y);
    int width = abs(state->end_x - state->start_x);
    int height = abs(state->end_y - state->start_y);
    
    if (width <= 0 || height <= 0) {
        return NULL;
    }
    
    // Create a sub-pixbuf of the selected region from the ORIGINAL screenshot
    GdkPixbuf *region = gdk_pixbuf_new_subpixbuf(state->original_screenshot, x, y, width, height);
    if (!region) {
        fprintf(stderr, "Failed to create region pixbuf\n");
        return NULL;
    }
    
    // Save temporary image for Tesseract
    const char *temp_file = "/tmp/circle2search_region.png";
    GError *error = NULL;
    gdk_pixbuf_save(region, temp_file, "png", &error, NULL);
    g_object_unref(region);
    
    if (error) {
        fprintf(stderr, "Failed to save region: %s\n", error->message);
        g_error_free(error);
        return NULL;
    }
    
    // Initialize Tesseract with multiple languages (Arabic + English)
    TessBaseAPI *api = TessBaseAPICreate();
    if (TessBaseAPIInit3(api, NULL, "ara+eng") != 0) {
        // Fallback to English only if Arabic not available
        if (TessBaseAPIInit3(api, NULL, "eng") != 0) {
            fprintf(stderr, "Failed to initialize Tesseract\n");
            TessBaseAPIDelete(api);
            return NULL;
        }
    }
    
    // Set Tesseract to auto mode for best multilingual support
    TessBaseAPISetPageSegMode(api, PSM_AUTO_OSD);
    
    // Load image with Leptonica
    PIX *image = pixRead(temp_file);
    if (!image) {
        fprintf(stderr, "Failed to read image with Leptonica\n");
        TessBaseAPIEnd(api);
        TessBaseAPIDelete(api);
        return NULL;
    }
    
    // Perform OCR
    TessBaseAPISetImage2(api, image);
    char *text = TessBaseAPIGetUTF8Text(api);
    
    // Clean up
    pixDestroy(&image);
    TessBaseAPIEnd(api);
    TessBaseAPIDelete(api);
    remove(temp_file);
    
    // Trim whitespace and return
    if (text) {
        // Remove trailing whitespace
        char *end = text + strlen(text) - 1;
        while (end > text && (*end == '\n' || *end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }
        
        // Remove leading whitespace
        char *start = text;
        while (*start == ' ' || *start == '\t' || *start == '\n') {
            start++;
        }
        
        if (strlen(start) == 0) {
            TessDeleteText(text);
            return NULL;
        }
        
        char *result = strdup(start);
        TessDeleteText(text);
        return result;
    }
    
    return NULL;
}

// Check if extracted text is meaningful (not garbage)
gboolean is_text_meaningful(const char *text) {
    if (!text || strlen(text) < 2) {
        return FALSE;
    }
    
    // Use GLib's UTF-8 functions for proper Unicode support
    int char_count = 0;
    int total_chars = 0;
    int word_count = 0;
    gboolean in_word = FALSE;
    
    const char *p = text;
    while (*p) {
        gunichar c = g_utf8_get_char(p);
        total_chars++;
        
        // Check if character is a letter (supports all Unicode scripts including Arabic)
        if (g_unichar_isalpha(c)) {
            char_count++;
            if (!in_word) {
                word_count++;
                in_word = TRUE;
            }
        } else if (g_unichar_isspace(c)) {
            in_word = FALSE;
        } else if (g_unichar_ispunct(c)) {
            // Punctuation doesn't end word context
        } else {
            in_word = FALSE;
        }
        
        p = g_utf8_next_char(p);
    }
    
    // Text is meaningful if:
    // - Has at least 1 word with 2+ characters OR
    // - Has at least 3 characters and >40% are letters
    if (word_count >= 1 && char_count >= 2) {
        return TRUE;
    }
    
    if (total_chars >= 3 && char_count * 5 > total_chars * 2) {
        return TRUE;
    }
    
    return FALSE;
}
