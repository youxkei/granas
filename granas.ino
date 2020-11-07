#include <hidboot.h>
#include <hiduniversal.h>
#include <usbhub.h>

#include <HID-Project.h>

using InputStickState = uint8_t;
using InputButtonState = uint32_t;

constexpr bool OUTPUT_TO_SERIAL = false;

namespace buf_index {
    constexpr size_t X_AXIS  = 1;
    constexpr size_t Y_AXIS  = 2;
    constexpr size_t BUTTON  = 5;
    constexpr size_t TRIGGER = 6;
    constexpr size_t CENTER  = 7;
}

namespace input {
    constexpr uint8_t LEFT  = 0x00;
    constexpr uint8_t RIGHT = 0xFF;
    constexpr uint8_t UP    = 0x00;
    constexpr uint8_t DOWN  = 0xFF;

    constexpr uint8_t SQUARE   = 0x10;
    constexpr uint8_t CROSS    = 0x20;
    constexpr uint8_t CIRCLE   = 0x40;
    constexpr uint8_t TRIANGLE = 0x80;

    constexpr uint8_t L1     = 0x01;
    constexpr uint8_t R1     = 0x02;
    constexpr uint8_t L2     = 0x04;
    constexpr uint8_t R2     = 0x08;
    constexpr uint8_t SELECT = 0x10;
    constexpr uint8_t START  = 0x20;
    constexpr uint8_t L3     = 0x40;
    constexpr uint8_t R3     = 0x80;

    constexpr uint8_t CENTER = 0x01;
}

namespace input_state {
    constexpr InputStickState LEFT  = 1 << 0;
    constexpr InputStickState RIGHT = 1 << 1;
    constexpr InputStickState UP    = 1 << 2;
    constexpr InputStickState DOWN  = 1 << 3;

    constexpr InputButtonState SQUARE   = 1 << 0;
    constexpr InputButtonState CROSS    = 1 << 1;
    constexpr InputButtonState CIRCLE   = 1 << 2;
    constexpr InputButtonState TRIANGLE = 1 << 3;
    constexpr InputButtonState L1       = 1 << 4;
    constexpr InputButtonState R1       = 1 << 5;
    constexpr InputButtonState L2       = 1 << 6;
    constexpr InputButtonState R2       = 1 << 7;
    constexpr InputButtonState SELECT   = 1 << 8;
    constexpr InputButtonState START    = 1 << 9;
    constexpr InputButtonState L3       = 1 << 10;
    constexpr InputButtonState R3       = 1 << 11;
    constexpr InputButtonState CENTER   = 1 << 12;
}

namespace input_to_input_state {
    constexpr InputStickState LEFT  = input_state::LEFT;
    constexpr InputStickState RIGHT = input_state::RIGHT;
    constexpr InputStickState UP    = input_state::UP;
    constexpr InputStickState DOWN  = input_state::DOWN;

    constexpr InputButtonState SQUARE   = input_state::SQUARE;
    constexpr InputButtonState CROSS    = input_state::CROSS;
    constexpr InputButtonState CIRCLE   = input_state::CIRCLE;
    constexpr InputButtonState TRIANGLE = input_state::TRIANGLE;
    constexpr InputButtonState L1       = input_state::R1;
    constexpr InputButtonState R1       = input_state::L3;
    constexpr InputButtonState L2       = input_state::R2;
    constexpr InputButtonState R2       = input_state::R3;
    constexpr InputButtonState SELECT   = input_state::SELECT;
    constexpr InputButtonState START    = input_state::START;
    constexpr InputButtonState L3       = input_state::L1;
    constexpr InputButtonState R3       = input_state::L2;
    constexpr InputButtonState CENTER   = input_state::CENTER;
}

namespace stick_state_index {
    constexpr size_t LEFT = 0;
    constexpr size_t RIGHT = 1;
    constexpr size_t UP = 2;
    constexpr size_t DOWN = 3;
    constexpr size_t COUNT = 4;
}

namespace repeat {
    constexpr uint32_t INTERVAL_USEC = 33333;
    constexpr InputButtonState ENABLE_INPUT_STATE = input_state::R2;
}

