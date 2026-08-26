#ifndef SCROLLWHEEL
#define SCROLLWHEEL

#pragma once

extern const struct gpio_dt_spec buttonPin1;
extern const struct gpio_dt_spec buttonPin2;

static struct gpio_callback button_cb_data_1;
static struct gpio_callback button_cb_data_2;


#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>


void button_pressed_1(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button_pressed_2(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void scroll_wheel_init();
void getScrollUpdate(int* cnt);



#endif