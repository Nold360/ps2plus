#include "test_commands.h"

const uint8_t COMMAND_ID = 0x44;
const size_t PAYLOAD_SIZE = 6;

const uint8_t PAYLOAD_SET_DIGITAL[PAYLOAD_SIZE] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t PAYLOAD_SET_ANALOG[PAYLOAD_SIZE] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t PAYLOAD_SET_ANALOG_WITH_LOCK[PAYLOAD_SIZE] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x00 };

void test_switch_digital_analog_mode_command_set_digital() {
  state.analog_mode = CMAnalog;
  state.analog_mode_locked = false;

  uint8_t *actual_output = helper_run_command(&state, COMMAND_ID, PAYLOAD_SET_DIGITAL, PAYLOAD_SIZE);
  TEST_ASSERT_EACH_EQUAL_HEX8(0, actual_output, PAYLOAD_SIZE);
  TEST_ASSERT_EQUAL_MESSAGE(CMDigital, state.analog_mode, "Controller should be in digital mode");
  TEST_ASSERT_FALSE_MESSAGE(state.analog_mode_locked, "Controller should not have analog lock enabled");
}

void test_switch_digital_analog_mode_command_set_analog() {
  state.analog_mode = CMDigital;
  state.analog_mode_locked = false;

  uint8_t *actual_output = helper_run_command(&state, COMMAND_ID, PAYLOAD_SET_ANALOG, PAYLOAD_SIZE);
  TEST_ASSERT_EACH_EQUAL_HEX8(0, actual_output, PAYLOAD_SIZE);
  TEST_ASSERT_EQUAL_MESSAGE(CMAnalog, state.analog_mode, "Controller should be in analog mode");
  TEST_ASSERT_FALSE_MESSAGE(state.analog_mode_locked, "Controller should not have analog lock enabled");
}

void test_switch_digital_analog_mode_command_set_analog_with_lock() {
  state.analog_mode = CMDigital;
  state.analog_mode_locked = false;

  uint8_t *actual_output = helper_run_command(&state, COMMAND_ID, PAYLOAD_SET_ANALOG_WITH_LOCK, PAYLOAD_SIZE);
  TEST_ASSERT_EACH_EQUAL_HEX8(0, actual_output, PAYLOAD_SIZE);
  TEST_ASSERT_EQUAL_MESSAGE(CMAnalog, state.analog_mode, "Controller should be in analog mode");
  TEST_ASSERT_TRUE_MESSAGE(state.analog_mode_locked, "Controller should have analog lock enabled");
}

void test_switch_digital_analog_mode_command_toggling_removes_pressures() {
  // Starting from full analog with pressures, toggling twice should return to 
  // analog mode but with pressure data no longer enabled
  state.analog_mode = CMAnalogFull;
  state.analog_mode_locked = false;

  uint8_t *first_output = helper_run_command(&state, COMMAND_ID, PAYLOAD_SET_DIGITAL, PAYLOAD_SIZE);
  uint8_t *second_output = helper_run_command(&state, COMMAND_ID, PAYLOAD_SET_ANALOG, PAYLOAD_SIZE);
  TEST_ASSERT_EACH_EQUAL_HEX8(0, first_output, PAYLOAD_SIZE);
  TEST_ASSERT_EACH_EQUAL_HEX8(0, second_output, PAYLOAD_SIZE);
  TEST_ASSERT_EQUAL_MESSAGE(CMAnalog, state.analog_mode, "Controller should be in analog mode");
}