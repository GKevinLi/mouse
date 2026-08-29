/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */



/*
future changelog

- check to see if moving out of LDO mode breaks the code
- figure out how to send custom button on hid reportmap


*/

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <assert.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/bluetooth/services/bas.h>
#include <bluetooth/services/hids.h>
#include <zephyr/bluetooth/services/dis.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include "pmw3610_driver.h"
#include "scrollWheel.h"

LOG_MODULE_REGISTER(MOUSLOG,LOG_LEVEL_DBG);

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)




#define BASE_USB_HID_SPEC_VERSION   0x0101

/* Number of pixels by which the cursor is moved when a button is pushed. */
#define MOVEMENT_SPEED              5
/* Number of input reports in this application. */
#define INPUT_REPORT_COUNT          3
/* Length of Mouse Input Report containing button data. */
#define INPUT_REP_BUTTONS_LEN       3
/* Length of Mouse Input Report containing movement data. */
#define INPUT_REP_MOVEMENT_LEN      3
/* Length of Mouse Input Report containing media player data. */
#define INPUT_REP_MEDIA_PLAYER_LEN  1
/* Index of Mouse Input Report containing button data. */
#define INPUT_REP_BUTTONS_INDEX     0
/* Index of Mouse Input Report containing movement data. */
#define INPUT_REP_MOVEMENT_INDEX    1
/* Index of Mouse Input Report containing media player data. */
#define INPUT_REP_MPLAYER_INDEX     2
/* Id of reference to Mouse Input Report containing button data. */
#define INPUT_REP_REF_BUTTONS_ID    1
/* Id of reference to Mouse Input Report containing movement data. */
#define INPUT_REP_REF_MOVEMENT_ID   2
/* Id of reference to Mouse Input Report containing media player data. */
#define INPUT_REP_REF_MPLAYER_ID    3

/* HIDs queue size. */
#define HIDS_QUEUE_SIZE 10


#define KEY_LCLICK_MASK DK_BTN1_MSK
#define KEY_RCLICK_MASK DK_BTN2_MSK
#define KEY_MCLICK_MASK DK_BTN3_MSK


/* Timeout for low duty directed advertising (2 seconds). */
#define LOW_DUTY_DIRECTED_ADV_TIMEOUT_MS 2000

/* HIDS instance. */
BT_HIDS_DEF(hids_obj,
	    INPUT_REP_BUTTONS_LEN,
	    INPUT_REP_MOVEMENT_LEN,
	    INPUT_REP_MEDIA_PLAYER_LEN);

static struct k_work hids_work;
struct mouse_pos {
	int16_t x_val;
	int16_t y_val;
	uint8_t buttons;
};

uint8_t prevButtons = 0;



bool deepSleep = false;
const int AWAKE_SAMPLING_TIMEOUT_US = 500;
const int SLEEP1_SAMPLING_TIMEOUT_US = 5000;

static struct k_thread *main_thread;

volatile int SLEEP_TIMEOUT_US = 500;
volatile int ticksSinceLastMotion = 0;

const struct gpio_dt_spec motion_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(motion), gpios);
static struct gpio_callback motion_cb_data;

const struct gpio_dt_spec side_button_1 = GPIO_DT_SPEC_GET(DT_NODELABEL(side_button_1), gpios);
static struct gpio_callback sb1_cb_data;
int sb1_config_id = 1;

const struct gpio_dt_spec side_button_2 = GPIO_DT_SPEC_GET(DT_NODELABEL(side_button_2), gpios);
static struct gpio_callback sb2_cb_data;
int sb2_config_id = 1;

/*
side button configs
1: Raise CPI by 1 increment (200)
2: Lower CPI by 1 increment (200)

*/


//const struct gpio_dt_spec buttonPin1 = GPIO_DT_SPEC_GET(DT_NODELABEL(sdio), gpios);
//const struct gpio_dt_spec buttonPin2 = GPIO_DT_SPEC_GET(DT_NODELABEL(sclk), gpios);


