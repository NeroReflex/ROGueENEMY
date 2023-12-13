#include "xbox360.h"
#include "message.h"

int xbox360_ev_map(const evdev_collected_t *const coll, in_message_t *const messages, size_t messages_len, void* user_data) {
	const xbox360_settings_t *const settings = (xbox360_settings_t*)user_data;
	int written_msg = 0;

    for (uint32_t i = 0; i < coll->ev_count; ++i) {
		if (coll->ev[i].type == EV_KEY) {
			in_message_t current_message = {
				.type = GAMEPAD_SET_ELEMENT
			};

			if (coll->ev[i].code == BTN_EAST) {
				current_message.data.gamepad_set.element = (settings->nintendo_layout) ? GAMEPAD_BTN_CROSS : GAMEPAD_BTN_CIRCLE;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == BTN_NORTH) {
				current_message.data.gamepad_set.element = (settings->nintendo_layout) ? GAMEPAD_BTN_TRIANGLE : GAMEPAD_BTN_SQUARE;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == BTN_SOUTH) {
				current_message.data.gamepad_set.element = (settings->nintendo_layout) ? GAMEPAD_BTN_CIRCLE : GAMEPAD_BTN_CROSS;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == BTN_WEST) {
				current_message.data.gamepad_set.element = (settings->nintendo_layout) ? GAMEPAD_BTN_SQUARE : GAMEPAD_BTN_TRIANGLE;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == BTN_SELECT) {
				current_message.data.gamepad_set.element = GAMEPAD_BTN_OPTION;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == BTN_START) {
				current_message.data.gamepad_set.element = GAMEPAD_BTN_SHARE;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == BTN_TR) {
				current_message.data.gamepad_set.element = GAMEPAD_BTN_R1;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == BTN_TL) {
				current_message.data.gamepad_set.element = GAMEPAD_BTN_L1;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == BTN_THUMBR) {
				current_message.data.gamepad_set.element = GAMEPAD_BTN_R3;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == BTN_THUMBL) {
				current_message.data.gamepad_set.element = GAMEPAD_BTN_L3;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == BTN_MODE) {
				current_message.type = GAMEPAD_ACTION;
				current_message.data.action = GAMEPAD_ACTION_PRESS_AND_RELEASE_CENTER;
			}

			if (written_msg == (messages_len-1)) {
				return -ENOMEM;
			}

			messages[written_msg++] = current_message;
		} else if (coll->ev[i].type == EV_ABS) {
			in_message_t current_message = {
				.type = GAMEPAD_SET_ELEMENT,
			};
			
			if (coll->ev[i].code == ABS_X) {
				current_message.data.gamepad_set.element = GAMEPAD_LEFT_JOYSTICK_X;
				current_message.data.gamepad_set.status.joystick_pos = (int32_t)coll->ev[i].value;
			} else if (coll->ev[i].code == ABS_Y) {
				current_message.data.gamepad_set.element = GAMEPAD_LEFT_JOYSTICK_Y;
				current_message.data.gamepad_set.status.joystick_pos = (int32_t)coll->ev[i].value;
			} else if (coll->ev[i].code == ABS_RX) {
				current_message.data.gamepad_set.element = GAMEPAD_RIGHT_JOYSTICK_X;
				current_message.data.gamepad_set.status.joystick_pos = (int32_t)coll->ev[i].value;
			} else if (coll->ev[i].code == ABS_RY) {
				current_message.data.gamepad_set.element = GAMEPAD_RIGHT_JOYSTICK_Y;
				current_message.data.gamepad_set.status.joystick_pos = (int32_t)coll->ev[i].value;
			} else if (coll->ev[i].code == ABS_Z) {
				current_message.data.gamepad_set.element = GAMEPAD_BTN_L2_TRIGGER;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == ABS_RZ) {
				current_message.data.gamepad_set.element = GAMEPAD_BTN_R2_TRIGGER;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == ABS_HAT0X) {
				current_message.data.gamepad_set.element = GAMEPAD_DPAD_X;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			} else if (coll->ev[i].code == ABS_HAT0Y) {
				current_message.data.gamepad_set.element = GAMEPAD_DPAD_Y;
				current_message.data.gamepad_set.status.btn = coll->ev[i].value;
			}

			if (written_msg == (messages_len-1)) {
				return -ENOMEM;
			}

			messages[written_msg++] = current_message;
		}
	}

	return written_msg;
}
