#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <string.h> // Include for strlen, strcpy, strcat

// --- Configuration ---
// Define UI dimensions based on the desired portrait layout (like the HTML)
#define UI_WIDTH 320
#define UI_HEIGHT 480

// Layout dimensions (matching CSS, adapted for 320x480 UI)
#define STATUS_BAR_HEIGHT 20
#define TEXT_AREA_HEIGHT 140 // Adjusted from 150 in HTML to 140 as requested
#define KEYBOARD_PADDING 9
#define TOP_ROW_HEIGHT 45
#define BOTTOM_ROW_HEIGHT 40
#define KEY_ROW_V_GAP 12
// #define KEY_H_GAP 20 // Will be calculated based on key width and UI width
#define TOP_ROW_H_GAP 20
#define BOTTOM_ROW_H_GAP 18
#define ACTION_BTN_WIDTH 60
#define BLOB_KEY_WIDTH 62  // Fixed 62px width as requested
#define BLOB_KEY_HEIGHT 50 // Fixed 50px height as requested

// Calculate the height for the keyboard area
#define KEYBOARD_HEIGHT (UI_HEIGHT - STATUS_BAR_HEIGHT - TEXT_AREA_HEIGHT)

// --- Colors ---
#define COLOR_BUTTON lv_color_hex(0xf79b2b)
#define COLOR_BUTTON_ACTIVE lv_color_hex(0x3aeb3a)
#define COLOR_BLACK lv_color_hex(0x000000)
#define COLOR_WHITE lv_color_hex(0xffffff)
#define COLOR_STATUS_BAR_TEXT COLOR_WHITE
#define COLOR_TEXT_AREA_BG COLOR_WHITE
#define COLOR_TEXT_AREA_TEXT COLOR_BLACK
#define COLOR_KEYBOARD_BG COLOR_BLACK
#define COLOR_INPUT_BG COLOR_WHITE
#define COLOR_INPUT_TEXT COLOR_BLACK
#define COLOR_CURSOR lv_color_hex(0x0004d4)
#define COLOR_CURSOR_INPUT COLOR_BUTTON

// --- Globals ---
static lv_obj_t *scr;
static lv_obj_t *text_content_label;
static lv_obj_t *input_text_label;
static lv_obj_t *text_cursor;
static lv_obj_t *input_cursor;
static lv_timer_t *cursor_timer;

static char input_buffer[128] = "";
static int active_blob_key_letter_index = -1; // 0: left, 1: center, 2: right
static lv_point_t last_touch_point = {0, 0};

// --- Styles ---
static lv_style_t style_key;
static lv_style_t style_key_pressed;
static lv_style_t style_blob_key_cont;
static lv_style_t style_input_cont;
static lv_style_t style_text_area;
static lv_style_t style_status_bar;
static lv_style_t style_keyboard_area;
static lv_style_t style_letter_label;
static lv_style_t style_letter_label_active;
static lv_style_t style_letter_label_hidden;

// --- Function Prototypes ---
static void create_status_bar(lv_obj_t *parent);
static void create_text_area(lv_obj_t *parent);
static void create_keyboard(lv_obj_t *parent);
static lv_obj_t *create_blob_key(lv_obj_t *parent, const char *letters);
static void blob_key_event_cb(lv_event_t *e);
static void action_button_event_cb(lv_event_t *e);
static void update_blob_key_visuals(lv_obj_t *key, int letter_index, bool pressed);
static void reset_blob_key_visuals(lv_obj_t *key);
static void cursor_blink_timer_cb(lv_timer_t *timer);
static void update_input_display();
static void update_text_area_display();
static void accept_input();
static void clear_input();
static void add_char_to_input(char c);

