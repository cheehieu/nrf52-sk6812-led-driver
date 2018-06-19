/**
 * Copyright (c) 2015 - 2018, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
/** @file
 * @defgroup i2s_example_main main.c
 * @{
 * @ingroup i2s_example
 *
 * @brief SK6812 I2S Example Application main file.
 *
 * This file contains the source code for a sample application using I2S with SK6812 LEDs.
 */

#include <stdio.h>
#include "nrf_drv_i2s.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "boards.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define LED_DIN_PIN		    2
#define LED_CLK_PIN		    11
#define NUM_LEDS		    9
#define DATA_BYTES_PER_LED	    3	// 24-bit GRB data structure
#define LEDS_DATA_BYTE_SIZE	    NUM_LEDS * DATA_BYTES_PER_LED

#define I2S_BITS_PER_DATA_BIT	    4
#define I2S_SK6812_ZERO		    0x8		//0b'1000 (0.3us; 0.9us)
#define I2S_SK6812_ONE		    0xC		//0b'1100 (0.6us; 0.6us)
#define I2S_WS2812B_ZERO	    0x8		//0b'1000 (0.4us; 0.85us)
#define I2S_WS2812B_ONE		    0xE		//0b'1110 (0.8us; 0.45us)
#define I2S_BYTES_PER_RESET	    256/8	//reset_pulse / period = 80us / 0.3125us = 256
#define I2S_LEDS_WORD_SIZE	    BYTES_TO_WORDS(LEDS_DATA_BYTE_SIZE * I2S_BITS_PER_DATA_BIT)
#define I2S_RESET_WORD_SIZE	    BYTES_TO_WORDS(I2S_BYTES_PER_RESET)	//8 words
#define I2S_LEDS_FRAME_WORD_SIZE    I2S_LEDS_WORD_SIZE + I2S_RESET_WORD_SIZE

typedef struct {
    uint8_t g;
    uint8_t r;
    uint8_t b;
} sk6812_led_t;

sk6812_led_t	m_led_buffer_tx[NUM_LEDS];
uint32_t	m_i2s_led_buffer_tx[I2S_LEDS_FRAME_WORD_SIZE];
//sk6812_led_t *p_led_buffer_tx;
//uint32_t *p_i2s_led_buffer_tx;


/**
 * @brief Clears the LED and I2S data buffers 
 */
ret_code_t sk6812_i2s_init_mem() {
// TODO: dynamically allocate memory when needed instead of using global arrays
//    // Init memory for LED data
//    if (p_led_buffer_tx) free(p_led_buffer_tx);
//    p_led_buffer_tx = (sk6812_led_t *)malloc(LEDS_DATA_BYTE_SIZE);
//    if (p_led_buffer_tx) memset(p_led_buffer_tx, 0, LEDS_DATA_BYTE_SIZE);
//    else return NRF_ERROR_NO_MEM;
//    
//    // Init memory for I2S data
//    if (p_i2s_led_buffer_tx) free(p_i2s_led_buffer_tx);
//    p_i2s_led_buffer_tx = (uint32_t *)malloc(I2S_LEDS_WORD_SIZE * 4);
//    if (p_i2s_led_buffer_tx) memset(p_i2s_led_buffer_tx, 0, I2S_LEDS_WORD_SIZE * 4);
//    else return NRF_ERROR_NO_MEM;
    
    // Reset data buffers
    memset(m_led_buffer_tx, 0, sizeof(m_led_buffer_tx));
    memset(m_i2s_led_buffer_tx, 0, sizeof(m_i2s_led_buffer_tx));
    
    return NRF_SUCCESS;
}


/**
 * @brief Stops data transfer after the first full transmission (only using single buffer)
 * @brief Handles transmitted data when receiving the TXPTRUPD and RXPTRUPD events, every RXTXD.MAXCNT number of transmitted data words
 * @brief Addresses written to the pointer registers TXD.PTR and RXD.PTR are double-buffered in hardware, and these double buffers are updated for every RXTXD.MAXCNT words
 */
void data_handler(nrf_drv_i2s_buffers_t const * p_released, uint32_t status) {
    // 'nrf_drv_i2s_next_buffers_set' is called directly from the handler
    // each time next buffers are requested, so data corruption is not expected.
    ASSERT(p_released);

    // When the handler is called after the transfer has been stopped
    // (no next buffers are needed, only the used buffers are to be
    // released), there is nothing to do.
    if (!(status & NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED)) {
        return;
    }

    // Stop data transfer after the first full transmission (only using single buffer)
    if (p_released->p_tx_buffer != NULL) {
//	nrf_drv_i2s_stop();
	nrf_drv_i2s_uninit();
    }
}


