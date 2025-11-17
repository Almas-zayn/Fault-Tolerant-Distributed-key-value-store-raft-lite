#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "colors.h"

static void print_line(int width)
{
    printf("  ");
    for (int i = 0; i < width; i++)
        putchar('-');
    putchar('\n');
}

void print_header(const char *title, const char *color)
{
    int width = (int)strlen(title) + 13;

    print_line(width);
    printf("  |  %sStatus   |%s%s\n", color, title, RESET);
    print_line(width);
}

void print_header_bg(const char *title, const char *bg_color)
{
    printf("%s  \n%-100s  %s\n", bg_color, title, RESET);
}

void print_success(const char *msg)
{
    printf("  %s[ SUCCESS ] : %s %s\n", GREEN, msg, RESET);
}

void print_error(const char *msg)
{
    printf("  %s[ ERROR ] : %s %s\n", RED, msg, RESET);
}

void print_info(const char *msg)
{
    printf("  %s[ INFO ] : %s %s\n", YELLOW, msg, RESET);
}

void printHeaderName(const char *cacheName, char *color)
{
    printf("\n\n-----------------------------------------------------------------------\n");
    printf("----------->%s%10s %s %10s%s<---------\n", color, " ", cacheName, " ", RESET);
    printf("-----------------------------------------------------------------------\n");
}

// ---------------- Variadic versions ----------------

void vprint_success(const char *fmt, ...)
{
    printf("  %s[ SUCCESS ] :  ", GREEN);
    printf("%s", LIGHT_PURPLE);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("%s\n", RESET);
}

void vprint_error(const char *fmt, ...)
{
    printf("  %s[ ERROR ] : %s ", RED, RESET);
    printf("%s", LIGHT_PINK);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("%s\n", RESET);
}

void vprint_info(const char *fmt, ...)
{
    printf("  %s[ INFO ] : %s ", YELLOW, RESET);
    printf("%s", CYAN);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("%s\n", RESET);
}
