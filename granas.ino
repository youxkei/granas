#include <hidboot.h>
#include <hiduniversal.h>
#include <usbhub.h>

#include <HID-Project.h>

#define OUTPUT_TO_SERIAL 0

#define INPUT_X_AXIS_INDEX 1
#define INPUT_Y_AXIS_INDEX 2
#define INPUT_BUTTON_INDEX 5
#define INPUT_TRIGGER_INDEX 6
#define INPUT_CENTER_INDEX 7

#define INPUT_LEFT    0x00
#define INPUT_RIGHT   0xFF
#define INPUT_UP      0x00
#define INPUT_DOWN    0xFF

#define INPUT_SQUARE   0x10
#define INPUT_CROSS    0x20
#define INPUT_CIRCLE   0x40
#define INPUT_TRIANGLE 0x80

#define INPUT_L1     0x01
#define INPUT_R1     0x02
#define INPUT_L2     0x04
#define INPUT_R2     0x08
#define INPUT_SELECT 0x10
#define INPUT_START  0x20
#define INPUT_L3     0x40
#define INPUT_R3     0x80

#define INPUT_CENTER 0x01

#define OUTPUT_NEUTRAL  0
#define OUTPUT_LEFT     -0x7FFF
#define OUTPUT_RIGHT    0x7FFF
#define OUTPUT_UP       -0x7FFF
#define OUTPUT_DOWN     0x7FFF

#define STATE_LEFT     (1 << 0)
#define STATE_RIGHT    (1 << 1)
#define STATE_UP       (1 << 2)
#define STATE_DOWN     (1 << 3)

#define STATE_SQUARE   (1 << 0)
#define STATE_CROSS    (1 << 1)
#define STATE_CIRCLE   (1 << 2)
#define STATE_TRIANGLE (1 << 3)
#define STATE_L1       (1 << 4)
#define STATE_R1       (1 << 5)
#define STATE_L2       (1 << 6)
#define STATE_R2       (1 << 7)
#define STATE_SELECT   (1 << 8)
#define STATE_START    (1 << 9)
#define STATE_L3       (1 << 10)
#define STATE_R3       (1 << 11)
#define STATE_CENTER   (1 << 12)

#define INPUT_LEFT_TO_STATE  STATE_LEFT
#define INPUT_RIGHT_TO_STATE STATE_RIGHT
#define INPUT_UP_TO_STATE    STATE_UP
#define INPUT_DOWN_TO_STATE  STATE_DOWN

#define INPUT_SQUARE_TO_STATE   STATE_SQUARE
#define INPUT_CROSS_TO_STATE    STATE_CROSS
#define INPUT_CIRCLE_TO_STATE   STATE_CIRCLE
#define INPUT_TRIANGLE_TO_STATE STATE_TRIANGLE
#define INPUT_L1_TO_STATE       STATE_R1
#define INPUT_R1_TO_STATE       STATE_L3
#define INPUT_L2_TO_STATE       STATE_R2
#define INPUT_R2_TO_STATE       STATE_R3
#define INPUT_SELECT_TO_STATE   STATE_SELECT
#define INPUT_START_TO_STATE    STATE_START
#define INPUT_L3_TO_STATE       STATE_L1
#define INPUT_R3_TO_STATE       STATE_L2
#define INPUT_CENTER_TO_STATE   STATE_CENTER

#define StickState uint8_t
StickState stickState;

#define ButtonState uint32_t
ButtonState buttonState;

#define REPEAT_INTERVAL_USEC 33333
#define REPEAT_ENABLE STATE_R2
#define FORCE_DOWN STATE_L2

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
#if OUTPUT_TO_SERIAL
    Serial.begin(115200);
    while (!Serial);

    Serial.println("start");
#endif

    if (usb.Init() == -1) {
#if OUTPUT_TO_SERIAL
        Serial.println("failed to initialize USB");
#endif
    }

    if (!hid.SetReportParser(0, &hidReportParser)) {
#if OUTPUT_TO_SERIAL
        Serial.println("failed to set report parser");
#endif
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