// static struct gpio_callback button_cb_data_1;
// static struct gpio_callback button_cb_data_2;

// volatile int Input1Val = 0;
// volatile int Input2Val = 0;

// volatile int Input1Prev = 0;
// volatile int Input2Prev = 0;

// volatile int prevScrollCount = 0;
// volatile int scrollCount = 0;


// Sensor Interface SPI Stuff


#define MOUSE_MOVEMENT_MULTIPLIER  1


/* Mouse movement queue. */
K_MSGQ_DEFINE(hids_queue,
	      sizeof(struct mouse_pos),
	      HIDS_QUEUE_SIZE,
	      4);

#if CONFIG_SAMPLE_BT_DIRECTED_ADVERTISING
/* Bonded address queue. */
K_MSGQ_DEFINE(bonds_queue,
	      sizeof(bt_addr_le_t),
	      CONFIG_BT_MAX_PAIRED,
	      4);

static struct k_work_delayable low_duty_directed_adv_timeout_work;
#endif /* CONFIG_SAMPLE_BT_DIRECTED_ADVERTISING */

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
		      (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
		      (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
					  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct conn_mode {
	struct bt_conn *conn;
	bool in_boot_mode;
} conn_mode[CONFIG_BT_HIDS_MAX_CLIENT_COUNT];

static bool is_adv_running;

static struct k_work adv_work;

static void advertising_continue(void);

#if CONFIG_SAMPLE_BT_DIRECTED_ADVERTISING
static void bond_find(const struct bt_bond_info *info, void *user_data)
{
	int err;

	/* Filter already connected peers. */
	for (size_t i = 0; i < ARRAY_SIZE(conn_mode); i++) {
		if (conn_mode[i].conn) {
			const bt_addr_le_t *dst =
				bt_conn_get_dst(conn_mode[i].conn);

			if (!bt_addr_le_cmp(&info->addr, dst)) {
				return;
			}
		}
	}

	err = k_msgq_put(&bonds_queue, (void *) &info->addr, K_NO_WAIT);
	if (err) {
		printk("No space in the queue for the bond.\n");
	}
}

static bool is_any_peer_connected(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_mode); i++) {
		if (conn_mode[i].conn) {
			return true;
		}
	}

	return false;
}

static void low_duty_directed_adv_timeout_handler(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	printk("Low duty directed advertising timed out\n");

	__ASSERT_NO_MSG(is_adv_running);

	err = bt_le_adv_stop();
	if (err) {
		printk("Advertising failed to stop (err %d)\n", err);
		return;
	}
	is_adv_running = false;

	advertising_continue();
}
#endif /* CONFIG_SAMPLE_BT_DIRECTED_ADVERTISING */

static void advertising_continue(void)
{
	struct bt_le_adv_param adv_param;

#if CONFIG_SAMPLE_BT_DIRECTED_ADVERTISING
	bt_addr_le_t addr;

	if (!k_msgq_get(&bonds_queue, &addr, K_NO_WAIT)) {
		char addr_buf[BT_ADDR_LE_STR_LEN];
		bool any_peer_connected = is_any_peer_connected();
		int err;

		if (is_adv_running) {
			err = bt_le_adv_stop();
			if (err) {
				printk("Advertising failed to stop (err %d)\n", err);
				return;
			}
			is_adv_running = false;
			(void)k_work_cancel_delayable(&low_duty_directed_adv_timeout_work);
		}

		if (any_peer_connected) {
			/* Use low duty directed advertising to maintain connections with other
			 * peers. High duty directed advertising may disconnect existing
			 * connections.
			 */
			adv_param = *BT_LE_ADV_CONN_DIR_LOW_DUTY(&addr);
		} else {
			adv_param = *BT_LE_ADV_CONN_DIR(&addr);
		}
		adv_param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;

		err = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);

		if (err) {
			printk("Directed advertising failed to start (err %d)\n", err);
			return;
		}

		bt_addr_le_to_str(&addr, addr_buf, BT_ADDR_LE_STR_LEN);
		printk("Direct advertising to %s started\n", addr_buf);

		/* High-duty directed advertising expires automatically after timeout defined in
		 * the Bluetooth specification. The Bluetooth stack will call connected() callback
		 * with error code.
		 */
		if (any_peer_connected) {
			(void)k_work_schedule(&low_duty_directed_adv_timeout_work,
					      K_MSEC(LOW_DUTY_DIRECTED_ADV_TIMEOUT_MS));
		}
	} else
