#include <hidboot.h>
#include <hiduniversal.h>
#include <usbhub.h>

#include <HID-Project.h>

using StickState = uint8_t;
using ButtonState = uint32_t;

constexpr bool OUTPUT_TO_SERIAL = false;

constexpr size_t INPUT_X_AXIS_INDEX  = 1;
constexpr size_t INPUT_Y_AXIS_INDEX  = 2;
constexpr size_t INPUT_BUTTON_INDEX  = 5;
constexpr size_t INPUT_TRIGGER_INDEX = 6;
constexpr size_t INPUT_CENTER_INDEX  = 7;

constexpr uint8_t INPUT_LEFT  = 0x00;
constexpr uint8_t INPUT_RIGHT = 0xFF;
constexpr uint8_t INPUT_UP    = 0x00;
constexpr uint8_t INPUT_DOWN  = 0xFF;

constexpr uint8_t INPUT_SQUARE   = 0x10;
constexpr uint8_t INPUT_CROSS    = 0x20;
constexpr uint8_t INPUT_CIRCLE   = 0x40;
constexpr uint8_t INPUT_TRIANGLE = 0x80;

constexpr uint8_t INPUT_L1     = 0x01;
constexpr uint8_t INPUT_R1     = 0x02;
constexpr uint8_t INPUT_L2     = 0x04;
constexpr uint8_t INPUT_R2     = 0x08;
constexpr uint8_t INPUT_SELECT = 0x10;
constexpr uint8_t INPUT_START  = 0x20;
constexpr uint8_t INPUT_L3     = 0x40;
constexpr uint8_t INPUT_R3     = 0x80;

constexpr uint8_t INPUT_CENTER = 0x01;

constexpr int16_t OUTPUT_NEUTRAL = 0;
constexpr int16_t OUTPUT_LEFT    = -0x7FFF;
constexpr int16_t OUTPUT_RIGHT   = 0x7FFF;
constexpr int16_t OUTPUT_UP      = -0x7FFF;
constexpr int16_t OUTPUT_DOWN    = 0x7FFF;

constexpr StickState STATE_LEFT  = 1 << 0;
constexpr StickState STATE_RIGHT = 1 << 1;
constexpr StickState STATE_UP    = 1 << 2;
constexpr StickState STATE_DOWN  = 1 << 3;

constexpr ButtonState STATE_SQUARE   = 1 << 0;
constexpr ButtonState STATE_CROSS    = 1 << 1;
constexpr ButtonState STATE_CIRCLE   = 1 << 2;
constexpr ButtonState STATE_TRIANGLE = 1 << 3;
constexpr ButtonState STATE_L1       = 1 << 4;
constexpr ButtonState STATE_R1       = 1 << 5;
constexpr ButtonState STATE_L2       = 1 << 6;
constexpr ButtonState STATE_R2       = 1 << 7;
constexpr ButtonState STATE_SELECT   = 1 << 8;
constexpr ButtonState STATE_START    = 1 << 9;
constexpr ButtonState STATE_L3       = 1 << 10;
constexpr ButtonState STATE_R3       = 1 << 11;
constexpr ButtonState STATE_CENTER   = 1 << 12;

constexpr StickState INPUT_LEFT_TO_STATE  = STATE_LEFT;
constexpr StickState INPUT_RIGHT_TO_STATE = STATE_RIGHT;
constexpr StickState INPUT_UP_TO_STATE    = STATE_UP;
constexpr StickState INPUT_DOWN_TO_STATE  = STATE_DOWN;

