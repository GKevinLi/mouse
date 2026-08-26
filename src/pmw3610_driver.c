#include "pmw3610_driver.h"


const struct gpio_dt_spec cs_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(cs), gpios);
const struct gpio_dt_spec motion_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(motion), gpios);

const struct gpio_dt_spec sdio_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(sdio), gpios);
const struct gpio_dt_spec sclk_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(sclk), gpios);

uint8_t bitbang_read(int addr) {
	uint8_t result = 0;

	
	gpio_pin_configure_dt(&sdio_pin, GPIO_OUTPUT);

	for(int i = 7; i >= 0; i--) {
		gpio_pin_set_dt(&sclk_pin, 0);
		gpio_pin_set_dt(&sdio_pin, (addr >> i) & 0x01);
		k_busy_wait(2);
		gpio_pin_set_dt(&sclk_pin, 1);
		k_busy_wait(2);
	}

	gpio_pin_configure_dt(&sdio_pin, GPIO_INPUT);
	k_busy_wait(10);

	for(int i = 7; i >= 0; i--) {
		gpio_pin_set_dt(&sclk_pin, 0);
		k_busy_wait(2);
		gpio_pin_set_dt(&sclk_pin, 1);
		k_busy_wait(1);
		result |= (gpio_pin_get_dt(&sdio_pin) << i);
		k_busy_wait(1);
	}

	
	k_busy_wait(2);
	gpio_pin_configure_dt(&sdio_pin, GPIO_OUTPUT_INACTIVE);

	k_busy_wait(20);
	
	return result;

}

void bitbang_write(int addr, int value) {

	addr |= 0x80;

	
	gpio_pin_configure_dt(&sdio_pin, GPIO_OUTPUT);

	for(int i = 7; i >= 0; i--) {
		gpio_pin_set_dt(&sclk_pin, 0);
		gpio_pin_set_dt(&sdio_pin, (addr >> i) & 0x01);
		k_busy_wait(2);
		gpio_pin_set_dt(&sclk_pin, 1);
		k_busy_wait(2);
	}
	

	for(int i = 7; i >= 0; i--) {
		gpio_pin_set_dt(&sclk_pin, 0);
		gpio_pin_set_dt(&sdio_pin, (value >> i) & 0x01);
		k_busy_wait(2);
		gpio_pin_set_dt(&sclk_pin, 1);
		k_busy_wait(2);
	}

}

void pmw3610_spi_on() {

	int addr = 0xC1;
	int value = 0xBA;

	
	gpio_pin_configure_dt(&sdio_pin, GPIO_OUTPUT);

	for(int i = 7; i >= 0; i--) {
		gpio_pin_set_dt(&sclk_pin, 0);
		gpio_pin_set_dt(&sdio_pin, (addr >> i) & 0x01);
		k_busy_wait(2);
		gpio_pin_set_dt(&sclk_pin, 1);
		k_busy_wait(2);
	}
	

	for(int i = 7; i >= 0; i--) {
		gpio_pin_set_dt(&sclk_pin, 0);
		gpio_pin_set_dt(&sdio_pin, (value >> i) & 0x01);
		k_busy_wait(2);
		gpio_pin_set_dt(&sclk_pin, 1);
		k_busy_wait(2);
	}

}

void pmw3610_spi_off() {

	int addr = 0xC1;
	int value = 0xB5;

	
	gpio_pin_configure_dt(&sdio_pin, GPIO_OUTPUT);

	for(int i = 7; i >= 0; i--) {
		gpio_pin_set_dt(&sclk_pin, 0);
		gpio_pin_set_dt(&sdio_pin, (addr >> i) & 0x01);
		k_busy_wait(2);
		gpio_pin_set_dt(&sclk_pin, 1);
		k_busy_wait(2);
	}
	

	for(int i = 7; i >= 0; i--) {
		gpio_pin_set_dt(&sclk_pin, 0);
		gpio_pin_set_dt(&sdio_pin, (value >> i) & 0x01);
		k_busy_wait(2);
		gpio_pin_set_dt(&sclk_pin, 1);
		k_busy_wait(2);
	}

}

void pmw3610_change_page() {

	int registerVal = bitbang_read(0x7f);
	if(registerVal == 0x00) {
		bitbang_write(0x7f, 0xFF);
	}
	else if(registerVal = 0xFF) {
		bitbang_write(0x7f, 0x00);
	}

}



