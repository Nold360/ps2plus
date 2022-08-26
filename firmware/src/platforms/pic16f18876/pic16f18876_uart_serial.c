#ifdef PLATFORM_PIC16F18876

#include "pic16f18876_platform.h"

#define PIC_UART_PPS_RX_REGISTER U1RXPPS // Table 17-1 in PIC16F18876 data sheet
#define PIC_UART_PPS_TX_CHANNEL 0b010011 // Table 17-2 in PIC16F18876 data sheet

const pic_digital_io_pin PIN_UART_TX = PIC_DIGITAL_IO_PIN(B, 6);
const pic_digital_io_pin PIN_UART_RX = PIC_DIGITAL_IO_PIN(B, 7);

// See the "How Do I Use Printf to Send Text to a Peripheral?" section of the XC8 user guide
// https://onlinedocs.microchip.com/pr/GUID-BB433107-FD4E-4D28-BB58-9D4A58955B1A-en-US-3/index.html

/**
 * Implements the XC8 getch function to bridge stdin to the UART RX port
 */
char getch(void) {
  while(!PIR3bits.RCIF);
  return RCREG;
}

/**
 * Implements the XC8 putch function to brdige stdout to the UART TX port
 * @param value
 */
void putch(char value) {
  while(!PIR3bits.RCTX);
  TXREG = value;
}

void pic16f18876_setup_uart_serial() {
  // Set pin modes for the UART TX and RX pins
  pic_digital_io_pin_mode(&PIN_UART_TX, PICPinMode_Output);
  pic_digital_io_pin_mode(&PIN_UART_RX, PICPinMode_Input);

  // Configure PPS (Peripheral Pin Select) to remap the UART1 TX/RX pins
  pic_digital_io_pin_pps_output(&PIN_UART_TX, PIC_UART_PPS_TX_CHANNEL);
  pic_digital_io_pin_pps_input(&PIN_UART_RX, &PIC_UART_PPS_RX_REGISTER);

  // Enable UART serial communication with a baud rate of 57600
  // See section 31.17 - UART Baud Rate Generator of the PIC16F18876 data sheet
  SP1BRGL = _XTAL_FREQ / 57600 / 16;
  TX1STAbits.TXEN = 1;
  TX1STAbits.RXEN = 1;
  RC1STABits.SPON = 1;
}

#endif /* PLATFORM_PIC16F18876 */
