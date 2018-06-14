# nrf52-sk6812-led-driver
Based on Nordic's SDK15, this code example uses the nRF52's I2S interface to communicate with SK6812 single-wire-based LED pixels.

# Overview
- Differences to WS2812B "Neopixels"
- Advantages over SPI, PWM
- More flexible transmission rates (3.2MHz)
- Larger EasyDMA max buffer size

# Code
```
// NEO_GRB indicates a NeoPixel-compatible
// device expecting three bytes per pixel, with the first byte
// containing the green value, second containing red and third
// containing blue.

// This technique uses the PWM peripheral on the NRF52. The PWM uses the
// EasyDMA feature included on the chip. This technique loads the duty 
// cycle configuration for each cycle when the PWM is enabled. For this 
// to work we need to store a 16 bit configuration for each bit of the
// RGB(W) values in the pixel buffer.

// To support both the SoftDevice + Neopixels we use the EasyDMA
// feature from the NRF25. However this technique implies to
// generate a pattern and store it on the memory. The actual
// memory used in bytes corresponds to the following formula:
//              totalMem = numBytes*8*2+(2*2)
// The two additional bytes at the end are needed to reset the
// sequence.

// The Neopixel implementation is a blocking algorithm. DMA
// allows for non-blocking operation.
```

## I2S Configuration

## Data Preparation

## I2S Control

## Waveforms


# Results


# Resources
- http://takafuminaka.blogspot.com/2016/02/nrf52832-ws2812b-5-i2s.html
- http://electronut.in/nrf52-i2s-ws2812/
- https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fi2s.html&cp=2_1_0_43&anchor=concept_z2v_24y_vr
- https://github.com/adafruit/Adafruit_NeoPixel
- https://learn.adafruit.com/adafruit-neopixel-uberguide?view=all