namespace pulse {
    constexpr uint32_t DURATION_USEC = 16667;
    constexpr InputButtonState ENABLE_INPUT_STATE = 0;
}

namespace force_down {
    constexpr InputButtonState ENABLE_INPUT_STATE = input_state::TRIANGLE;
}


namespace output {
    constexpr int16_t NEUTRAL = 0;
    constexpr int16_t LEFT    = -0x7FFF;
    constexpr int16_t RIGHT   = 0x7FFF;
    constexpr int16_t UP      = -0x7FFF;
    constexpr int16_t DOWN    = 0x7FFF;
}

struct StickState {
    bool pressed;
    uint32_t repeatTime;
    uint32_t pulseTime;
};

struct {
    struct {
        InputStickState stick;
        InputButtonState button;
    } input;

    struct {
        StickState left;
        StickState right;
        StickState up;
        StickState down;
    } stick;

    bool changed;
} state;

class : public HIDReportParser {
public:
    virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
        uint32_t now = micros();

        InputStickState newInputStickState = 0;

        switch (buf[buf_index::X_AXIS]) {
            case input::LEFT:
                newInputStickState |= input_to_input_state::LEFT;
                break;

            case input::RIGHT:
                newInputStickState |= input_to_input_state::RIGHT;
                break;
        }

        switch (buf[buf_index::Y_AXIS]) {
            case input::UP:
                newInputStickState |= input_to_input_state::UP;
                break;

            case input::DOWN:
                newInputStickState |= input_to_input_state::DOWN;
                break;
        }

        InputStickState inputStickStateDiff = state.input.stick ^ newInputStickState;

        if (inputStickStateDiff & newInputStickState & input_state::LEFT) {
            state.stick.left.pressed = true;
            state.stick.left.repeatTime = now;
            state.stick.left.pulseTime = now;
        }

        if (inputStickStateDiff & newInputStickState & input_state::RIGHT) {
            state.stick.right.pressed = true;
            state.stick.right.repeatTime = now;
            state.stick.right.pulseTime = now;
        }

        if (inputStickStateDiff & newInputStickState & input_state::UP) {
            state.stick.up.pressed = true;
            state.stick.up.repeatTime = now;
            state.stick.up.pulseTime = now;
        }

        if (inputStickStateDiff & newInputStickState & input_state::DOWN) {
            state.stick.down.pressed = true;
            state.stick.down.repeatTime = now;
            state.stick.down.pulseTime = now;
        }

        InputButtonState newInputButtonState = 0;

        if (buf[buf_index::BUTTON]  & input::SQUARE)   newInputButtonState |= input_to_input_state::SQUARE;
        if (buf[buf_index::BUTTON]  & input::CROSS)    newInputButtonState |= input_to_input_state::CROSS;
        if (buf[buf_index::BUTTON]  & input::CIRCLE)   newInputButtonState |= input_to_input_state::CIRCLE;
        if (buf[buf_index::BUTTON]  & input::TRIANGLE) newInputButtonState |= input_to_input_state::TRIANGLE;
        if (buf[buf_index::TRIGGER] & input::L1)       newInputButtonState |= input_to_input_state::L1;
        if (buf[buf_index::TRIGGER] & input::R1)       newInputButtonState |= input_to_input_state::R1;
        if (buf[buf_index::TRIGGER] & input::L2)       newInputButtonState |= input_to_input_state::L2;
        if (buf[buf_index::TRIGGER] & input::R2)       newInputButtonState |= input_to_input_state::R2;
        if (buf[buf_index::TRIGGER] & input::SELECT)   newInputButtonState |= input_to_input_state::SELECT;
        if (buf[buf_index::TRIGGER] & input::START)    newInputButtonState |= input_to_input_state::START;
        if (buf[buf_index::TRIGGER] & input::L3)       newInputButtonState |= input_to_input_state::L3;
        if (buf[buf_index::TRIGGER] & input::R3)       newInputButtonState |= input_to_input_state::R3;
        if (buf[buf_index::CENTER]  & input::CENTER)   newInputButtonState |= input_to_input_state::CENTER;