constexpr ButtonState INPUT_SQUARE_TO_STATE   = STATE_SQUARE;
constexpr ButtonState INPUT_CROSS_TO_STATE    = STATE_CROSS;
constexpr ButtonState INPUT_CIRCLE_TO_STATE   = STATE_CIRCLE;
constexpr ButtonState INPUT_TRIANGLE_TO_STATE = STATE_TRIANGLE;
constexpr ButtonState INPUT_L1_TO_STATE       = STATE_R1;
constexpr ButtonState INPUT_R1_TO_STATE       = STATE_L3;
constexpr ButtonState INPUT_L2_TO_STATE       = STATE_R2;
constexpr ButtonState INPUT_R2_TO_STATE       = STATE_R3;
constexpr ButtonState INPUT_SELECT_TO_STATE   = STATE_SELECT;
constexpr ButtonState INPUT_START_TO_STATE    = STATE_START;
constexpr ButtonState INPUT_L3_TO_STATE       = STATE_L1;
constexpr ButtonState INPUT_R3_TO_STATE       = STATE_L2;
constexpr ButtonState INPUT_CENTER_TO_STATE   = STATE_CENTER;

constexpr uint32_t REPEAT_INTERVAL_USEC = 33333;
constexpr ButtonState REPEAT_ENABLE = STATE_R2;
constexpr ButtonState FORCE_DOWN = STATE_L2;

StickState stickState;
ButtonState buttonState;

struct {
    bool left;
    bool right;
    bool up;
    bool down;
} repeatState;

struct {
    uint32_t left;
    uint32_t right;
    uint32_t up;
    uint32_t down;
} repeatTime;

bool changed = false;

class : public HIDReportParser {
public:
    virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
        uint32_t now = micros();

        StickState newStickState = 0;

        switch (buf[INPUT_X_AXIS_INDEX]) {
            case INPUT_LEFT:
                newStickState |= INPUT_LEFT_TO_STATE;
                break;

            case INPUT_RIGHT:
                newStickState |= INPUT_RIGHT_TO_STATE;
                break;
        }

        switch (buf[INPUT_Y_AXIS_INDEX]) {
            case INPUT_UP:
                newStickState |= INPUT_UP_TO_STATE;
                break;

            case INPUT_DOWN:
                newStickState |= INPUT_DOWN_TO_STATE;
                break;
        }

        StickState stickDiff = stickState ^ newStickState;

        if (stickDiff & newStickState & STATE_LEFT) {
            repeatTime.left = now;
            repeatState.left = true;
        }

        if (stickDiff & newStickState & STATE_RIGHT) {
            repeatTime.right = now;
            repeatState.right = true;
        }

        if (stickDiff & newStickState & STATE_UP) {
            repeatTime.up = now;
            repeatState.up = true;
        }

        if (stickDiff & newStickState & STATE_DOWN) {
            repeatTime.down = now;
            repeatState.down = true;
        }

        ButtonState newButtonState = 0;

        if (buf[INPUT_BUTTON_INDEX]  & INPUT_SQUARE)   newButtonState |= INPUT_SQUARE_TO_STATE;
        if (buf[INPUT_BUTTON_INDEX]  & INPUT_CROSS)    newButtonState |= INPUT_CROSS_TO_STATE;
        if (buf[INPUT_BUTTON_INDEX]  & INPUT_CIRCLE)   newButtonState |= INPUT_CIRCLE_TO_STATE;
        if (buf[INPUT_BUTTON_INDEX]  & INPUT_TRIANGLE) newButtonState |= INPUT_TRIANGLE_TO_STATE;
        if (buf[INPUT_TRIGGER_INDEX] & INPUT_L1)       newButtonState |= INPUT_L1_TO_STATE;
        if (buf[INPUT_TRIGGER_INDEX] & INPUT_R1)       newButtonState |= INPUT_R1_TO_STATE;
        if (buf[INPUT_TRIGGER_INDEX] & INPUT_L2)       newButtonState |= INPUT_L2_TO_STATE;
        if (buf[INPUT_TRIGGER_INDEX] & INPUT_R2)       newButtonState |= INPUT_R2_TO_STATE;
        if (buf[INPUT_TRIGGER_INDEX] & INPUT_SELECT)   newButtonState |= INPUT_SELECT_TO_STATE;
        if (buf[INPUT_TRIGGER_INDEX] & INPUT_START)    newButtonState |= INPUT_START_TO_STATE;
        if (buf[INPUT_TRIGGER_INDEX] & INPUT_L3)       newButtonState |= INPUT_L3_TO_STATE;
        if (buf[INPUT_TRIGGER_INDEX] & INPUT_R3)       newButtonState |= INPUT_R3_TO_STATE;
        if (buf[INPUT_CENTER_INDEX]  & INPUT_CENTER)   newButtonState |= INPUT_CENTER_TO_STATE;

