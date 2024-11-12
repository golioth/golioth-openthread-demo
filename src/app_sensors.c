/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_sensors, LOG_LEVEL_DBG);

#include <golioth/client.h>
#include <golioth/stream.h>
#include <zcbor_encode.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "app_sensors.h"

static struct golioth_client *client;
/* Add Sensor structs here */

/* Callback for LightDB Stream */
static void async_error_handler(struct golioth_client *client,
				const struct golioth_response *response, const char *path,
				void *arg)
{
	if (response->status != GOLIOTH_OK) {
		LOG_ERR("Async task failed: %d", response->status);
		return;
	}
}

/* This will be called by the main() loop */
/* Do all of your work here! */
void app_sensors_read_and_stream(void)
{
	int err;

	/* Send sensor data to Golioth */
	/* For this demo, we just send counter data to Golioth */
	static uint16_t counter;

	/* Only stream sensor data if connected */
	if (golioth_client_is_connected(client)) {
		/* Encode sensor data using CBOR serialization */
		uint8_t cbor_buf[13];

		ZCBOR_STATE_E(zse, 1, cbor_buf, sizeof(cbor_buf), 1);

		bool ok = zcbor_map_start_encode(zse, 1) && zcbor_tstr_put_lit(zse, "counter") &&
			  zcbor_uint32_put(zse, counter) && zcbor_map_end_encode(zse, 1);

		if (!ok) {
			LOG_ERR("Failed to encode CBOR.");
			return;
		}

		size_t cbor_size = zse->payload - cbor_buf;

		LOG_DBG("Streaming counter: %d", counter);

		/* Stream data to Golioth */
		err = golioth_stream_set_async(client, "sensor", GOLIOTH_CONTENT_TYPE_CBOR,
					       cbor_buf, cbor_size, async_error_handler, NULL);
		if (err) {
			LOG_ERR("Failed to send sensor data to Golioth: %d", err);
		}
	} else {
		LOG_DBG("No connection available, skipping streaming counter: %d", counter);
	}

	++counter;
}

void app_sensors_set_client(struct golioth_client *sensors_client)
{
	client = sensors_client;
}
