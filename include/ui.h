#ifndef FIDDLE_UI_H
#define FIDDLE_UI_H

#include <stdbool.h>
#include "common.h"

// TODO - implement...
typedef struct str {} String8;

// ----------------------------------------------------------------------------
// UI Data Structures
// ----------------------------------------------------------------------------

typedef struct UI_Key UI_Key;
struct UI_Key {
    u64 u64[1];
};

enum UI_SizeKind {
    UI_SizeKind_Null,                 // widget size not needed
    UI_SizeKind_Pixels,               // widget size directly specified in pixels
    UI_SizeKind_TextContent,          // widget size must fit associated string
    UI_SizeKind_PercentOfParent,      // widget size is a percent of the parent widget's size (on the same axis)
    UI_SizeKind_ChildrenSum,          // widget size is sum of child widget sizes laid out in order (on a given axis)
};

struct UI_Size {
    enum UI_SizeKind kind;
    f32 value;                      // the size value for the current kind of sizing
    f32 strictness;                 // what percent of the final computed size value can not be given up?
};

enum Axis2 {
    Axis2_X,
    Axis2_Y,
    Axis2_COUNT,
};

typedef int UI_WidgetFlags;
enum {
    UI_WidgetFlag_Clickable       = (1<<0),
    UI_WidgetFlag_ViewScroll      = (1<<1),
    UI_WidgetFlag_DrawText        = (1<<2),
    UI_WidgetFlag_DrawBorder      = (1<<3),
    UI_WidgetFlag_DrawBackground  = (1<<4),
    UI_WidgetFlag_DrawDropShadow  = (1<<5),
    UI_WidgetFlag_Clip            = (1<<6),
    UI_WidgetFlag_HotAnimation    = (1<<7),
    UI_WidgetFlag_ActiveAnimation = (1<<8),
    // ...
};

// NOTES
// - Contains both input ('semantic' sizing) and output (computed pos/size/rect) of autolayout algorithm
// - .rect can be used in both:
//   - input event consumption on the frame immediately following autolayout pass
//   - output size on frame immediately following the autolayout pass, and rendering pass of current frame
// - struct doubles as cache and immediate-mode data structure
// - despite being cached as if a 'retained-mode' data struct, the API remains immediate-mode
struct UI_Widget {
    // tree links
    struct UI_Widget *first;
    struct UI_Widget *last;
    struct UI_Widget *next;
    struct UI_Widget *prev;
    struct UI_Widget *parent;

    // hash links
    struct UI_Widget *hash_next;
    struct UI_Widget *hash_prev;

    // key + generation info
    UI_Key key;
    long last_frame_touched_index;

    // per-frame info provided by builders
    UI_WidgetFlags flags;
    String8 string;
    struct UI_Size semantic_size[Axis2_COUNT];    // widget 'input' (semantic info about layout and sizing)

    // widget 'output' (computed every frame)
    f32 computed_rel_position[Axis2_COUNT];     // computed position (relative to parent position)
    f32 computed_size[Axis2_COUNT];             // computed size (in pixels)
//    struct Rect2f rect;                           // final on-screen rectangular coords produced when taking into account computed values and rest of hierarchy

    // persistent data (for things like transitions)
    f32 hot;
    f32 active;
};

/*
 * Autolayout algorithm overview: (for each axis)
 * - (any order) calculate 'standalone' sizes
 *   - sizes that don't depend on other widgets, can be calculated purely w/data from single widget (SizeKind_Pixels, SizeKind_TextContent)
 * - (pre-order) calculate 'upwards-dependent' sizes
 *   - sizes that strictly depend on ancestor's size (other than ancenstors that have 'downward-dependent' sizes on the given axis) (SizeKind_PercentOfParent)
 * - (post-order) calculate 'downwards-dependent' sizes
 *   - sizes that depend on sizes of descendants (SizeKind_ChildrenSum)
 * - (pre-order) solve violations
 *   - for-ea level in hierarchy, verify children don't extend past boundaries of a given parent (unless explicitly allowed, eg. parent is scrollable on a given axis), best effort, not always perfectly solvable
 *   - if violation, take a proportion of ea child widget's size (on the given axis) proportional to both size of violation and (1 - strictness) for strictness specified in semantic size on child widget for given axis
 * - (pre-order) given the calc'd sizes of ea widget, compute relative positions of ea widget
 *   - lay out on an axis which can be specified on any parent node
 *   - can also compute final screen-coord rect
 *
 * unsolved: nothing related to wrapping (leaves up to builder code)
 */

// ----------------------------------------------------------------------------
// UI API
// ----------------------------------------------------------------------------

// basic key type helpers
UI_Key UI_KeyNull(void);
UI_Key UI_KeyFromString(String8 string);
b8 UI_KeyMatch(UI_Key a, UI_Key b);

// construct a widget, looking up from cache if possible,
// and push it as a new child of the active parent
struct UI_Widget *UI_WidgetMake(UI_WidgetFlags flags, String8 string);
struct UI_Widget *UI_WidgetEquipChildLayoutAxis(struct UI_Widget *widget, enum Axis2 axis);

// managing the parent stack
struct UI_Widget *UI_PushParent(struct UI_Widget *widget);
struct UI_Widget *UI_PopParent(void);


// interaction results,
// TODO - bools can be switched to 8-bit type B8 in fleury's code
struct UI_Interaction {
    struct UI_Widget *widget;
//    struct Vec2f mouse;
//    struct Vec2f drag_delta;
    b8 clicked;
    b8 double_clicked;
    b8 right_clicked;
    b8 pressed;
    b8 released;
    b8 dragging;
    b8 hovering;
};

struct UI_Interaction UI_InteractionFromWidget(struct UI_Widget *widget);



// example implementation for something like a button...
struct UI_Interaction UI_Button(String8 string) {
    UI_WidgetFlags flags =
              UI_WidgetFlag_Clickable
            | UI_WidgetFlag_DrawBorder
            | UI_WidgetFlag_DrawText
            | UI_WidgetFlag_DrawBackground
            | UI_WidgetFlag_HotAnimation
            | UI_WidgetFlag_ActiveAnimation
            ;
    struct UI_Widget *widget = UI_WidgetMake(flags, string);
    struct UI_Interaction interaction = UI_InteractionFromWidget(widget);
    return interaction;
}
// usage example:
//if (UI_Button("Foo").clicked) {
//    // handle button click...
//}



#endif //FIDDLE_UI_H