#endif /* CONFIG_SAMPLE_BT_DIRECTED_ADVERTISING */
	{
		int err;

		if (is_adv_running) {
			return;
		}

		adv_param = *BT_LE_ADV_CONN_FAST_2;
		err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad),
				  sd, ARRAY_SIZE(sd));
		if (err) {
			printk("Advertising failed to start (err %d)\n", err);
			return;
		}

		printk("Regular advertising started\n");
	}

	is_adv_running = true;
}

static void advertising_start(void)
{
#if CONFIG_SAMPLE_BT_DIRECTED_ADVERTISING
	k_msgq_purge(&bonds_queue);
	bt_foreach_bond(BT_ID_DEFAULT, bond_find, NULL);
#endif /* CONFIG_SAMPLE_BT_DIRECTED_ADVERTISING */

	k_work_submit(&adv_work);
}

static void advertising_process(struct k_work *work)
{
	advertising_continue();
}


static void insert_conn_object(struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_mode); i++) {
		if (!conn_mode[i].conn) {
			conn_mode[i].conn = conn;
			conn_mode[i].in_boot_mode = false;

			return;
		}
	}

	printk("Connection object could not be inserted %p\n", conn);
}


static bool is_conn_slot_free(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_mode); i++) {
		if (!conn_mode[i].conn) {
			return true;
		}
	}

	return false;
}


static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	is_adv_running = false;

#if CONFIG_SAMPLE_BT_DIRECTED_ADVERTISING
	(void)k_work_cancel_delayable(&low_duty_directed_adv_timeout_work);
#endif

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		if (err == BT_HCI_ERR_ADV_TIMEOUT) {
			printk("High-duty directed advertising to %s timed out\n", addr);
			k_work_submit(&adv_work);
		} else {
			printk("Failed to connect to %s 0x%02x %s\n", addr, err,
			       bt_hci_err_to_str(err));
		}
		return;
	}

	printk("Connected %s\n", addr);

	err = bt_hids_connected(&hids_obj, conn);

	if (err) {
		printk("Failed to notify HID service about connection\n");
		return;
	}

	insert_conn_object(conn);

	if (is_conn_slot_free()) {
		advertising_start();
	}
}


static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected from %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	err = bt_hids_disconnected(&hids_obj, conn);

	if (err) {
		printk("Failed to notify HID service about disconnection\n");
	}

	for (size_t i = 0; i < ARRAY_SIZE(conn_mode); i++) {
		if (conn_mode[i].conn == conn) {
			conn_mode[i].conn = NULL;
			break;
		}
	}

	advertising_start();
}


static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d %s\n", addr, level, err,
		       bt_security_err_to_str(err));
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,

};


static void hids_pm_evt_handler(enum bt_hids_pm_evt evt,
				struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	size_t i;

	for (i = 0; i < ARRAY_SIZE(conn_mode); i++) {
		if (conn_mode[i].conn == conn) {
			break;
		}
	}

	if (i >= ARRAY_SIZE(conn_mode)) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	switch (evt) {
	case BT_HIDS_PM_EVT_BOOT_MODE_ENTERED:
		printk("Boot mode entered %s\n", addr);
		conn_mode[i].in_boot_mode = true;
		break;

	case BT_HIDS_PM_EVT_REPORT_MODE_ENTERED:
		printk("Report mode entered %s\n", addr);
		conn_mode[i].in_boot_mode = false;
		break;

	default:
		break;
	}
}


