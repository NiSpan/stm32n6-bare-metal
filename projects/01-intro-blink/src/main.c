#include "stm32n6xx.h"
#include <stdint.h>

int counter = 0;

int main(void) {
    /*
     * RCC is a struct with base address 0x56028000
     * If we look at its members in stm32n657xx.h,
     * AHB4ENR is on the 152 position of the struct.
     * Each member is uint32, meaning 4 bytes.
     * So 151 x 4 = 604 or 0x025C in HEX
     *
     * We assign the value to enable GPIOG
     * */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOGEN;

    /*
     * Now we have to make the pins 0, 8, 10
     * as outputs.
     *
     * GPIOG is a struct with base address 0x56021800.
     * The MODER member sits at first element, meaning 0x00
     * It is the 32bit register we need to set the pins as
     * general outputs (01 mode)
     *
     * We will set the correct value by:
     * 1. Reading the initial register value
     * 2. Clearing the bits for pins 0, 8, 10 using the
     *    masks from stm32n657xx.h
     * 3. Set the bits to 01 mode, using the macros from same header
     * 4. Assign the value back to register
     * */
    
    // Get initial value
    uint32_t moder = GPIOG->MODER;
    
    // 1. Invert so we make 1 to zero in the right positions for pins 0, 8, 10
    // 2. OR between them so we have a 32 bit value of 1s, and 0 only in the pin
    // positions
    // 3. AND with initial value. So only the 0s clear what value it had before
    moder &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE8 | GPIO_MODER_MODE10);
    
    // Now that we have a cleared initial value, we set only the bits of
    // pins 0, 8, 10 to mode 01
    moder |= (GPIO_MODER_MODE0_0 |GPIO_MODER_MODE8_0 |GPIO_MODER_MODE10_0 );

    // Assign the value to hardware register
    GPIOG->MODER = moder;

    // Like before, to turn the LEDs on and off
    // We have to construct the value to write
    // To the BSRR register.
    uint32_t LEDS_OFF = (GPIO_BSRR_BS0 | GPIO_BSRR_BS8 | GPIO_BSRR_BS10);
    uint32_t LEDS_ON = (GPIO_BSRR_BR0 | GPIO_BSRR_BR8 | GPIO_BSRR_BR10);

    while (1) {
        // Turn off the LED pins
        GPIOG->BSRR = LEDS_OFF;
        while(counter < 50000){
            counter++;
        }
        counter = 0;
        // Turn on the LED Pins
        GPIOG->BSRR = LEDS_ON;
        while(counter < 50000){
            counter++;
        }
        counter = 0;
    }

    return 0;
}
