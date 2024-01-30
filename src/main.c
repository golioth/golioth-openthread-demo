/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/kernel.h"
#include "zephyr/sys/util_macro.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(golioth_openthread_demo, LOG_LEVEL_DBG);

#include <modem/lte_lc.h>
#include <net/golioth/system_client.h>
#include <zephyr/net/coap.h>
#include "app_rpc.h"
#include "app_settings.h"
#include "app_state.h"
#include "app_work.h"
#include "dfu/app_dfu.h"

#include <zephyr/drivers/gpio.h>

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/socket.h>

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();

K_SEM_DEFINE(connected, 0, 1);
K_SEM_DEFINE(dfu_status_unreported, 1, 1);

static k_tid_t _system_thread = 0;

static const struct gpio_dt_spec golioth_led = GPIO_DT_SPEC_GET(DT_ALIAS(golioth_led), gpios);
static const struct gpio_dt_spec user_btn = GPIO_DT_SPEC_GET(DT_ALIAS(user_btn), gpios);

static struct gpio_callback button_cb_data;

static struct k_work on_connect_work;
static struct k_work on_disconnect_work;

static void on_ot_connect(struct k_work *item)
{
	ARG_UNUSED(item);

	LOG_INF("OpenThread on connect");
}

static void on_ot_disconnect(struct k_work *item)
{
	ARG_UNUSED(item);

	LOG_INF("OpenThread on disconnect");
}

static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
				    void *user_data)
{
	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(ot_context->instance)) {
		case OT_DEVICE_ROLE_CHILD:
		case OT_DEVICE_ROLE_ROUTER:
		case OT_DEVICE_ROLE_LEADER:
			k_work_submit(&on_connect_work);
			break;

		case OT_DEVICE_ROLE_DISABLED:
		case OT_DEVICE_ROLE_DETACHED:

		default:
			k_work_submit(&on_disconnect_work);
			break;
		}
	}

	if (flags == OT_CHANGED_IP6_ADDRESS_ADDED) {
		golioth_system_client_start();

	}
}

static struct openthread_state_changed_cb ot_state_chaged_cb = {
	.state_changed_cb = on_thread_state_changed
};

/* forward declarations */
void golioth_connection_led_set(uint8_t state);

void wake_system_thread(void)
{
	k_wakeup(_system_thread);
}

static void golioth_on_connect(struct golioth_client *client)
{
	k_sem_give(&connected);
	golioth_connection_led_set(1);

	LOG_INF("Registering observations with Golioth");
	app_dfu_observe();
	app_settings_observe();
	app_rpc_observe();
	app_state_observe();
	
	if (k_sem_take(&dfu_status_unreported, K_NO_WAIT) == 0) {
		/* Report firmware update status on first connect after power up */
		app_dfu_report_state_to_golioth();
	}
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("Button pressed at %d", k_cycle_get_32());
	/* This function is an Interrupt Service Routine. Do not call functions that
	 * use other threads, or perform long-running operations here
	 */
	k_wakeup(_system_thread);
}

/* Set (unset) LED indicators for active Golioth connection */
void golioth_connection_led_set(uint8_t state)
{
	uint8_t pin_state = state ? 1 : 0;
	/* Turn on Golioth logo LED once connected */
	gpio_pin_set_dt(&golioth_led, pin_state);
}


int main(void)
{
	int err = 0;

	LOG_DBG("Start Reference Design Template sample");

	IF_ENABLED(CONFIG_NET_L2_OPENTHREAD, (
		k_work_init(&on_connect_work, on_ot_connect);
		k_work_init(&on_disconnect_work, on_ot_disconnect);

		openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);
		openthread_start(openthread_get_default_context());
	));

	LOG_INF("Firmware version: %s", CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION);

	/* Get system thread id so loop delay change event can wake main */
	_system_thread = k_current_get();

	/* Initialize Golioth logo LED */
	err = gpio_pin_configure_dt(&golioth_led, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Unable to configure LED for Golioth Logo");
	}

	/* Initialize app state */
	app_state_init(client);

	/* Initialize app work */
	app_work_init(client);

	/* Initialize DFU components */
	app_dfu_init(client);

	/* Initialize app settings */
	app_settings_init(client);

	/* Initialize app RPC */
	app_rpc_init(client);
	
	/* Register Golioth on_connect callback */
	client->on_connect = golioth_on_connect;
	
	/* Block until connected to Golioth */
	k_sem_take(&connected, K_FOREVER);

	/* Turn on Golioth logo LED once connected */
	gpio_pin_set_dt(&golioth_led, 1);

	/* Set up user button */
	err = gpio_pin_configure_dt(&user_btn, GPIO_INPUT);
	if (err) {
		LOG_ERR("Error %d: failed to configure %s pin %d", err, user_btn.port->name,
			user_btn.pin);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&user_btn, GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d", err,
			user_btn.port->name, user_btn.pin);
		return err;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(user_btn.pin));
	gpio_add_callback(user_btn.port, &button_cb_data);

	while (true) {
		app_work_sensor_read();

		k_sleep(K_SECONDS(get_loop_delay_s()));
	}
}
