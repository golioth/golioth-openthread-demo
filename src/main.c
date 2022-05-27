#include <dk_buttons_and_leds.h>
#include <logging/log.h>

#include <drivers/uart.h>
#include <usb/usb_device.h>

#include <net/golioth/system_client.h>

#include <net/openthread.h>
#include <openthread/thread.h>

#include <drivers/sensor.h>
#include <device.h>

#include <init.h>



LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define CONSOLE_LABEL DT_LABEL(DT_CHOSEN(zephyr_console))
#define OT_CONNECTION_LED DK_LED2

int sensor_interval = 30;
int counter = 0;

struct device *temp_sensor;
struct device *imu_sensor;

static struct k_work on_connect_work;
static struct k_work on_disconnect_work;

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();

static void golioth_on_message(struct golioth_client *client,
			       struct coap_packet *rx)
{
	uint16_t payload_len;
	const uint8_t *payload;
	uint8_t type;

	type = coap_header_get_type(rx);
	payload = coap_packet_get_payload(rx, &payload_len);

	if (!IS_ENABLED(CONFIG_LOG_BACKEND_GOLIOTH) && payload) {
		LOG_HEXDUMP_DBG(payload, payload_len, "Payload");
	}
}

// This work function will submit a LightDB Stream output
// It should be called every time the sensor takes a reading

void my_stream_work_handler(struct k_work *work)
{
	int err;
	struct sensor_value temp;
	struct sensor_value accel_x;
	struct sensor_value accel_y;
	struct sensor_value accel_z;
	char temp_string[sizeof("4294967295")];
	
	// kick off a temp sensor reading!
	sensor_sample_fetch(temp_sensor);
	sensor_channel_get(temp_sensor, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	LOG_DBG("Temp is %d.%06d", temp.val1, abs(temp.val2));

	snprintk(temp_string, sizeof(temp_string) - 1, "%d.%06d", temp.val1, abs(temp.val2));

	err = golioth_lightdb_set(client,
					  GOLIOTH_LIGHTDB_STREAM_PATH("temp"),
					  COAP_CONTENT_FORMAT_TEXT_PLAIN,
					  temp_string,
					  strlen(temp_string));
	if (err) {
		LOG_WRN("Failed to send temperature: %d", err);
		printk("Failed to send temperature: %d\n", err);	
	}

	// kick off a temp sensor reading!
	sensor_sample_fetch(imu_sensor);
	sensor_channel_get(imu_sensor, SENSOR_CHAN_ACCEL_X, &accel_x);
	LOG_DBG("Accel X is %d.%06d", accel_x.val1, abs(accel_x.val2));
	sensor_channel_get(imu_sensor, SENSOR_CHAN_ACCEL_Y, &accel_y);
	LOG_DBG("Accel Y is %d.%06d", accel_y.val1, abs(accel_y.val2));	
	sensor_channel_get(imu_sensor, SENSOR_CHAN_ACCEL_Z, &accel_z);
	LOG_DBG("Accel Z is %d.%06d", accel_z.val1, abs(accel_z.val2));	

	// snprintk(imu_string, sizeof(temp_string) - 1, "%d.%06d", temp.val1, abs(temp.val2));

	// err = golioth_lightdb_set(client,
	// 				  GOLIOTH_LIGHTDB_STREAM_PATH("temp"),
	// 				  COAP_CONTENT_FORMAT_TEXT_PLAIN,
	// 				  temp_string,
	// 				  strlen(temp_string));
	// if (err) {
	// 	LOG_WRN("Failed to send temperature: %d", err);
	// 	printk("Failed to send temperature: %d\n", err);	
	// }

	
	dk_set_led_off(DK_LED1);
	//LOG_DBG("Done sending to Golioth LightDB Stream\n");
}

K_WORK_DEFINE(my_stream_work, my_stream_work_handler);


// This work function will take a sensor reading
// And kick off a LightDB stream event
// It should be called every time the timer fires

void my_sensor_work_handler(struct k_work *work)
{

	LOG_DBG("Taking sensor readings (placeholder)");

	dk_set_led_on(DK_LED1);

	k_work_submit(&my_stream_work);
	

}

K_WORK_DEFINE(my_sensor_work, my_sensor_work_handler);

void my_timer_handler(struct k_timer *dummy) {

	char sbuf[sizeof("4294967295")];
	int err;

	snprintk(sbuf, sizeof(sbuf) - 1, "%d", counter);

	LOG_DBG("Interval of %d seconds is up, updating state and taking a reading", sensor_interval);
	
	err = golioth_lightdb_set(client,
				  GOLIOTH_LIGHTDB_PATH("number_of_timed_updates"),
				  COAP_CONTENT_FORMAT_TEXT_PLAIN,
				  sbuf, strlen(sbuf));
	if (err) {
		LOG_WRN("Failed to update counter: %d", err);
	}
	
	counter++;


	k_work_submit(&my_sensor_work);

}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);


static void on_ot_connect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_on(OT_CONNECTION_LED);

	client->on_message = golioth_on_message;
	golioth_system_client_start();
}

static void on_ot_disconnect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_off(OT_CONNECTION_LED);
}


static void on_button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;

	if ((buttons & DK_BTN1_MSK) && button_state == 1) {
		golioth_send_hello(client); 
		LOG_DBG("Button %d pressed, taking a reading now!", has_changed);
		k_work_submit(&my_sensor_work);
	}

}

static void on_thread_state_changed(uint32_t flags, void *context)
{
	struct openthread_context *ot_context = context;

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
}

void main(void)
{
	int ret;

#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
	const struct device *dev;
	uint32_t dtr = 0U;

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	if (dev == NULL) {
		LOG_ERR("Failed to find specific UART device");
		return;
	}

	LOG_INF("Waiting for host to be ready to communicate");

	/* Data Terminal Ready - check if host is ready to communicate */
	while (!dtr) {
		ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		if (ret) {
			LOG_ERR("Failed to get Data Terminal Ready line state: %d",
				ret);
			continue;
		}
		k_msleep(100);
	}

	/* Data Carrier Detect Modem - mark connection as established */
	(void)uart_line_ctrl_set(dev, UART_LINE_CTRL_DCD, 1);
	/* Data Set Ready - the NCP SoC is ready to communicate */
	(void)uart_line_ctrl_set(dev, UART_LINE_CTRL_DSR, 1);
#endif

	LOG_INF("Start Golioth Thread sample");

	ret = dk_buttons_init(on_button_changed);
	if (ret) {
		LOG_ERR("Cannot init buttons (error: %d)", ret);
		return;
	}

	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("Cannot init leds, (error: %d)", ret);
		return;
	}

	temp_sensor = (void *)DEVICE_DT_GET_ANY(silabs_si7055);

    if (temp_sensor == NULL) {
        printk("Could not get si7055 device\n");
        return;
    }

	imu_sensor = (void *)DEVICE_DT_GET_ANY(st_lis2dh12);

    if (imu_sensor == NULL) {
        printk("Could not get lis2dh12 device\n");
        return;
    }

	k_work_init(&on_connect_work, on_ot_connect);
	k_work_init(&on_disconnect_work, on_ot_disconnect);

	openthread_set_state_changed_cb(on_thread_state_changed);
	openthread_start(openthread_get_default_context());


    k_timer_start(&my_timer, K_SECONDS(sensor_interval), K_SECONDS(sensor_interval));

}