// --- Style Initialization ---
void init_styles()
{
    // --- Key Style (Common for Action and Blob) ---
    lv_style_init(&style_key);
    lv_style_set_radius(&style_key, 10);
    lv_style_set_border_width(&style_key, 2);
    lv_style_set_border_color(&style_key, COLOR_BUTTON);
    lv_style_set_text_color(&style_key, COLOR_BUTTON);
    lv_style_set_bg_opa(&style_key, LV_OPA_TRANSP);      // Transparent background for containers
    lv_style_set_bg_color(&style_key, COLOR_BLACK);      // Background for actual buttons
    lv_style_set_text_font(&style_key, LV_FONT_DEFAULT); // Adjust font if needed
    lv_style_set_pad_all(&style_key, 0);
    lv_style_set_align(&style_key, LV_ALIGN_CENTER); // Center content (like labels)

    // --- Key Pressed Style ---
    lv_style_init(&style_key_pressed);
    lv_style_set_border_color(&style_key_pressed, COLOR_BUTTON_ACTIVE);
    lv_style_set_text_color(&style_key_pressed, COLOR_BUTTON_ACTIVE);
    // Add transform later if desired

    // --- Blob Key Container Style ---
    lv_style_init(&style_blob_key_cont);
    lv_style_set_radius(&style_blob_key_cont, 10); // Use standard radius for now
    lv_style_set_border_width(&style_blob_key_cont, 2);
    lv_style_set_border_color(&style_blob_key_cont, COLOR_BUTTON);
    lv_style_set_bg_color(&style_blob_key_cont, COLOR_BLACK);
    lv_style_set_bg_opa(&style_blob_key_cont, LV_OPA_COVER);
    lv_style_set_clip_corner(&style_blob_key_cont, true); // Helps with radius
    lv_style_set_pad_all(&style_blob_key_cont, 0);

    // --- Input Container Style ---
    lv_style_init(&style_input_cont);
    lv_style_set_radius(&style_input_cont, 5);
    lv_style_set_border_width(&style_input_cont, 1);
    lv_style_set_border_color(&style_input_cont, COLOR_BUTTON);
    lv_style_set_bg_color(&style_input_cont, COLOR_INPUT_BG);
    lv_style_set_bg_opa(&style_input_cont, LV_OPA_COVER);
    lv_style_set_pad_hor(&style_input_cont, 5);
    lv_style_set_pad_ver(&style_input_cont, 0);               // Pad vertically to center text
    lv_style_set_align(&style_input_cont, LV_ALIGN_LEFT_MID); // Align content vertically

    // --- Text Area Style ---
    lv_style_init(&style_text_area);
    lv_style_set_bg_color(&style_text_area, COLOR_TEXT_AREA_BG);
    lv_style_set_bg_opa(&style_text_area, LV_OPA_COVER);
    lv_style_set_pad_all(&style_text_area, 10);
    lv_style_set_border_width(&style_text_area, 0);
    lv_style_set_radius(&style_text_area, 0);

    // --- Status Bar Style ---
    lv_style_init(&style_status_bar);
    lv_style_set_bg_color(&style_status_bar, COLOR_BLACK);
    lv_style_set_bg_opa(&style_status_bar, LV_OPA_COVER);
    lv_style_set_text_color(&style_status_bar, COLOR_STATUS_BAR_TEXT);
    lv_style_set_pad_hor(&style_status_bar, 10);
    lv_style_set_pad_ver(&style_status_bar, 0);
    lv_style_set_border_width(&style_status_bar, 0);
    lv_style_set_radius(&style_status_bar, 0);

    // --- Keyboard Area Style ---
    lv_style_init(&style_keyboard_area);
    lv_style_set_bg_color(&style_keyboard_area, COLOR_KEYBOARD_BG);
    lv_style_set_bg_opa(&style_keyboard_area, LV_OPA_COVER);
    lv_style_set_pad_all(&style_keyboard_area, KEYBOARD_PADDING);
    lv_style_set_border_width(&style_keyboard_area, 0);
    lv_style_set_radius(&style_keyboard_area, 0);

    // --- Letter Label Style ---
    lv_style_init(&style_letter_label);
    lv_style_set_text_color(&style_letter_label, COLOR_BUTTON);
    lv_style_set_text_font(&style_letter_label, &lv_font_montserrat_14); // Using available font
    lv_style_set_text_opa(&style_letter_label, LV_OPA_COVER);

    // --- Letter Label Active Style ---
    lv_style_init(&style_letter_label_active);
    lv_style_set_text_color(&style_letter_label_active, COLOR_BUTTON_ACTIVE);
    lv_style_set_text_opa(&style_letter_label_active, LV_OPA_COVER);

    // --- Letter Label Hidden Style ---
    lv_style_init(&style_letter_label_hidden);
    lv_style_set_text_opa(&style_letter_label_hidden, LV_OPA_TRANSP); // Hide by making transparent
}

