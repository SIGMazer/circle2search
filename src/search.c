#include "circle2search.h"

// URL encode a string
char* url_encode(const char *str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char *encoded = malloc(len * 3 + 1);
    if (!encoded) return NULL;
    
    char *pout = encoded;
    for (const char *pin = str; *pin; pin++) {
        unsigned char c = *pin;
        
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            *pout++ = c;
        } else if (c == ' ') {
            *pout++ = '+';
        } else {
            sprintf(pout, "%%%02X", c);
            pout += 3;
        }
    }
    *pout = '\0';
    
    return encoded;
}

// Start local HTTP server
int start_local_server() {
    system("pkill -f 'python3 -m http.server 8899' 2>/dev/null");
    usleep(200000);
    
    system("cd /tmp && python3 -m http.server 8899 >/dev/null 2>&1 &");
    usleep(500000);
    
    return 1;
}

// Create SSH tunnel
char* create_tunnel() {
    const char *tunnel_output = "/tmp/circle2search_tunnel.txt";
    unlink(tunnel_output);
    
    printf("Creating tunnel...\n");
    
    system("ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 "
           "-o ServerAliveInterval=20 "
           "-R 80:localhost:8899 serveo.net 2>&1 | "
           "grep -m1 'https://' > /tmp/circle2search_tunnel.txt &");
    
    for (int i = 0; i < 20; i++) {
        usleep(500000);
        
        FILE *f = fopen(tunnel_output, "r");
        if (f) {
            char url[512] = {0};
            if (fgets(url, sizeof(url), f) != NULL) {
                fclose(f);
                
                char *https_start = strstr(url, "https://");
                if (https_start) {
                    char *url_end = https_start;
                    while (*url_end && *url_end != ' ' && *url_end != '\n' && *url_end != '\r') {
                        url_end++;
                    }
                    *url_end = '\0';
                    
                    if (strlen(https_start) > 15) {
                        char *full_url = malloc(strlen(https_start) + 50);
                        sprintf(full_url, "%s/circle2search_image.png", https_start);
                        printf("Tunnel ready: %s\n", https_start);
                        return full_url;
                    }
                }
            }
            fclose(f);
        }
    }
    
    fprintf(stderr, "Tunnel timeout\n");
    return NULL;
}

// Cleanup function to be called before exit
void cleanup_servers() {
    printf("Cleaning up servers...\n");
    system("pkill -f 'python3 -m http.server 8899' 2>/dev/null");
    system("pkill -f 'ssh.*serveo.net' 2>/dev/null");
    usleep(200000);
}

// Search with self-hosted image
void search_image_selfhosted(const char *image_file) {
    (void)image_file;
    printf("Self-hosting image...\n");
    
    if (!start_local_server()) {
        fprintf(stderr, "Failed to start server\n");
        cleanup_servers();
        return;
    }
    
    char *public_url = create_tunnel();
    if (!public_url) {
        fprintf(stderr, "Failed to create tunnel\n");
        cleanup_servers();
        return;
    }
    
    char *encoded_url = url_encode(public_url);
    if (encoded_url) {
        char lens_url[2048];
        snprintf(lens_url, sizeof(lens_url), 
                "https://lens.google.com/uploadbyurl?url=%s", encoded_url);
        
        char command[2560];
        snprintf(command, sizeof(command), "xdg-open '%s' >/dev/null 2>&1", lens_url);
        
        system(command);
        printf("Opening Google Lens...\n");
        
        // Give browser a moment to start the request
        sleep(2);
        
        printf("Cleaning up servers...\n");
        cleanup_servers();
        printf("Done!\n");
        
        free(encoded_url);
    }
    
    free(public_url);
}

// Open browser with search
void open_browser_with_search(const char *query, SelectionMode mode) {
    if (mode == MODE_TEXT && query && strlen(query) > 0) {
        char *encoded_query = url_encode(query);
        if (!encoded_query) return;
        
        char url[2048];
        snprintf(url, sizeof(url), "https://www.google.com/search?q=%s", encoded_query);
        
        char command[2560];
        snprintf(command, sizeof(command), "xdg-open '%s' 2>/dev/null &", url);
        system(command);
        
        free(encoded_query);
    } else if (mode == MODE_IMAGE) {
        search_image_selfhosted("/tmp/circle2search_image.png");
    }
}

// Search button callback
void on_search_clicked(GtkWidget *widget, AppState *state) {
    (void)widget;
    
    if (!state->selection_done) return;
    
    if (state->mode == MODE_TEXT && state->detected_text) {
        printf("Searching for text: %s\n", state->detected_text);
        open_browser_with_search(state->detected_text, MODE_TEXT);
        gtk_main_quit();
    } else if (state->mode == MODE_IMAGE) {
        printf("Searching for image\n");
        
        int x = MIN(state->start_x, state->end_x);
        int y = MIN(state->start_y, state->end_y);
        int width = abs(state->end_x - state->start_x);
        int height = abs(state->end_y - state->start_y);
        
        if (width > 0 && height > 0) {
            GdkPixbuf *region = gdk_pixbuf_new_subpixbuf(state->screenshot, x, y, width, height);
            if (region) {
                const char *temp_file = "/tmp/circle2search_image.png";
                GError *error = NULL;
                gdk_pixbuf_save(region, temp_file, "png", &error, NULL);
                g_object_unref(region);
                
                if (!error) {
                    open_browser_with_search(NULL, MODE_IMAGE);
                    gtk_main_quit();
                } else {
                    fprintf(stderr, "Failed to save image: %s\n", error->message);
                    g_error_free(error);
                }
            }
        }
    }
}

void on_translate_clicked(GtkWidget *widget, AppState *state) {
    (void)widget;
    
    if (!state->detected_text) return;
    
    char *encoded_text = url_encode(state->detected_text);
    if (encoded_text) {
        char url[8192];
        int url_len = snprintf(url, sizeof(url), "https://translate.google.com/?sl=auto&tl=auto&text=%s", encoded_text);
        
        if (url_len > 0 && url_len < (int)sizeof(url)) {
            char *command = malloc(url_len + 20);
            if (command) {
                sprintf(command, "xdg-open '%s' &", url);
                system(command);
                free(command);
            }
        }
        
        free(encoded_text);
        gtk_main_quit();
    }
}