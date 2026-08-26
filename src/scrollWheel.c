#include "scrollWheel.h"


const struct gpio_dt_spec buttonPin1 = GPIO_DT_SPEC_GET(DT_NODELABEL(scroll_a), gpios);
const struct gpio_dt_spec buttonPin2 = GPIO_DT_SPEC_GET(DT_NODELABEL(scroll_b), gpios);


volatile int Input1Val = 0;
volatile int Input2Val = 0;

volatile int Input1Prev = 0;
volatile int Input2Prev = 0;

volatile int prevScrollCount = 0;
volatile int scrollCount = 0;

void button_pressed_1(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {

	Input1Val = gpio_pin_get_dt(&buttonPin1);

}

void button_pressed_2(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {

	Input2Val = gpio_pin_get_dt(&buttonPin2);

}

void scroll_wheel_init() {
    if(!gpio_is_ready_dt(&buttonPin1)) {
		return;

	}
	if(!gpio_is_ready_dt(&buttonPin2)) {
		return;
		
	}

	gpio_pin_configure_dt(&buttonPin1, GPIO_INPUT | GPIO_PULL_UP);
	gpio_pin_interrupt_configure_dt(&buttonPin1,
					      GPIO_INT_EDGE_BOTH);

	gpio_pin_configure_dt(&buttonPin2, GPIO_INPUT | GPIO_PULL_UP);
	gpio_pin_interrupt_configure_dt(&buttonPin2,
					      GPIO_INT_EDGE_BOTH);

	gpio_init_callback(&button_cb_data_1, button_pressed_1, BIT(buttonPin1.pin));
	gpio_add_callback(buttonPin1.port, &button_cb_data_1);

	gpio_init_callback(&button_cb_data_2, button_pressed_2, BIT(buttonPin2.pin));
	gpio_add_callback(buttonPin2.port, &button_cb_data_2);


}

void getScrollUpdate(int* cnt) {
    int val = Input1Val;
	int val2 = Input2Val;
	if(val != Input1Prev || val2 != Input2Prev) {


    	if(Input1Prev == 0 && Input2Prev == 0 && val == 0 && val2 == 1) {
      		scrollCount--;
    	}
    
    	else if(Input1Prev == 0 && Input2Prev == 1 && val == 0 && val2 == 0) {
    	    scrollCount++;
    	}
    
    	else if(Input1Prev == 1 && Input2Prev == 0 && val == 1 && val2 == 1) {
    	    scrollCount++;
    	}
    
    	else if(Input1Prev == 1 && Input2Prev == 1 && val == 1 && val2 == 0) {
    	    scrollCount--;
    	}

    	if(scrollCount != prevScrollCount) {
    	   *cnt = scrollCount - prevScrollCount;
		  
    	}
        else {
           *cnt = 0;
        }

    	Input1Prev = val;
    	Input2Prev = val2;

    	prevScrollCount = scrollCount;


		
  	}


}