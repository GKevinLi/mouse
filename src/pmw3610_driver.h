
#ifndef PMW3610_DRIVER_H
#define PMW3610_DRIVER_H

#pragma once

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#define PMW3610_PROD_ID		0x00
#define PMW3610_REV_ID		0x01
#define PMW3610_MOTION		0x02
#define PMW3610_DELTA_X_L	0x03
#define PMW3610_DELTA_Y_L	0x04
#define PMW3610_DELTA_XY_H	0x05
#define PMW3610_PERFORMANCE	0x11
#define PMW3610_BURST_READ	0x12
#define PMW3610_RUN_DOWNSHIFT	0x1b
#define PMW3610_REST1_RATE	0x1c
#define PMW3610_REST1_DOWNSHIFT	0x1d
#define PMW3610_OBSERVATION1	0x2d
#define PMW3610_SMART_MODE	0x32
#define PMW3610_POWER_UP_RESET	0x3a
#define PMW3610_SHUTDOWN	0x3b
#define PMW3610_SPI_CLK_ON_REQ	0x41
#define PWM3610_SPI_PAGE0	0x7f

#define PMW3610_CRC0  0x0c
#define PMW3610_CRC1  0x0d
#define PMW3610_CRC2  0x0e
#define PMW3610_CRC3  0x0f

#define SPIOP      SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA 

// bool smart_enable = false;
// bool smart_flag = true;

extern const struct gpio_dt_spec cs_pin;
extern const struct gpio_dt_spec motion_pin;

extern const struct gpio_dt_spec sdio_pin;
extern const struct gpio_dt_spec sclk_pin;

#define MOUSE_MOVEMENT_MULTIPLIER  1

uint8_t bitbang_read(int addr);

void pmw3610_spi_on();
void pmw3610_spi_off();
void bitbang_write(int addr, int value);
void bitbang_powerup();
void getXYMovement_bitbang(int* x, int* y);
void pmw3610_init();

void pmw3610_change_page();
void pmw3610_change_cpi(int cpi_value);
int pmw3610_get_cpi(); 

#endif