static void hid_init(void)
{
	int err;
	struct bt_hids_init_param hids_init_param = { 0 };
	struct bt_hids_inp_rep *hids_inp_rep;
	static const uint8_t mouse_movement_mask[DIV_ROUND_UP(INPUT_REP_MOVEMENT_LEN, 8)] = {0};

	static const uint8_t report_map[] = {
		0x05, 0x01,     /* Usage Page (Generic Desktop) */
		0x09, 0x02,     /* Usage (Mouse) */

		0xA1, 0x01,     /* Collection (Application) */

		/* Report ID 1: Mouse buttons + scroll/pan */
		0x85, 0x01,       /* Report Id 1 */
		0x09, 0x01,       /* Usage (Pointer) */
		0xA1, 0x00,       /* Collection (Physical) */
		0x95, 0x05,       /* Report Count (3) */
		0x75, 0x01,       /* Report Size (1) */
		0x05, 0x09,       /* Usage Page (Buttons) */
		0x19, 0x01,       /* Usage Minimum (01) */
		0x29, 0x05,       /* Usage Maximum (05) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x01,       /* Logical Maximum (1) */
		0x81, 0x02,       /* Input (Data, Variable, Absolute) */
		0x95, 0x01,       /* Report Count (1) */
		0x75, 0x03,       /* Report Size (3) */
		0x81, 0x01,       /* Input (Constant) for padding */
		0x75, 0x08,       /* Report Size (8) */
		0x95, 0x01,       /* Report Count (1) */
		0x05, 0x01,       /* Usage Page (Generic Desktop) */
		0x09, 0x38,       /* Usage (Wheel) */
		0x15, 0x81,       /* Logical Minimum (-127) */
		0x25, 0x7F,       /* Logical Maximum (127) */
		0x81, 0x06,       /* Input (Data, Variable, Relative) */
		0x05, 0x0C,       /* Usage Page (Consumer) */
		0x0A, 0x38, 0x02, /* Usage (AC Pan) */
		0x95, 0x01,       /* Report Count (1) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0xC0,             /* End Collection (Physical) */

		/* Report ID 2: Mouse motion */
		0x85, 0x02,       /* Report Id 2 */
		0x09, 0x01,       /* Usage (Pointer) */
		0xA1, 0x00,       /* Collection (Physical) */
		0x75, 0x0C,       /* Report Size (12) */
		0x95, 0x02,       /* Report Count (2) */
		0x05, 0x01,       /* Usage Page (Generic Desktop) */
		0x09, 0x30,       /* Usage (X) */
		0x09, 0x31,       /* Usage (Y) */
		0x16, 0x01, 0xF8, /* Logical maximum (2047) */
		0x26, 0xFF, 0x07, /* Logical minimum (-2047) */
		0x81, 0x06,       /* Input (Data, Variable, Relative) */
		0xC0,             /* End Collection (Physical) */
		0xC0,             /* End Collection (Application) */

		/* Report ID 3: Advanced buttons */
		0x05, 0x0C,       /* Usage Page (Consumer) */
		0x09, 0x01,       /* Usage (Consumer Control) */
		0xA1, 0x01,       /* Collection (Application) */
		0x85, 0x03,       /* Report Id (3) */
		0x15, 0x00,       /* Logical minimum (0) */
		0x25, 0x01,       /* Logical maximum (1) */
		0x75, 0x01,       /* Report Size (1) */
		0x95, 0x01,       /* Report Count (1) */

		0x09, 0xCD,       /* Usage (Play/Pause) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x0A, 0x83, 0x01, /* Usage (Consumer Control Configuration) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x09, 0xB5,       /* Usage (Scan Next Track) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x09, 0xB6,       /* Usage (Scan Previous Track) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */

		0x09, 0xEA,       /* Usage (Volume Down) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x09, 0xE9,       /* Usage (Volume Up) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x0A, 0x25, 0x02, /* Usage (AC Forward) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x0A, 0x24, 0x02, /* Usage (AC Back) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0xC0              /* End Collection */
	};

	hids_init_param.rep_map.data = report_map;
	hids_init_param.rep_map.size = sizeof(report_map);

	hids_init_param.info.bcd_hid = BASE_USB_HID_SPEC_VERSION;
	hids_init_param.info.b_country_code = 0x00;
	hids_init_param.info.flags = (BT_HIDS_REMOTE_WAKE |
				      BT_HIDS_NORMALLY_CONNECTABLE);

	hids_inp_rep = &hids_init_param.inp_rep_group_init.reports[0];
	hids_inp_rep->size = INPUT_REP_BUTTONS_LEN;
	hids_inp_rep->id = INPUT_REP_REF_BUTTONS_ID;
	hids_init_param.inp_rep_group_init.cnt++;

	hids_inp_rep++;
	hids_inp_rep->size = INPUT_REP_MOVEMENT_LEN;
	hids_inp_rep->id = INPUT_REP_REF_MOVEMENT_ID;
	hids_inp_rep->rep_mask = mouse_movement_mask;
	hids_init_param.inp_rep_group_init.cnt++;

	hids_inp_rep++;
	hids_inp_rep->size = INPUT_REP_MEDIA_PLAYER_LEN;
	hids_inp_rep->id = INPUT_REP_REF_MPLAYER_ID;
	hids_init_param.inp_rep_group_init.cnt++;

	hids_init_param.is_mouse = true;
	hids_init_param.pm_evt_handler = hids_pm_evt_handler;

	err = bt_hids_init(&hids_obj, &hids_init_param);
	__ASSERT(err == 0, "HIDS initialization failed\n");
}