void pmw3610_change_cpi(int cpi_value) {
	if(cpi_value > 3200) {
		cpi_value = 3200;
	}

	cpi_value = cpi_value / 200;

	uint8_t prev_value = bitbang_read(0x85);
	prev_value = (prev_value >> 5) << 5;

	pmw3610_spi_on();
	pmw3610_change_page();

	uint8_t write_value = (prev_value | cpi_value);
	bitbang_write(0x85, write_value);

	pmw3610_change_page();
	pmw3610_spi_off();


}

int pmw3610_get_cpi() {

	uint8_t cpi_value_raw = bitbang_read(0x85);
	int cpi_value = (cpi_value_raw & 0x1F) * 200;
	return cpi_value;

}




void bitbang_powerup() {

	/*II. Drive NCS high, and then low to reset the SPI port.
III. Wait for at least one frame (150us).
IV. Write register 0x2d with value 0x00 (clear observation1 register for sensor self test check).
V. Wait for 10ms, after that read register 0x2d again. Make sure all the bit [3-0] must be set to 1.
VI. Read from registers 0x02, 0x03, 0x04 and 0x05 one time regardless of the motion pin state.
VII. Write register 0x11 with value 0x0d, required setting to configure.
VIII. Write register 0x1b with value 0x04, required setting to configure.
IX. Write register 0x1c with value 0x04, required setting to configure.
X. Write register 0x1d with value 0x0f, required setting to configure
	*/

	gpio_pin_configure_dt(&cs_pin, GPIO_OUTPUT);

	gpio_pin_set_dt(&cs_pin, 1);

	k_busy_wait(150);

	bitbang_write(0x41, 0xBA);
	k_busy_wait(300);
	bitbang_write(0x2d, 0x00);
	k_busy_wait(300);
	bitbang_write(0x41, 0xB5);

	k_msleep(10);

	uint8_t result = bitbang_read(0x2d);
	k_busy_wait(40);

	bitbang_read(0x02);
	k_busy_wait(40);

	bitbang_read(0x03);
	k_busy_wait(40);

	bitbang_read(0x04);
	k_busy_wait(40);

	bitbang_read(0x05);
	k_busy_wait(40);

	bitbang_write(0x41, 0xBA);
	k_busy_wait(300);
	bitbang_write(0x11, 0x0d);
	k_busy_wait(300);
	bitbang_write(0x1b, 0x04);
	k_busy_wait(300);
	bitbang_write(0x1c, 0x04);
	k_busy_wait(300);
	bitbang_write(0x1d, 0x0f);
	k_busy_wait(300);
	bitbang_write(0x41, 0xB5);


}

void getXYMovement_bitbang(int* x, int* y) {
	uint8_t xMovementLower = bitbang_read(0x03);
	uint8_t yMovementLower = bitbang_read(0x04);
	uint8_t xyMovementUpper = bitbang_read(0x05);

	int xMovementUpper = xyMovementUpper >> 4;
	int xUpperBit = xyMovementUpper >> 7;
	xMovementUpper = xMovementUpper & 0b111;

	int yMovementUpper = xyMovementUpper & 0xF;
	int yUpperBit = yMovementUpper >> 3;
	yMovementUpper = yMovementUpper & 0b111;




	int xProcessed = xMovementLower | (xMovementUpper << 8);
	int yProcessed =  yMovementLower | (yMovementUpper << 8);

	if(xUpperBit) {
		xProcessed -= 2048;
	}
	if(yUpperBit) {
		yProcessed -= 2048;
	}




	*x = xProcessed;
	*y = yProcessed;

}





void pmw3610_init() {

    

    if (!gpio_is_ready_dt(&cs_pin)) {
    	return;
	}
    if (!gpio_is_ready_dt(&sdio_pin)) {
    	return;
	}
    if (!gpio_is_ready_dt(&sclk_pin)) {
    	return;
	}
	if (!gpio_is_ready_dt(&motion_pin)) {
    	return;
	}

	gpio_pin_configure_dt(&motion_pin, GPIO_INPUT);
	gpio_pin_configure_dt(&cs_pin, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&sclk_pin, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&sdio_pin, GPIO_OUTPUT_INACTIVE);

	gpio_pin_set_dt(&sclk_pin, 1);
	gpio_pin_set_dt(&sdio_pin, 0);
	gpio_pin_set_dt(&cs_pin, 0);
	
	k_msleep(20); 

	bitbang_powerup();
	

}