/**
 * @brief Initializes the I2S interface for ~3.2MHz clock for SK6812
 */
void sk6812_i2s_init() {
    nrf_drv_i2s_config_t config;     //= NRF_DRV_I2S_DEFAULT_CONFIG;
    config.sck_pin	= NRF_DRV_I2S_PIN_NOT_USED; // I2S_CONFIG_SCK_PIN
    config.lrck_pin	= LED_CLK_PIN;	// I2S_CONFIG_LRCK_PIN
    config.mck_pin	= NRF_DRV_I2S_PIN_NOT_USED;
    config.sdout_pin	= LED_DIN_PIN;	// I2S_CONFIG_SDOUT_PIN
    config.sdin_pin	= NRF_DRV_I2S_PIN_NOT_USED;
    config.irq_priority = I2S_CONFIG_IRQ_PRIORITY;
    config.mode         = NRF_I2S_MODE_MASTER;
    config.format       = NRF_I2S_FORMAT_I2S;
    config.alignment    = NRF_I2S_ALIGN_LEFT;
    config.sample_width = NRF_I2S_SWIDTH_8BIT;
    config.channels     = NRF_I2S_CHANNELS_STEREO;
    config.mck_setup    = NRF_I2S_MCK_32MDIV5;
    config.ratio        = NRF_I2S_RATIO_32X;
    
    uint32_t err_code = nrf_drv_i2s_init(&config, data_handler);
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Convert a byte of LED data to a word of I2S data 
 * @brief 1 data bit   <--> 4 I2S bits
 * @brief 1 data byte  <--> 1 I2S word
 */
uint32_t convert_byte_to_i2s_bits(uint8_t data_byte) {
    uint32_t data_bits;
    
    // Set data_bits based on MSB, then left-shift data_byte
    for (int ii=0; ii < 8; ii++) {
	data_bits |= ((data_byte & 0x80) ? I2S_SK6812_ONE : I2S_SK6812_ZERO) << ((8-1-ii) * 4);
//	data_bits |= ((data_byte & 0x80) ? I2S_WS2812B_ONE : I2S_WS2812B_ZERO) << ((8-1-ii) * 4);
	data_byte = data_byte << 1;
    }
    return data_bits;
}


/**
 * @brief Sets I2S data from converted LED data
 */
void set_i2s_led_data() {
    uint16_t jj = 0;
    for (uint16_t ii=0; ii < NUM_LEDS; ii++) {
	m_i2s_led_buffer_tx[jj] = convert_byte_to_i2s_bits(m_led_buffer_tx[ii].g);
	m_i2s_led_buffer_tx[jj+1] = convert_byte_to_i2s_bits(m_led_buffer_tx[ii].r);
	m_i2s_led_buffer_tx[jj+2] = convert_byte_to_i2s_bits(m_led_buffer_tx[ii].b);
	jj += 3;
    }
}
 

/**
 * @brief Initializes I2S and starts the data transfer
 * @brief Assumes data buffers have already been set
 */
void send_i2s_led_data() {
    // Configure the I2S module and map IO pins
    sk6812_i2s_init();
    
    // Configure TX data buffer
    nrf_drv_i2s_buffers_t const initial_buffers = {
	.p_tx_buffer = m_i2s_led_buffer_tx,
	.p_rx_buffer = NULL
    };
    
    // Enable the I2S module and start data streaming
    uint32_t err_code = nrf_drv_i2s_start(&initial_buffers, I2S_LEDS_FRAME_WORD_SIZE+1, 0);
    APP_ERROR_CHECK(err_code);
}

void clear_leds() {
    // Set to all I2S_SK6812_ZERO's
    memset(m_i2s_led_buffer_tx, 0x88, I2S_LEDS_WORD_SIZE * 4);
    send_i2s_led_data();
}


/**
 * @brief Set an RGB LED color data in the global sk6812_led_t array
 */
void set_led_pixel_RGB(uint16_t pos, uint8_t r, uint8_t g, uint8_t b) {
    m_led_buffer_tx[pos].r = r;
    m_led_buffer_tx[pos].g = g;
    m_led_buffer_tx[pos].b = b;
}


/**
 * @brief Ported Adafruit NeoPixel paint animation functions
 */
// Fill the dots one after the other with a color
void color_wipe(uint8_t r, uint8_t g, uint8_t b, uint8_t ms_delay) {
    sk6812_i2s_init_mem();
    
    for(uint16_t ii=0; ii < NUM_LEDS; ii++) {
	set_led_pixel_RGB(ii, r, g, b);
	set_i2s_led_data();
	send_i2s_led_data();
	nrf_delay_ms(ms_delay);
    }
}

// Input a value 0 to 255 to get a color value
sk6812_led_t wheel(uint8_t wheel_pos) {
    sk6812_led_t color;
    wheel_pos = 255 - wheel_pos;
    
    if(wheel_pos < 85) {
	color.r = 255 - wheel_pos * 3;
	color.g = 0;
	color.b = wheel_pos * 3;
	return color;
    }
    
    if(wheel_pos < 170) {
	wheel_pos -= 85;
	color.r = 0;
	color.g = wheel_pos * 3;
	color.b = 255 - wheel_pos * 3;
	return color;	
    }
    
    wheel_pos -= 170;
    color.r = wheel_pos * 3;
    color.g = 255 - wheel_pos * 3;
    color.b = 0;
    return color;
}

void rainbow(uint16_t ms_delay) {
    uint16_t ii, jj;
    sk6812_led_t color;
    
    sk6812_i2s_init_mem();
    for (jj=0; jj < NUM_LEDS; jj++) {
	for (ii=0; ii < NUM_LEDS; ii++) {
	    color = wheel(((ii * 256 / NUM_LEDS) + jj) & 255);
	    set_led_pixel_RGB(ii, color.r, color.g, color.b);
	}
	set_i2s_led_data();
	send_i2s_led_data();
	nrf_delay_ms(ms_delay);
    }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbow_cycle(uint16_t ms_delay) {
    uint16_t ii, jj;
    sk6812_led_t color;
    
    sk6812_i2s_init_mem();
    for(jj=0; jj < 256*3; jj++) { // 3 cycles of all colors on wheel
	for(ii=0; ii < NUM_LEDS; ii++) {
	    color = wheel(((ii * 256 / NUM_LEDS) + jj) & 255);
	    set_led_pixel_RGB(ii, color.r, color.g, color.b);
	}
	set_i2s_led_data();
	send_i2s_led_data();
	nrf_delay_ms(ms_delay);
    }
}

// Theater-style crawling lights.
void theater_chase(uint8_t r, uint8_t g, uint8_t b, uint16_t ms_delay) {
    sk6812_i2s_init_mem();
    for (int jj=0; jj < 10; jj++) {  //do 10 cycles of chasing
	for (int qq=0; qq < 3; qq++) {
	    for (uint16_t ii=0; ii < NUM_LEDS; ii=ii+3) {
		set_led_pixel_RGB(ii+qq, r, g, b);	//turn every third pixel on
	    }
	    set_i2s_led_data();
	    send_i2s_led_data();

	    nrf_delay_ms(ms_delay);

	    for (uint16_t ii=0; ii < NUM_LEDS; ii=ii+3) {
		set_led_pixel_RGB(ii+qq, 0, 0, 0);	//turn every third pixel off
	    }
	}
    }
}

// Theater-style crawling lights with rainbow effect
void theater_chase_rainbow(uint16_t ms_delay) {
    sk6812_led_t color;
    sk6812_i2s_init_mem();
    for (int jj=0; jj < 256; jj++) {     // cycle all 256 colors in the wheel
	for (int qq=0; qq < 3; qq++) {
	    for (uint16_t ii=0; ii < NUM_LEDS; ii=ii+3) {
		color = wheel((ii+jj) % 255);
		set_led_pixel_RGB(ii+qq, color.r, color.g, color.b);   //turn every third pixel on
	    }
	    set_i2s_led_data();
	    send_i2s_led_data();

	    nrf_delay_ms(ms_delay);

	    for (uint16_t ii=0; ii < NUM_LEDS; ii=ii+3) {
		set_led_pixel_RGB(ii+qq, 0, 0, 0);	//turn every third pixel off
	    }
	}
    }
}


int main(void) {
    uint32_t err_code = NRF_SUCCESS;
    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    NRF_LOG_INFO("SK6812 I2S example started.");
    NRF_LOG_FLUSH();

    // Ported LED paint animations from Adafruit's Neopixel library
    // https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest/strandtest.ino
    
    // Some example procedures showing how to display to the pixels:
    color_wipe(255, 0, 0, 50); // Red
    color_wipe(0, 255, 0, 50); // Green
    color_wipe(0, 0, 255, 50); // Blue    
    
    // Send a theater pixel chase in...
    theater_chase(127, 127, 127, 50); // White
    theater_chase(127, 0, 0, 50); // Red
    theater_chase(0, 0, 127, 50); // Blue

    rainbow(20);
    rainbow_cycle(20);
    theater_chase_rainbow(50);

    clear_leds();
}

/** @} */