static void mouse_movement_send(int16_t x_delta, int16_t y_delta, uint8_t buttons, uint8_t scrollWheel, uint8_t media)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_mode); i++) {

		if (!conn_mode[i].conn) {
			continue;
		}

		if (conn_mode[i].in_boot_mode) {
			x_delta = MAX(MIN(x_delta, SCHAR_MAX), SCHAR_MIN);
			y_delta = MAX(MIN(y_delta, SCHAR_MAX), SCHAR_MIN);

			bt_hids_boot_mouse_inp_rep_send(&hids_obj,
							     conn_mode[i].conn,
							     NULL,
							     (int8_t) x_delta,
							     (int8_t) y_delta,
							     NULL);
		} else {

			if(x_delta != 0 || y_delta != 0) {
				uint8_t x_buff[2];
				uint8_t y_buff[2];
				uint8_t buffer[INPUT_REP_MOVEMENT_LEN];

				int16_t x = MAX(MIN(x_delta, 0x07ff), -0x07ff);
				int16_t y = MAX(MIN(y_delta, 0x07ff), -0x07ff);

				sys_put_le16(x, x_buff);
				sys_put_le16(y, y_buff);

				
				BUILD_ASSERT(sizeof(buffer) == 3,
					 "Only 2 axis, 12-bit each, are supported");

				buffer[0] = x_buff[0];
				buffer[1] = (y_buff[0] << 4) | (x_buff[1] & 0x0f);
				buffer[2] = (y_buff[1] << 4) | (y_buff[0] >> 4);


				bt_hids_inp_rep_send(&hids_obj, conn_mode[i].conn,
						  INPUT_REP_MOVEMENT_INDEX,
						  buffer, sizeof(buffer), NULL);

			}


			//Send Button and Scroll Wheel Data

			if(buttons != prevButtons || scrollWheel != 0) {
				uint8_t buffer2[3];

				buffer2[0] = buttons;
				buffer2[1] = scrollWheel;
				buffer2[2] = 0;

				bt_hids_inp_rep_send(&hids_obj, conn_mode[i].conn,
						  INPUT_REP_BUTTONS_INDEX,
						  buffer2, sizeof(buffer2), NULL);

			}
			prevButtons = buttons;


			// uint8_t buffer3 = 0;

			// bt_hids_inp_rep_send(&hids_obj, conn_mode[i].conn,
			// 			  INPUT_REP_MPLAYER_INDEX,
			// 			  buffer3, sizeof(buffer3), NULL);




			//Send Other Data???? maybe




		}
	}
}


