#ifndef COLOR_H
#define COLOR_H

#define RESET "\033[0m"
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[1;36m"
#define BOLD "\033[1m"
#define ORANGE "\033[38;5;208m"

// Light / Pastel Colors (256-color palette)
#define LIGHT_RED "\033[38;5;210m"
#define LIGHT_GREEN "\033[38;5;157m"
#define LIGHT_YELLOW "\033[38;5;229m"
#define LIGHT_BLUE "\033[38;5;153m"
#define LIGHT_CYAN "\033[38;5;159m"
#define LIGHT_MAGENTA "\033[38;5;218m"
#define LIGHT_ORANGE "\033[38;5;216m"
#define LIGHT_PINK "\033[38;5;219m"
#define LIGHT_PURPLE "\033[38;5;182m"
#define LIGHT_TEAL "\033[38;5;80m"
#define LIGHT_GRAY "\033[38;5;250m"
#define LIGHT_WHITE "\033[38;5;255m"

#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN "\033[46m"
#define BG_WHITE "\033[47m"

void print_header(const char *title, const char *color);
void print_header_bg(const char *title, const char *bg_color);

void printHeaderName(const char *cacheName, char *color);

void print_success(const char *msg);
void print_error(const char *msg);
void print_info(const char *msg);

void vprint_success(const char *fmt, ...);
void vprint_error(const char *fmt, ...);
void vprint_info(const char *fmt, ...);

#endif