        ButtonState buttonDiff = buttonState ^ newButtonState;

        if (stickDiff || buttonDiff) {
            changed = true;
        }

        stickState = newStickState;
        buttonState = newButtonState;
    }
} hidReportParser;

USB usb;
USBHub hub(&usb);
HIDUniversal hid(&usb);

void setup() {
    if constexpr (OUTPUT_TO_SERIAL) {
        Serial.begin(115200);
        while (!Serial);

        Serial.println("start");
    }

    if (usb.Init() == -1) {
        if constexpr (OUTPUT_TO_SERIAL) {
            Serial.println("failed to initialize USB");
        }
    }

    if (!hid.SetReportParser(0, &hidReportParser)) {
        if constexpr (OUTPUT_TO_SERIAL) {
            Serial.println("failed to set report parser");
        }
    }

    Gamepad.begin();

    uint32_t i = -2;
    Serial.println(i, HEX);
    uint32_t j = i + 4;
    Serial.println(j, HEX);
    Serial.println(j - i, HEX);
}

void loop() {
    usb.Task();

    uint32_t now = micros();
    bool repeatEnabled = buttonState & REPEAT_ENABLE;
    bool forceDown = buttonState & FORCE_DOWN;

    if (repeatEnabled && stickState & STATE_LEFT && now - repeatTime.left >= REPEAT_INTERVAL_USEC) {
        repeatTime.left += REPEAT_INTERVAL_USEC;
        repeatState.left = !repeatState.left;
        changed = true;
    }

    if (repeatEnabled && stickState & STATE_RIGHT && now - repeatTime.right >= REPEAT_INTERVAL_USEC) {
        repeatTime.right += REPEAT_INTERVAL_USEC;
        repeatState.right = !repeatState.right;
        changed = true;
    }

    if (repeatEnabled && stickState & STATE_UP && now - repeatTime.up >= REPEAT_INTERVAL_USEC) {
        repeatTime.up += REPEAT_INTERVAL_USEC;
        repeatState.up = !repeatState.up;
        changed = true;
    }

    if (repeatEnabled && stickState & STATE_DOWN && now - repeatTime.down >= REPEAT_INTERVAL_USEC) {
        repeatTime.down += REPEAT_INTERVAL_USEC;
        repeatState.down = !repeatState.down;

        if (!forceDown) changed = true;
    }

    if (stickState & STATE_LEFT && (repeatEnabled ? repeatState.left : true)) {
        Gamepad.xAxis(OUTPUT_LEFT);
    } else if (stickState & STATE_RIGHT && (repeatEnabled ? repeatState.right : true)) {
        Gamepad.xAxis(OUTPUT_RIGHT);
    } else {
        Gamepad.xAxis(OUTPUT_NEUTRAL);
    }

    if (forceDown) {
        Gamepad.yAxis(OUTPUT_DOWN);
    } else if (stickState & STATE_UP && (repeatEnabled ? repeatState.up : true)) {
        Gamepad.yAxis(OUTPUT_UP);
    } else if (stickState & STATE_DOWN && (repeatEnabled ? repeatState.down : true)) {
        Gamepad.yAxis(OUTPUT_DOWN);
    } else {
        Gamepad.yAxis(OUTPUT_NEUTRAL);
    }

    Gamepad.buttons(buttonState);

    if (changed) Gamepad.write();
    changed = false;
}