static void mouse_handler(struct k_work *work)
{
	struct mouse_pos pos;
	while (!k_msgq_get(&hids_queue, &pos, K_NO_WAIT)) {

		mouse_movement_send(pos.x_val, pos.y_val, pos.buttons, 0, 0);
	}
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
};


#if defined(CONFIG_SAMPLE_BT_HIDS_SECURITY)
static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d %s\n", addr, reason,
	       bt_security_err_to_str(reason));
}


static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};
#else
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif 


void button_changed(uint32_t button_state, uint32_t has_changed)
{
	bool data_to_send = false;
	struct mouse_pos pos;
	uint32_t buttons = button_state & has_changed;

	memset(&pos, 0, sizeof(struct mouse_pos));


	if (button_state & KEY_LCLICK_MASK) {
		pos.buttons |= 1;
	}
	if (button_state & KEY_RCLICK_MASK) {
		pos.buttons |= 2;
	}
	if (button_state & KEY_MCLICK_MASK) {
		pos.buttons |= 4;
	 
	}

	if (has_changed & (KEY_LCLICK_MASK | KEY_RCLICK_MASK | KEY_MCLICK_MASK)) {
        data_to_send = true;
    }


	if (data_to_send) {
		int err;
		

		err = k_msgq_put(&hids_queue, &pos, K_NO_WAIT);
		if (err) {
			printk("No space in the queue for button pressed\n");
			return;
		}
		if (k_msgq_num_used_get(&hids_queue) == 1) {
			k_work_submit(&hids_work);
		}
	}
}


void configure_buttons(void)
{
	int err;


	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}
}


static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