        InputButtonState inputButtonStateDiff = state.input.button ^ newInputButtonState;

        if (inputStickStateDiff || inputButtonStateDiff) {
            state.changed = true;
        }

        state.input.stick = newInputStickState;
        state.input.button = newInputButtonState;
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
}

void loop() {
    usb.Task();

    uint32_t now = micros();
    bool repeatEnabled = state.input.button & repeat::ENABLE_INPUT_STATE;
    bool pulseEnabled = state.input.button & pulse::ENABLE_INPUT_STATE;
    bool forceDown = state.input.button & force_down::ENABLE_INPUT_STATE;


    // substraction can avoid the glitch caused by overflow
    if (repeatEnabled && state.input.stick & input_state::LEFT && now - state.stick.left.repeatTime >= repeat::INTERVAL_USEC) {
        state.stick.left.pressed = !state.stick.left.pressed;
        state.stick.left.repeatTime += repeat::INTERVAL_USEC;
        state.changed = true;
    }

    if (repeatEnabled && state.input.stick & input_state::RIGHT && now - state.stick.right.repeatTime >= repeat::INTERVAL_USEC) {
        state.stick.right.pressed = !state.stick.right.pressed;
        state.stick.right.repeatTime += repeat::INTERVAL_USEC;
        state.changed = true;
    }

    if (repeatEnabled && state.input.stick & input_state::UP && now - state.stick.up.repeatTime >= repeat::INTERVAL_USEC) {
        state.stick.up.pressed = !state.stick.up.pressed;
        state.stick.up.repeatTime += repeat::INTERVAL_USEC;
        state.changed = true;
    }

    if (repeatEnabled && state.input.stick & input_state::DOWN && now - state.stick.down.repeatTime >= repeat::INTERVAL_USEC) {
        state.stick.down.pressed = !state.stick.down.pressed;
        state.stick.down.repeatTime += repeat::INTERVAL_USEC;

        if (!forceDown) state.changed = true;
    }


    if (pulseEnabled && state.input.stick & input_state::LEFT && state.stick.left.pressed && now - state.stick.left.pulseTime >= pulse::DURATION_USEC) {
        state.stick.left.pressed = false;
        state.changed = true;
    }

    if (pulseEnabled && state.input.stick & input_state::RIGHT && state.stick.right.pressed && now - state.stick.right.pulseTime >= pulse::DURATION_USEC) {
        state.stick.right.pressed = false;
        state.changed = true;
    }

    if (pulseEnabled && state.input.stick & input_state::UP && state.stick.up.pressed && now - state.stick.up.pulseTime >= pulse::DURATION_USEC) {
        state.stick.up.pressed = false;
        state.changed = true;
    }

    if (pulseEnabled && state.input.stick & input_state::DOWN && state.stick.down.pressed && now - state.stick.down.pulseTime >= pulse::DURATION_USEC) {
        state.stick.down.pressed = false;
        if (!forceDown) state.changed = true;
    }


    if (state.input.stick & input_state::LEFT && ((repeatEnabled || pulseEnabled) ? state.stick.left.pressed : true)) {
        Gamepad.xAxis(output::LEFT);
    } else if (state.input.stick & input_state::RIGHT && ((repeatEnabled || pulseEnabled) ? state.stick.right.pressed : true)) {
        Gamepad.xAxis(output::RIGHT);
    } else {
        Gamepad.xAxis(output::NEUTRAL);
    }

    if (forceDown) {
        Gamepad.yAxis(output::DOWN);
    } else if (state.input.stick & input_state::UP && ((repeatEnabled || pulseEnabled) ? state.stick.up.pressed : true)) {
        Gamepad.yAxis(output::UP);
    } else if (state.input.stick & input_state::DOWN && ((repeatEnabled || pulseEnabled) ? state.stick.down.pressed : true)) {
        Gamepad.yAxis(output::DOWN);
    } else {
        Gamepad.yAxis(output::NEUTRAL);
    }

    Gamepad.buttons(state.input.button);

    if (state.changed) Gamepad.write();
    state.changed = false;
}
