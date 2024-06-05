/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/kernel.h"
#include "zephyr/sys/util_macro.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(golioth_openthread_demo, LOG_LEVEL_DBG);

#include "app_rpc.h"
#include "app_settings.h"
#include "app_state.h"
#include "app_sensors.h"
#include <golioth/client.h>
#include <golioth/fw_update.h>
#include <samples/common/net_connect.h>
#include <samples/common/sample_credentials.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/socket.h>

/* Current firmware version; update in prj.conf or via build argument */
static const char *_current_version = CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION;

static struct golioth_client *client;
K_SEM_DEFINE(connected, 0, 1);

static k_tid_t _system_thread = 0;

#if DT_NODE_EXISTS(DT_ALIAS(golioth_led))
static const struct gpio_dt_spec golioth_led = GPIO_DT_SPEC_GET(DT_ALIAS(golioth_led), gpios);
static const struct gpio_dt_spec user_btn = GPIO_DT_SPEC_GET(DT_ALIAS(user_btn), gpios);
#endif /* DT_NODE_EXISTS(DT_ALIAS(golioth_led)) */

static struct gpio_callback button_cb_data;

static struct k_work on_connect_work;
static struct k_work on_disconnect_work;

/* forward declarations */
void golioth_connection_led_set(uint8_t state);

void wake_system_thread(void)
{
	k_wakeup(_system_thread);
}

static void on_client_event(struct golioth_client *client,
			    enum golioth_client_event event,
			    void *arg)
{
	bool is_connected = (event == GOLIOTH_CLIENT_EVENT_CONNECTED);

	if (is_connected) {
		k_sem_give(&connected);
		golioth_connection_led_set(1);
	} else {
		golioth_connection_led_set(0);
	}

	LOG_INF("Golioth client %s", is_connected ? "connected" : "disconnected");
}

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

static void start_golioth_client(void)
{
	/* Get the client configuration from auto-loaded settings */
	const struct golioth_client_config *client_config = golioth_sample_credentials_get();

	/* Create and start a Golioth Client */
	client = golioth_client_create(client_config);

	/* Register Golioth on_connect callback */
	golioth_client_register_event_callback(client, on_client_event, NULL);

	/* Initialize DFU components */
	golioth_fw_update_init(client, _current_version);

	/*** Call Golioth APIs for other services in dedicated app files ***/
	/* Observe State service data */
	app_state_observe(client);

	/* Set Golioth Client for streaming sensor data */
	app_sensors_set_client(client);

	/* Register Settings service */
	app_settings_register(client);

	/* Register RPC service */
	app_rpc_register(client);
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
		start_golioth_client();
	}
}

static struct openthread_state_changed_cb ot_state_chaged_cb = {
	.state_changed_cb = on_thread_state_changed
};

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
#if DT_NODE_EXISTS(DT_ALIAS(golioth_led))
	/* Turn on Golioth logo LED once connected */
	gpio_pin_set_dt(&golioth_led, pin_state);
#endif /* #if DT_NODE_EXISTS(DT_ALIAS(golioth_led)) */
	/* Change the state of the Golioth LED on Ostentus */
	IF_ENABLED(CONFIG_LIB_OSTENTUS, (led_golioth_set(pin_state);));
}


int main(void)
{
	int err = 0;

	LOG_DBG("Start OpenThread demo");

	IF_ENABLED(CONFIG_NET_L2_OPENTHREAD, (
		k_work_init(&on_connect_work, on_ot_connect);
		k_work_init(&on_disconnect_work, on_ot_disconnect);

		openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);
		openthread_start(openthread_get_default_context());
	));

	LOG_INF("Firmware version: %s", CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION);

	/* Get system thread id so loop delay change event can wake main */
	_system_thread = k_current_get();

#if DT_NODE_EXISTS(DT_ALIAS(golioth_led))
	/* Initialize Golioth logo LED */
	err = gpio_pin_configure_dt(&golioth_led, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Unable to configure LED for Golioth Logo");
	}
#endif /* #if DT_NODE_EXISTS(DT_ALIAS(golioth_led)) */

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
		app_sensors_read_and_stream();

		k_sleep(K_SECONDS(get_loop_delay_s()));
	}
}