void motion_pin_active(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {

	ticksSinceLastMotion = 0;
	SLEEP_TIMEOUT_US = AWAKE_SAMPLING_TIMEOUT_US;
	deepSleep = false;
	k_thread_resume(main_thread);

}

void sidebutton_1_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {

	if(sb1_config_id == 1) {
		int cpi = pmw3610_get_cpi();
		cpi += 200;
		LOG_INF_RATELIMIT_RATE(500, "New CPI: %d\n", cpi);
		pmw3610_change_cpi(cpi);

	}

	if(sb1_config_id == 2) {
		int cpi = pmw3610_get_cpi();
		cpi -= 200;
		LOG_INF_RATELIMIT_RATE(500, "New CPI: %d\n", cpi);
		pmw3610_change_cpi(cpi);
	}
	

}


void sidebutton_2_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {

	if(sb1_config_id == 1) {
		int cpi = pmw3610_get_cpi();
		cpi += 200;
		LOG_INF_RATELIMIT_RATE(500, "New CPI: %d\n", cpi);
		pmw3610_change_cpi(cpi);

	}

	if(sb1_config_id == 2) {
		int cpi = pmw3610_get_cpi();
		cpi -= 200;
		LOG_INF_RATELIMIT_RATE(500, "New CPI: %d\n", cpi);
		pmw3610_change_cpi(cpi);
	}
	

}


int main(void)
{
	int err;
	main_thread = k_current_get(); 

	printk("Starting Bluetooth Peripheral HIDS mouse sample\n");

	
			err = bt_conn_auth_cb_register(&conn_auth_callbacks);
			if (err) {
				printk("Failed to register authorization callbacks.\n");
				return 0;
			}
		

		err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
		if (err) {
			printk("Failed to register authorization info callbacks.\n");
			return 0;
		}
	

	/* DIS initialized at system boot with SYS_INIT macro. */
	printk("Testing print here\n");

	hid_init();

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	k_work_init(&hids_work, mouse_handler);
	k_work_init(&adv_work, advertising_process);
#if CONFIG_SAMPLE_BT_DIRECTED_ADVERTISING
	k_work_init_delayable(&low_duty_directed_adv_timeout_work,
			      low_duty_directed_adv_timeout_handler);
#endif

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	advertising_start();

	configure_buttons();



	pmw3610_init();

	uint8_t val = 0;

	int x = 0;
	int y = 0;
	int scrollCount = 0;

	int motionVal = 0;


	scroll_wheel_init();


	if (!gpio_is_ready_dt(&motion_pin)) {
    	return;
	}

	//Set up motion pin interrupt

	gpio_pin_configure_dt(&motion_pin, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&motion_pin,
					      GPIO_INT_EDGE_TO_ACTIVE | GPIO_INT_WAKEUP);

	gpio_init_callback(&motion_cb_data, motion_pin_active, BIT(motion_pin.pin));
	gpio_add_callback(motion_pin.port, &motion_cb_data);


	//Set up side button interrupts

	gpio_pin_configure_dt(&side_button_1, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&side_button_1,
					      GPIO_INT_EDGE_FALLING);

	gpio_init_callback(&sb1_cb_data, sidebutton_1_pressed, BIT(side_button_1.pin));
	gpio_add_callback(side_button_1.port, &sb1_cb_data);


	// gpio_pin_configure_dt(&side_button_2, GPIO_INPUT);
	// gpio_pin_interrupt_configure_dt(&side_button_2,
	// 				      GPIO_INT_EDGE_FALLING);

	// gpio_init_callback(&sb2_cb_data, sidebutton_2_pressed, BIT(side_button_2.pin));
	// gpio_add_callback(side_button_2.port, &sb2_cb_data);






	
	while (1) {
		scrollCount = 0;
		x = 0;
		y = 0;

		val = bitbang_read(0x00);
		LOG_INF_RATELIMIT_RATE(500, "Product ID (0x3e): %u\n", val);
		
		val = bitbang_read(0x02);
		LOG_INF_RATELIMIT_RATE(500, "Motion Register val: %u\n", val);

		motionVal = gpio_pin_get_dt(&motion_pin);
		LOG_INF_RATELIMIT_RATE(500, "Motion Pin val: %u\n", motionVal);

		if(val) {
			getXYMovement_bitbang(&x, &y);
			// LOG_INF_RATELIMIT_RATE(500, "X Value Returned: %d \n", x);
			// LOG_INF_RATELIMIT_RATE(500,"Y Value Returned:  %d \n", y);

			x = (int)(x * MOUSE_MOVEMENT_MULTIPLIER);
			y = (int)(y * MOUSE_MOVEMENT_MULTIPLIER);


			struct mouse_pos pos;
			pos.x_val = x;
			pos.y_val = y;

			

		}
		else {
			ticksSinceLastMotion++;
			if(ticksSinceLastMotion >= 10000 && SLEEP_TIMEOUT_US == AWAKE_SAMPLING_TIMEOUT_US) {
				SLEEP_TIMEOUT_US = SLEEP1_SAMPLING_TIMEOUT_US;
				ticksSinceLastMotion = 0;
			}
			if(ticksSinceLastMotion >= 36000 && SLEEP_TIMEOUT_US == SLEEP1_SAMPLING_TIMEOUT_US) {
				deepSleep = true;
				ticksSinceLastMotion = 0;
			}

		}
		getScrollUpdate(&scrollCount);

		//LOG_INF_RATELIMIT_RATE(500,"Scroll Count:  %d \n", scrollCount);

		if(scrollCount != 0 || val) {
			mouse_movement_send(x, -y, prevButtons, scrollCount, 0);
		}

		
		if(!deepSleep) {
			k_usleep(SLEEP_TIMEOUT_US);
		}
		else {
			k_thread_suspend(main_thread);
		}
		//bas_notify();
	}
}
