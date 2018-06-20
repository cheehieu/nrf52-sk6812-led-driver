/**
 * Copyright (c) 2014 - 2018, Nordic Semiconductor ASA
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
 *
 * @defgroup ble_sdk_uart_over_ble_main main.c
 * @{
 * @ingroup  ble_sdk_app_nus_eval
 * @brief    UART over BLE application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service.
 * This application uses the @ref srvlib_conn_params module.
 */


#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "nrf_pwr_mgmt.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

#define DEVICE_NAME                     "Nordic_UART"                               /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */

#define APP_ADV_DURATION                18000                                       /**< The advertising duration (180 seconds) in units of 10 milliseconds. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */


BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */

static uint16_t   m_conn_handle          = BLE_CONN_HANDLE_INVALID;                 /**< Handle of the current connection. */
static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t m_adv_uuids[]          =                                          /**< Universally unique service identifier. */
{
    {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};
    
    
#include "nrf_drv_i2s.h"
#include "nrf_delay.h"

#define LED_DIN_PIN		    2
#define LED_LRCK_PIN		    11
#define LED_SCK_PIN		    18
#define NUM_LEDS		    21	//9
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
    config.sck_pin	= LED_SCK_PIN;	// Don't set NRF_DRV_I2S_PIN_NOT_USED for I2S_CONFIG_SCK_PIN. (The program will stack.) 
    config.lrck_pin	= NRF_DRV_I2S_PIN_NOT_USED; //LED_LRCK_PIN; // I2S_CONFIG_LRCK_PIN
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


/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for initializing the timer module.
 */
static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_evt_t * p_evt)
{ 
    if (p_evt->type == BLE_NUS_EVT_RX_DATA)
    {
        uint32_t err_code;
	uint8_t * p_data = p_evt->params.rx_data.p_data;
	uint16_t length = p_evt->params.rx_data.length;

        NRF_LOG_DEBUG("Received data from BLE NUS. Writing data on UART.");
        NRF_LOG_HEXDUMP_DEBUG(p_data, length);
	
	// Ported LED paint animations from Adafruit's Neopixel library
	// https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest/strandtest.ino
	if (p_data[0] == '0') {
	    clear_leds();
	} 
	else if (p_data[0] == '1') {
	    color_wipe(255, 0, 0, 50); // Red
	    color_wipe(0, 255, 0, 50); // Green
	    color_wipe(0, 0, 255, 50); // Blue
	    clear_leds();
	} 
	else if (p_data[0] == '2') {
	    theater_chase(127, 127, 127, 50); // White
	    theater_chase(127, 0, 0, 50); // Red
	    theater_chase(0, 0, 127, 50); // Blue
	    clear_leds();
	} 
	else if (p_data[0] == '3') {
	    rainbow(20);
	    clear_leds();
	} 
	else if (p_data[0] == '4') {
	    rainbow_cycle(20);
	    clear_leds();
	} 
	else if (p_data[0] == '5') {
	    theater_chase_rainbow(50);
	    clear_leds();
	}

        for (uint32_t i = 0; i < length; i++)
        {
            do
            {
                err_code = app_uart_put(p_data[i]);
                if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
                {
                    NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
                    APP_ERROR_CHECK(err_code);
                }
            } while (err_code == NRF_ERROR_BUSY);
        }
        if (p_data[length - 1] == '\r')
        {
            while (app_uart_put('\n') == NRF_ERROR_BUSY);
        }
    }

}
/**@snippet [Handling the data received over BLE] */


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t           err_code;
    ble_nus_init_t     nus_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize NUS.
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;
        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint32_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            // LED indication will be changed when advertising starts.
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}


/**@brief Function for initializing the GATT library. */
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}


/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a string. The string will be be sent over BLE when the last character received was a
 *          'new line' '\n' (hex 0x0A) or if the string has reached the maximum data length.
 */
/**@snippet [Handling the data received over UART] */
void uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
    static uint8_t index = 0;
    uint32_t       err_code;

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            UNUSED_VARIABLE(app_uart_get(&data_array[index]));
            index++;

            if ((data_array[index - 1] == '\n') || (index >= (m_ble_nus_max_data_len)))
            {
                NRF_LOG_DEBUG("Ready to send data over BLE NUS");
                NRF_LOG_HEXDUMP_DEBUG(data_array, index);

                do
                {
                    uint16_t length = (uint16_t)index;
                    err_code = ble_nus_data_send(&m_nus, data_array, &length, m_conn_handle);
                    if ( (err_code != NRF_ERROR_INVALID_STATE) && (err_code != NRF_ERROR_BUSY) &&
                         (err_code != NRF_ERROR_NOT_FOUND) )
                    {
                        APP_ERROR_CHECK(err_code);
                    }
                } while (err_code == NRF_ERROR_BUSY);

                index = 0;
            }
            break;

        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}
/**@snippet [Handling the data received over UART] */


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
static void uart_init(void)
{
    uint32_t                     err_code;
    app_uart_comm_params_t const comm_params =
    {
        .rx_pin_no    = RX_PIN_NUMBER,
        .tx_pin_no    = TX_PIN_NUMBER,
        .rts_pin_no   = RTS_PIN_NUMBER,
        .cts_pin_no   = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
#if defined (UART_PRESENT)
        .baud_rate    = NRF_UART_BAUDRATE_115200
#else
        .baud_rate    = NRF_UARTE_BAUDRATE_115200
#endif
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);
    APP_ERROR_CHECK(err_code);
}
/**@snippet [UART Initialization] */


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t               err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    bsp_event_t startup_event;

    uint32_t err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
    nrf_pwr_mgmt_run();
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}


/**@brief Application main function.
 */
int main(void)
{
    bool erase_bonds;

    // Initialize.
    uart_init();
    log_init();
    timers_init();
    buttons_leds_init(&erase_bonds);
    power_management_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
    advertising_init();
    conn_params_init();

    // Start execution.
    printf("\r\nUART started.\r\n");
    NRF_LOG_INFO("Debug logging for UART over RTT started.");
    advertising_start();
    clear_leds();

    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }
}


/**
 * @}
 */
