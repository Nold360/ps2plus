#ifndef CONTROLLER_STATE_H
#define CONTROLLER_STATE_H

#include "bootloader.h"
#include "custom_config.h"
#include "input.h"

typedef enum controller_analog_mode {
  CMDigital,    // digital buttons only (2 bytes of data)
  CMAnalog,     // digital buttons, analog joysticks (6 bytes of data)
  CMAnalogFull, // digital buttons, analog joysticks, button pressures (18 bytes of data)
} controller_analog_mode;

typedef struct {
  uint8_t mapping;
  uint8_t value;
} controller_rumble_motor;

typedef struct {
  struct {
#if defined(PS2PLUS_FIRMWARE)
    uint16_t firmware;
    char microcontroller[32];
    uint16_t configuration;
#elif defined(PS2PLUS_BOOTLOADER)
    uint16_t bootloader;
#endif
  } versions;
  
  /**
   * @brief Current button and joystick input
   */
  controller_input input;

  /**
   * @brief Current digital/analog mode of the controller
   */
  controller_analog_mode analog_mode;

  /**
   * @brief If true, the analog mode of the controller has been locked by 
   *        the console and cannot be switched by the user
   */
  bool analog_mode_locked;

  /**
   * @brief Whether the controller has entered configuration mode (F3h)
   */
  bool config_mode;

  /**
   * @brief Mode identifier combining analog and config statuses
   */
  uint8_t mode;

  /**
   * @brief Rumble motor information for the small motor (FFh = on)
   */
  controller_rumble_motor rumble_motor_small;

  /**
   * @brief Rumble motor information for the large motor (40h+ = on)
   */
  controller_rumble_motor rumble_motor_large;

#ifdef PS2PLUS_FIRMWARE
  /**
   * @brief Custom configuration information, persisted in internal storage
   */
  controller_custom_config custom_config;
#endif
  
#ifdef PS2PLUS_BOOTLOADER
  /**
   * @brief State information required for the operation of the bootloader
   */
  controller_bootloader bootloader;
#endif
} controller_state;

void controller_state_initialize(controller_state *);
void controller_state_update_mode(controller_state *);
void controller_state_set_versions(controller_state *, uint16_t firmware, char microcontroller[32], uint16_t configuration, uint16_t bootloader);
void controller_state_reset(controller_state *);

#endif /* CONTROLLER_STATE_H */