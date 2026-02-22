#include "circle2search.h"

// Extract text from the selected region using Tesseract OCR
char* extract_text_from_region(AppState *state) {
    if (!state->screenshot || !state->selection_done) {
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
    
    // Create a sub-pixbuf of the selected region
    GdkPixbuf *region = gdk_pixbuf_new_subpixbuf(state->screenshot, x, y, width, height);
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
    
    // Initialize Tesseract
    TessBaseAPI *api = TessBaseAPICreate();
    if (TessBaseAPIInit3(api, NULL, "eng") != 0) {
        fprintf(stderr, "Failed to initialize Tesseract\n");
        TessBaseAPIDelete(api);
        return NULL;
    }
    
    // Set Tesseract to prioritize accuracy
    TessBaseAPISetPageSegMode(api, PSM_AUTO);
    TessBaseAPISetVariable(api, "tessedit_char_whitelist", 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,!?@#$%&*()-+=/:;<>[]{}'\"");
    
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
    
    int letter_count = 0;
    int total_count = 0;
    int word_count = 0;
    gboolean in_word = FALSE;
    
    for (const char *p = text; *p; p++) {
        total_count++;
        if (isalpha(*p)) {
            letter_count++;
            if (!in_word) {
                word_count++;
                in_word = TRUE;
            }
        } else if (isspace(*p)) {
            in_word = FALSE;
        }
    }
    
    // Text is meaningful if:
    // - Has at least 2 words OR
    // - Has at least 3 characters and >50% are letters
    if (word_count >= 2) {
        return TRUE;
    }
    
    if (total_count >= 3 && letter_count * 2 > total_count) {
        return TRUE;
    }
    
    return FALSE;
}