// --- UI Creation Functions ---

void create_status_bar(lv_obj_t *parent)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_add_style(bar, &style_status_bar, 0);
    lv_obj_set_size(bar, UI_WIDTH, STATUS_BAR_HEIGHT); // Use UI_WIDTH
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    // Dots (Simplified)
    lv_obj_t *dots_cont = lv_obj_create(bar);
    lv_obj_remove_style_all(dots_cont); // Remove default padding/border
    lv_obj_set_size(dots_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(dots_cont, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_flex_flow(dots_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dots_cont, 3, 0);
    lv_obj_set_style_bg_opa(dots_cont, LV_OPA_TRANSP, 0); // Make container transparent

    for (int i = 0; i < 5; i++)
    {
        lv_obj_t *dot = lv_obj_create(dots_cont);
        lv_obj_set_size(dot, 6, 6);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        if (i < 4)
        {
            lv_obj_set_style_bg_color(dot, COLOR_WHITE, 0);
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(dot, 0, 0);
        }
        else
        {
            lv_obj_set_style_bg_opa(dot, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_color(dot, COLOR_WHITE, 0);
            lv_obj_set_style_border_width(dot, 1, 0);
        }
    }

    // Time
    lv_obj_t *time_label = lv_label_create(bar);
    lv_label_set_text(time_label, "9:41"); // Static for now
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);

    // Battery (Using Symbol)
    lv_obj_t *battery_label = lv_label_create(bar);
    lv_label_set_text(battery_label, LV_SYMBOL_BATTERY_FULL);
    lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, 0, 0);
}

