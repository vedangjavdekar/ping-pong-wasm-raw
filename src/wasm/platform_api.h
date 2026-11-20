#pragma once

#define TRUE 1
#define FALSE 0

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef int i32;
typedef int b32;
typedef float f32;

#define PLATFORM_FN extern

// Logs
PLATFORM_FN void platform_log(const char *message);

// Draw Line
PLATFORM_FN void platform_stroke_line_t(i32 x1, i32 y1, i32 x2, i32 y2, i32 thickness, u32 color);
PLATFORM_FN void platform_stroke_line(i32 x1, i32 y1, i32 x2, i32 y2, u32 color);

// Draw Rectangle
PLATFORM_FN void platform_fill_rect(i32 x, i32 y, i32 w, i32 h, u32 color);
PLATFORM_FN void platform_stroke_rect(i32 x, i32 y, i32 w, i32 h, u32 color);

// Draw Circle
PLATFORM_FN void platform_fill_circle(i32 x, i32 y, i32 r, u32 color);
PLATFORM_FN void platform_stroke_circle(i32 x, i32 y, i32 r, u32 color);

// Draw Text
PLATFORM_FN void platform_fill_text(i32 x, i32 y, const char *text, u32 size, u32 color);
PLATFORM_FN void platform_log(const char *message);