void create_text_area(lv_obj_t *parent)
{
    lv_obj_t *area = lv_obj_create(parent);
    lv_obj_remove_style_all(area);
    lv_obj_add_style(area, &style_text_area, 0);
    lv_obj_set_size(area, UI_WIDTH, TEXT_AREA_HEIGHT); // Use UI_WIDTH
    lv_obj_align(area, LV_ALIGN_TOP_MID, 0, STATUS_BAR_HEIGHT);
    lv_obj_remove_flag(area, LV_OBJ_FLAG_SCROLLABLE);

    text_content_label = lv_label_create(area);
    lv_label_set_text(text_content_label, "");
    lv_obj_set_width(text_content_label, lv_pct(100));
    lv_label_set_long_mode(text_content_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(text_content_label, COLOR_TEXT_AREA_TEXT, 0);
    lv_obj_set_style_text_font(text_content_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_max_height(text_content_label, lv_pct(100), 0); // Constrain height
    lv_obj_align(text_content_label, LV_ALIGN_TOP_LEFT, 0, 0);

    // Cursor
    text_cursor = lv_obj_create(area);
    lv_obj_set_size(text_cursor, 2, 20); // Match font size
    lv_obj_set_style_bg_color(text_cursor, COLOR_CURSOR, 0);
    lv_obj_set_style_bg_opa(text_cursor, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(text_cursor, 0, 0);
    lv_obj_add_flag(text_cursor, LV_OBJ_FLAG_HIDDEN); // Start hidden
    // Position update needed in update_text_area_display
}

void create_keyboard(lv_obj_t *parent)
{
    // Create keyboard container
    lv_obj_t *kb_area = lv_obj_create(parent);
    lv_obj_remove_style_all(kb_area);
    lv_obj_add_style(kb_area, &style_keyboard_area, 0);
    // Explicitly set size and position based on UI dimensions
    lv_obj_set_size(kb_area, UI_WIDTH, KEYBOARD_HEIGHT);
    lv_obj_set_pos(kb_area, 0, STATUS_BAR_HEIGHT + TEXT_AREA_HEIGHT); // Position below text area
    lv_obj_remove_flag(kb_area, LV_OBJ_FLAG_SCROLLABLE);

    // Calculate inner dimensions (accounting for padding)
    lv_coord_t kb_inner_width = UI_WIDTH - 2 * KEYBOARD_PADDING;

    // Track vertical position as we build the keyboard
    lv_coord_t current_y = 0; // Start at the top of the kb_area (padding handled by kb_area style)

    // --- Top Row ---
    lv_obj_t *top_row_cont = lv_obj_create(kb_area);
    lv_obj_remove_style_all(top_row_cont);
    lv_obj_set_size(top_row_cont, kb_inner_width, TOP_ROW_HEIGHT);
    lv_obj_set_pos(top_row_cont, 0, current_y); // Position relative to kb_area top
    lv_obj_set_style_pad_all(top_row_cont, 0, 0);
    lv_obj_remove_flag(top_row_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(top_row_cont, LV_OPA_TRANSP, 0); // Make container transparent

    // Clear button (left)
    lv_obj_t *clear_btn = lv_button_create(top_row_cont);
    lv_obj_remove_style_all(clear_btn); // Remove button base style
    lv_obj_add_style(clear_btn, &style_key, 0);
    lv_obj_add_style(clear_btn, &style_key_pressed, LV_STATE_PRESSED);
    lv_obj_align(clear_btn, LV_ALIGN_DEFAULT, 0, 0);
    lv_obj_set_size(clear_btn, ACTION_BTN_WIDTH, TOP_ROW_HEIGHT);
    lv_obj_set_pos(clear_btn, 0, 0);
    lv_obj_add_event_cb(clear_btn, action_button_event_cb, LV_EVENT_CLICKED, (void *)"clear");

    lv_obj_t *clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, "clear");
    lv_obj_center(clear_label);

    // Accept button (right)
    lv_obj_t *accept_btn = lv_button_create(top_row_cont);
    lv_obj_remove_style_all(accept_btn); // Remove button base style
    lv_obj_add_style(accept_btn, &style_key, 0);
    lv_obj_add_style(accept_btn, &style_key_pressed, LV_STATE_PRESSED);
    lv_obj_align(accept_btn, LV_ALIGN_DEFAULT, 0, 0);
    lv_obj_set_size(accept_btn, ACTION_BTN_WIDTH, TOP_ROW_HEIGHT);
    lv_obj_set_pos(accept_btn, kb_inner_width - ACTION_BTN_WIDTH, 0);
    lv_obj_add_event_cb(accept_btn, action_button_event_cb, LV_EVENT_CLICKED, (void *)"accept");

    lv_obj_t *accept_label = lv_label_create(accept_btn);
    lv_label_set_text(accept_label, "accept");
    lv_obj_center(accept_label);

    // Input container (middle)
    lv_coord_t input_width = kb_inner_width - 2 * ACTION_BTN_WIDTH - 2 * TOP_ROW_H_GAP;
    lv_obj_t *input_cont = lv_obj_create(top_row_cont);
    lv_obj_remove_style_all(input_cont);
    lv_obj_add_style(input_cont, &style_input_cont, 0);
    lv_obj_align(input_cont, LV_ALIGN_DEFAULT, 0, 0);
    lv_obj_set_size(input_cont, input_width, TOP_ROW_HEIGHT);
    lv_obj_set_pos(input_cont, ACTION_BTN_WIDTH + TOP_ROW_H_GAP, 0);

    // Input text and cursor
    input_text_label = lv_label_create(input_cont);
    lv_label_set_text(input_text_label, "");
    lv_obj_set_style_text_color(input_text_label, COLOR_INPUT_TEXT, 0);
    lv_obj_set_style_text_font(input_text_label, &lv_font_montserrat_18, 0);
    lv_obj_align(input_text_label, LV_ALIGN_LEFT_MID, 0, 0); // Align within input_cont padding

    input_cursor = lv_obj_create(input_cont);
    lv_obj_set_size(input_cursor, 2, 18);
    lv_obj_set_style_bg_color(input_cursor, COLOR_CURSOR_INPUT, 0);
    lv_obj_set_style_bg_opa(input_cursor, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(input_cursor, 0, 0);
    lv_obj_add_flag(input_cursor, LV_OBJ_FLAG_HIDDEN);
    // Position updated in update_input_display

    // Move down for next row
    current_y += TOP_ROW_HEIGHT + KEY_ROW_V_GAP;

    // --- Blob Key Rows ---
    const char *key_letters[] = {
        "bac", "fdg", "jek", "mhp",
        "qiv", "wlx", "ynz", ".o?",
        ",r-", "@s'", ":t\"", "/u!"};

    // Calculate horizontal gap based on fixed key width
    lv_coord_t calculated_h_gap = (kb_inner_width - (4 * BLOB_KEY_WIDTH)) / 3;

    for (int row = 0; row < 3; row++)
    {
        lv_obj_t *row_cont = lv_obj_create(kb_area);
        lv_obj_remove_style_all(row_cont);
        lv_obj_set_size(row_cont, kb_inner_width, BLOB_KEY_HEIGHT);
        lv_obj_set_pos(row_cont, 0, current_y); // Position relative to kb_area top
        lv_obj_set_style_pad_all(row_cont, 0, 0);
        lv_obj_remove_flag(row_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(row_cont, LV_OPA_TRANSP, 0); // Make container transparent

        for (int col = 0; col < 4; col++)
        {
            int key_index = row * 4 + col;
            lv_obj_t *key = create_blob_key(row_cont, key_letters[key_index]);
            // Size is set within create_blob_key or here if needed
            lv_obj_set_size(key, BLOB_KEY_WIDTH, BLOB_KEY_HEIGHT);
            lv_obj_set_pos(key, col * (BLOB_KEY_WIDTH + calculated_h_gap), 0);
        }

        current_y += BLOB_KEY_HEIGHT + KEY_ROW_V_GAP;
    }

    // --- Bottom Row ---
    // Adjust vertical position slightly if needed to fit exactly
    current_y = KEYBOARD_HEIGHT - KEYBOARD_PADDING - BOTTOM_ROW_HEIGHT; // Position from bottom

    lv_obj_t *bottom_row_cont = lv_obj_create(kb_area);
    lv_obj_remove_style_all(bottom_row_cont);
    lv_obj_set_size(bottom_row_cont, kb_inner_width, BOTTOM_ROW_HEIGHT);
    lv_obj_set_pos(bottom_row_cont, 0, current_y); // Position relative to kb_area top
    lv_obj_set_style_pad_all(bottom_row_cont, 0, 0);
    lv_obj_remove_flag(bottom_row_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(bottom_row_cont, LV_OPA_TRANSP, 0); // Make container transparent

    // Shift button (left)
    lv_obj_t *shift_btn = lv_button_create(bottom_row_cont);
    lv_obj_remove_style_all(shift_btn); // Remove button base style
    lv_obj_add_style(shift_btn, &style_key, 0);
    // lv_obj_add_style(shift_btn, &style_key_pressed, LV_STATE_PRESSED);
    lv_obj_align(shift_btn, LV_ALIGN_DEFAULT, 0, 0);
    lv_obj_set_size(shift_btn, ACTION_BTN_WIDTH, BOTTOM_ROW_HEIGHT);
    lv_obj_set_pos(shift_btn, 0, 0);
    // Add event handler later if needed

    lv_obj_t *shift_label = lv_label_create(shift_btn);
    lv_label_set_text(shift_label, "shift");
    lv_obj_center(shift_label);

    // Numbers button (right)
    lv_obj_t *numbers_btn = lv_button_create(bottom_row_cont);
    lv_obj_remove_style_all(numbers_btn); // Remove button base style
    lv_obj_add_style(numbers_btn, &style_key, 0);
    // lv_obj_add_style(numbers_btn, &style_key_pressed, LV_STATE_PRESSED);
    lv_obj_align(numbers_btn, LV_ALIGN_DEFAULT, 0, 0);
    lv_obj_set_size(numbers_btn, ACTION_BTN_WIDTH, BOTTOM_ROW_HEIGHT);
    lv_obj_set_pos(numbers_btn, kb_inner_width - ACTION_BTN_WIDTH, 0);
    // Add event handler later if needed

    lv_obj_t *numbers_label = lv_label_create(numbers_btn);
    lv_label_set_text(numbers_label, "123");
    lv_obj_center(numbers_label);

    // Space button (middle)
    lv_coord_t space_width = kb_inner_width - 2 * ACTION_BTN_WIDTH - 2 * BOTTOM_ROW_H_GAP;
    lv_obj_t *space_btn = lv_button_create(bottom_row_cont);
    lv_obj_remove_style_all(space_btn); // Remove button base style
    lv_obj_add_style(space_btn, &style_key, 0);
    lv_obj_add_style(space_btn, &style_key_pressed, LV_STATE_PRESSED);
    lv_obj_align(space_btn, LV_ALIGN_DEFAULT, 0, 0);
    lv_obj_set_size(space_btn, space_width, BOTTOM_ROW_HEIGHT);
    lv_obj_set_pos(space_btn, ACTION_BTN_WIDTH + BOTTOM_ROW_H_GAP, 0);
    lv_obj_add_event_cb(space_btn, action_button_event_cb, LV_EVENT_CLICKED, (void *)"space");

    lv_obj_t *space_label = lv_label_create(space_btn);
    lv_label_set_text(space_label, "space");
    lv_obj_center(space_label);
}

lv_obj_t *create_blob_key(lv_obj_t *parent, const char *letters)
{
    // Create container for the blob key
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_add_style(cont, &style_blob_key_cont, 0);
    // Size is set in create_keyboard
    lv_obj_set_user_data(cont, (void *)letters); // Store letters for event handler
    lv_obj_add_event_cb(cont, blob_key_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    // Make container clickable to receive events
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);

    // Create letter labels
    // Left letter (bottom left)
    lv_obj_t *left_letter = lv_label_create(cont);
    lv_obj_add_style(left_letter, &style_letter_label, 0);
    lv_obj_add_style(left_letter, &style_letter_label_active, LV_STATE_USER_1); // Active state
    lv_obj_add_style(left_letter, &style_letter_label_hidden, LV_STATE_USER_2); // Hidden state
    lv_label_set_text_fmt(left_letter, "%c", letters[0]);
    lv_obj_align(left_letter, LV_ALIGN_BOTTOM_LEFT, 4, -4);

    // Center letter (top center)
    lv_obj_t *center_letter = lv_label_create(cont);
    lv_obj_add_style(center_letter, &style_letter_label, 0);
    lv_obj_add_style(center_letter, &style_letter_label_active, LV_STATE_USER_1);
    lv_obj_add_style(center_letter, &style_letter_label_hidden, LV_STATE_USER_2);
    lv_label_set_text_fmt(center_letter, "%c", letters[1]);
    lv_obj_align(center_letter, LV_ALIGN_TOP_MID, 0, 2);

    // Right letter (bottom right)
    lv_obj_t *right_letter = lv_label_create(cont);
    lv_obj_add_style(right_letter, &style_letter_label, 0);
    lv_obj_add_style(right_letter, &style_letter_label_active, LV_STATE_USER_1);
    lv_obj_add_style(right_letter, &style_letter_label_hidden, LV_STATE_USER_2);
    lv_label_set_text_fmt(right_letter, "%c", letters[2]);
    lv_obj_align(right_letter, LV_ALIGN_BOTTOM_RIGHT, -4, -4);

    return cont;
}

// --- Event Handlers ---

static void blob_key_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *key = static_cast<lv_obj_t *>(lv_event_get_target(e));
    if (!key)
        return;
    const char *letters = (const char *)lv_obj_get_user_data(key);

    if (code == LV_EVENT_PRESSING)
    {
        lv_indev_t *indev = lv_indev_active();
        if (indev)
        {
            lv_indev_get_point(indev, &last_touch_point);
            // Get key's absolute position on screen
            lv_area_t key_area;
            lv_obj_get_coords(key, &key_area);

            // Calculate touch position relative to the key's top-left corner
            lv_coord_t touch_x_rel = last_touch_point.x - key_area.x1;

            // Determine letter index (0: left, 1: center, 2: right)
            // Basic thresholding based on key width
            lv_coord_t key_width = lv_area_get_width(&key_area);
            lv_coord_t left_thresh = key_width / 3;
            lv_coord_t right_thresh = 2 * key_width / 3;

            if (touch_x_rel < left_thresh)
            {
                active_blob_key_letter_index = 0;
            }
            else if (touch_x_rel > right_thresh)
            {
                active_blob_key_letter_index = 2;
            }
            else
            {
                active_blob_key_letter_index = 1;
            }
            update_blob_key_visuals(key, active_blob_key_letter_index, true);
        }
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        if (active_blob_key_letter_index != -1)
        {
            // Add character to input buffer only on valid release
            if (code == LV_EVENT_RELEASED)
            {
                add_char_to_input(letters[active_blob_key_letter_index]);
            }
            // Reset visuals after a short delay
            lv_timer_t *t = lv_timer_create([](lv_timer_t *timer)
                                            {
                                                lv_obj_t *target_key = (lv_obj_t *)timer->user_data;
                                                reset_blob_key_visuals(target_key);
                                                lv_timer_delete(timer); },
                                            100, key);
            lv_timer_set_repeat_count(t, 1);

            active_blob_key_letter_index = -1; // Reset index
        }
        else
        {
            // If released outside or lost press without selecting
            reset_blob_key_visuals(key);
        }
    }
}

static void action_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const char *action = (const char *)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED)
    {
        if (strcmp(action, "clear") == 0)
        {
            clear_input();
        }
        else if (strcmp(action, "accept") == 0)
        {
            accept_input();
        }
        else if (strcmp(action, "space") == 0)
        {
            add_char_to_input(' ');
        }
        // Add shift, numbers later
    }
}

// --- UI Update Functions ---

static void update_blob_key_visuals(lv_obj_t *key, int letter_index, bool pressed)
{
    if (!key)
        return;

    // Update container border
    lv_obj_set_style_border_color(key, pressed ? COLOR_BUTTON_ACTIVE : COLOR_BUTTON, 0);

    // Update letter visuals
    for (uint32_t i = 0; i < lv_obj_get_child_count(key); i++)
    {
        lv_obj_t *child = lv_obj_get_child(key, i);
        if (lv_obj_check_type(child, &lv_label_class))
        {
            // Determine the index this label represents (0, 1, or 2) based on its alignment/position
            // This is a bit fragile; storing index in user_data might be better
            lv_align_t align = lv_obj_get_style_align(child, 0);
            int current_label_index = -1;
            if (align == LV_ALIGN_BOTTOM_LEFT)
                current_label_index = 0;
            else if (align == LV_ALIGN_TOP_MID)
                current_label_index = 1;
            else if (align == LV_ALIGN_BOTTOM_RIGHT)
                current_label_index = 2;

            if (current_label_index == letter_index)
            {
                lv_obj_add_state(child, LV_STATE_USER_1);    // Activate style
                lv_obj_remove_state(child, LV_STATE_USER_2); // Make visible
            }
            else
            {
                lv_obj_remove_state(child, LV_STATE_USER_1); // Deactivate style
                lv_obj_add_state(child, LV_STATE_USER_2);    // Hide
            }
        }
    }
}

static void reset_blob_key_visuals(lv_obj_t *key)
{
    if (!key)
        return;
    // Reset container border
    lv_obj_set_style_border_color(key, COLOR_BUTTON, 0);

    // Reset all letter visuals to default (visible, not active)
    for (uint32_t i = 0; i < lv_obj_get_child_count(key); i++)
    {
        lv_obj_t *child = lv_obj_get_child(key, i);
        if (lv_obj_check_type(child, &lv_label_class))
        {
            lv_obj_remove_state(child, LV_STATE_USER_1); // Deactivate
            lv_obj_remove_state(child, LV_STATE_USER_2); // Make visible
        }
    }
}

static void cursor_blink_timer_cb(lv_timer_t *timer)
{
    // Toggle visibility using add/remove flag based on current state
    if (lv_obj_has_flag(text_cursor, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_remove_flag(text_cursor, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(text_cursor, LV_OBJ_FLAG_HIDDEN);
    }

    if (lv_obj_has_flag(input_cursor, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_remove_flag(input_cursor, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(input_cursor, LV_OBJ_FLAG_HIDDEN);
    }

    // Recalculate cursor positions (needed if text changes)
    update_text_area_display(); // This implicitly updates text_cursor pos
    update_input_display();     // This implicitly updates input_cursor pos
}

static void update_input_display()
{
    lv_label_set_text(input_text_label, input_buffer);
    // Position cursor after the text in the input label
    lv_obj_update_layout(input_text_label); // Ensure label size is calculated
    // Align cursor relative to the input_text_label itself
    lv_obj_align_to(input_cursor, input_text_label, LV_ALIGN_OUT_RIGHT_MID, 1, 0); // Position cursor right after text
}

static void update_text_area_display()
{
    // Position cursor at the end of the text_content_label
    lv_obj_update_layout(text_content_label); // Ensure label size is calculated

    lv_point_t pos;
    const char *txt = lv_label_get_text(text_content_label);
    uint32_t txt_len = strlen(txt);

    // Get the position of the character *at* the end of the string
    lv_label_get_letter_pos(text_content_label, txt_len, &pos);

    // Adjust vertical position based on font line height for better centering
    const lv_font_t *font = lv_obj_get_style_text_font(text_content_label, 0);
    lv_coord_t line_height = lv_font_get_line_height(font);
    lv_coord_t cursor_height = lv_obj_get_height(text_cursor);

    // If the text ends exactly at a line break, the calculated pos.y might be
    // on the *next* line. We need to detect this or adjust.
    // A simpler approach for now: align to the label's bottom if needed.
    lv_coord_t label_height = lv_obj_get_content_height(text_content_label);
    if (pos.y + line_height > label_height && label_height > 0)
    {
        pos.y = label_height - line_height; // Put cursor on the last visible line
    }

    pos.y += (line_height - cursor_height) / 2; // Center vertically on the line

    // Align cursor relative to the text_content_label's top-left
    lv_obj_align_to(text_cursor, text_content_label, LV_ALIGN_TOP_LEFT, pos.x + 1, pos.y);
}

// --- Action Functions ---

static void add_char_to_input(char c)
{
    size_t len = strlen(input_buffer);
    if (len < sizeof(input_buffer) - 1)
    {
        input_buffer[len] = c;
        input_buffer[len + 1] = '\0';
        update_input_display();
    }
}

static void clear_input()
{
    input_buffer[0] = '\0';
    update_input_display();
}

static void accept_input()
{
    if (strlen(input_buffer) > 0)
    {
        // Append input buffer to the existing text content label
        const char *current_text = lv_label_get_text(text_content_label);
        // Use lv_label_ins_text for potentially better efficiency if available/needed
        // For simplicity, using strcat here. Be mindful of buffer overflows in real apps.
        char *new_text = (char *)malloc(strlen(current_text) + strlen(input_buffer) + 1);
        if (new_text)
        {
            strcpy(new_text, current_text);
            strcat(new_text, input_buffer);
            lv_label_set_text(text_content_label, new_text);
            free(new_text);
        }
        else
        {
            log_e("Failed to allocate memory for accept_input");
        }

        clear_input();
        update_text_area_display(); // Update main text area cursor position
    }
}

// --- Arduino Setup and Loop ---

void setup()
{
#ifdef ARDUINO_USB_CDC_ON_BOOT
    delay(5000);
#endif
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    log_i("Board: %s", BOARD_NAME);
    log_i("CPU: %s rev%d, CPU Freq: %d Mhz, %d core(s)", ESP.getChipModel(), ESP.getChipRevision(), getCpuFrequencyMhz(), ESP.getChipCores());
    log_i("Free heap: %d bytes", ESP.getFreeHeap());
    log_i("Free PSRAM: %d bytes", ESP.getPsramSize());
    log_i("SDK version: %s", ESP.getSdkVersion());

    smartdisplay_init();

    auto disp = lv_disp_get_default();
    // *** Rotate the display to portrait mode (320x480) ***
    // Use ROTATION_90 or ROTATION_270 depending on how the board is mounted/oriented
    // lv_disp_set_rotation(disp, LV_DISP_ROTATION_90);

    // Initialize styles
    init_styles();

    // Create screen - Set size to the *logical* UI dimensions
    scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);              // Use children's background colors
    lv_obj_set_size(scr, UI_WIDTH, UI_HEIGHT); // Use UI dimensions

    // Create UI components
    create_status_bar(scr);
    create_text_area(scr);
    create_keyboard(scr);

    // Initialize display content
    update_input_display();
    update_text_area_display();

    // Start cursor blinking timer
    cursor_timer = lv_timer_create(cursor_blink_timer_cb, 500, NULL); // 500ms interval

    // Load the screen
    lv_screen_load(scr);

    log_i("UI Initialized (Rotated to %dx%d)", lv_disp_get_hor_res(disp), lv_disp_get_ver_res(disp));
}

void loop()
{
    auto const now = millis();
    static auto lv_last_tick = now;

    // Update the LVGL ticker
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;

    // Handle LVGL tasks
    lv_timer_handler();

    // delay(5); // Usually not needed if lv_timer_handler yields
}