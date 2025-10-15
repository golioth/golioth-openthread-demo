# Golioth OpenThread Demo

This Golioth OpenThread Demo is a Reference Design for a Thread Protocol
enabled device using Zephyr and connecting to Golioth over IPv6.

Devices using the Thread protocol work with Golioth thanks to the
internet connection available through a border router. This example
application shows how to flash and provision a Thread device. Once
connected it will periodically stream an up-counting timer value to the
cloud to simulate time-series sensors data. The device may be controlled
from the Golioth web console to adjust settings (like the frequency of
sensor readings), issue RPCs, remotely view device logs, and deploy
Over-the-Air firmware updates (OTA).

Business use cases and hardware build details are available on [the
Golioth Thread Demo Project
Page](https://projects.golioth.io/reference-designs/openthread-demo/).

This repository contains the firmware source code and [pre-built release
firmware
images](https://github.com/golioth/golioth-openthread-demo/releases).

## Supported Hardware

- Nordic nRF52840-DK
- Adafruit Feather nRF52840 Express

### Additional Sensors/Components

  - A running Thread Border Router with NAT64 translation (we will be
    using the commercially available off-the-shelf [GL-S200 Thread
    Border Router](https://www.gl-inet.com/products/gl-s200/)).

## Golioth Features

This app implements:

  - [Device Settings
    Service](https://docs.golioth.io/firmware/golioth-firmware-sdk/device-settings-service)
  - [Remote Procedure Call
    (RPC)](https://docs.golioth.io/firmware/golioth-firmware-sdk/remote-procedure-call)
  - [Stream
    Client](https://docs.golioth.io/firmware/golioth-firmware-sdk/stream-client)
  - [LightDB State
    Client](https://docs.golioth.io/firmware/golioth-firmware-sdk/light-db-state/)
  - [Over-the-Air (OTA) Firmware
    Upgrade](https://docs.golioth.io/firmware/golioth-firmware-sdk/firmware-upgrade/firmware-upgrade)
  - [Backend
    Logging](https://docs.golioth.io/device-management/logging/)

### Settings Service

The following settings should be set in [the Device Settings menu of the
Golioth Console](https://console.golioth.io/device-settings).

  - `LOOP_DELAY_S`
    Adjusts the delay between sensor readings. Set to an integer value
    (seconds).

    Default value is `60` seconds.

### Remote Procedure Call (RPC) Service

The following RPCs can be initiated in the Remote Procedure Call tab of
each device in the [Golioth Console](https://console.golioth.io).

  - `reboot`
    Reboot the system.

  - `set_log_level`
    Set the log level.

    The method takes a single parameter which can be one of the
    following integer values:

      - `0`: `LOG_LEVEL_NONE`
      - `1`: `LOG_LEVEL_ERR`
      - `2`: `LOG_LEVEL_WRN`
      - `3`: `LOG_LEVEL_INF`
      - `4`: `LOG_LEVEL_DBG`

### Time-Series Stream data

Sensor readings are simulated using an up-counting timer. The value is
periodically sent to the `sensor/counter` path of the Golioth Stream
service.

``` json
{
  "counter": 1
}
```

> [!NOTE]
> Your Golioth project must have a Pipeline enabled to receive this
> data. See the [Add Pipeline to Golioth](#add-pipeline-to-golioth)
> section below.

### Stateful Data (LightDB State)

The concept of Digital Twin is demonstrated with the LightDB State
`example_int0` and `example_int1` variables that are subpaths of the
`desired` and `state` paths.

  - `desired` values may be changed from the cloud side. The device will
    recognize these, validate them for \[0..65535\] bounding, and then
    reset these values to `-1`
  - `state` values will be updated by the device to reflect the device's
    actual stored value. The cloud may read the `state` endpoints to
    determine device status. In this arrangement, only the device
    should ever write to the `state` endpoints.

``` json
{
  "desired": {
    "example_int0": -1,
    "example_int1": -1
  },
  "state": {
    "example_int0": 0,
    "example_int1": 1
  }
}
```

By default the state values will be `0` and `1`. Try updating the
`desired` values and observe how the device updates its state.

### OTA Firmware Update

This application includes the ability to perform Over-the-Air (OTA)
firmware updates. To do so, you need a binary compiled with a different
version number than what is currently running on the device.

> [!NOTE]
> If a newer release is available than what your device is currently
> running, you may download the pre-compiled binary that ends in
> `_update.bin` and use it in step 2 below.

1. Update the version number in the `VERSION` file and perform a
   pristine (important) build to incorporate the version change.
2. Upload the `build/app/zephyr/zephyr.signed.bin` file as a Package for
   your Golioth project.

   - Use `main` as the package name.
   - Use the same version number from step 1.

3. Create a Cohort and add your device to it.
4. Create a Deployment for your Cohort using the package name and
   version number from step 2.
5. Devices in your Cohort will automatically upgrade to the most
   recently deployed firmware.

Visit [the Golioth Docs OTA Firmware Upgrade
page](https://docs.golioth.io/firmware/golioth-firmware-sdk/firmware-upgrade/firmware-upgrade)
for more info.

## Add Pipeline to Golioth

Golioth uses [Pipelines](https://docs.golioth.io/data-routing) to route
stream data. This gives you flexibility to change your data routing
without requiring updated device firmware.

Whenever sending stream data, you must enable a pipeline in your Golioth
project to configure how that data is handled. Add the contents of
`pipelines/cbor-to-lightdb.yml` as a new pipeline as follows (note that
this is the default pipeline for new projects and may already be
present):

1.  Navigate to your project on the Golioth web console.
2.  Select `Pipelines` from the left sidebar and click the `Create`
    button.
3.  Give your new pipeline a name and paste the pipeline configuration
    into the editor.
4.  Click the toggle in the bottom right to enable the pipeline and
    then click `Create`.

All data streamed to Golioth in CBOR format will now be routed to
LightDB Stream and may be viewed using the web console. You may change
this behavior at any time without updating firmware simply by editing
this pipeline entry.

## Local set up

> [!IMPORTANT]
> Do not clone this repo using git. Zephyr's `west` meta tool should be
> used to set up your local workspace.

### Install the Python virtual environment (recommended)

``` shell
cd ~
mkdir golioth-openthread-demo
python -m venv golioth-openthread-demo/.venv
source golioth-openthread-demo/.venv/bin/activate
pip install wheel west ecdsa
```

### Use `west` to initialize and install

``` shell
cd ~/golioth-openthread-demo
west init -m git@github.com:golioth/golioth-openthread-demo.git .
west update
west zephyr-export
pip install -r deps/zephyr/scripts/requirements.txt
```

## Configure `prj.conf`

Configure the following Kconfig options:

  - `OPENTHREAD_NETWORK_NAME`: Name of your Thread network
  - `OPENTHREAD_NETWORKKEY`: Network Key of your Thread network

by changing these lines in the `prj.conf` configuration file, e.g.:

``` cfg
CONFIG_OPENTHREAD_NETWORKKEY="00:11:22:33:44:55:66:77:88:99:aa:bb:cc:dd:ee:ff"
CONFIG_OPENTHREAD_NETWORK_NAME="golioth-thread"
CONFIG_OPENTHREAD_CHANNEL=26
CONFIG_OPENTHREAD_PANID=34739
```

> [!NOTE]
> Make sure the Thread Network Name, Thread Network Key, Thread Channel
> and Thread PAN ID match your Border Router configuration.

## Building the application

Build the Zephyr sample application for the [Nordic nRF52840
DK](https://www.nordicsemi.com/Products/Development-hardware/nRF52840-DK)
(`nrf52840dk_nrf52840`) from the top level of your project. After a
successful build you will see a new `build` directory. Note that any
changes (and git commits) to the project itself will be inside the `app`
folder. The `build` and `deps` directories being one level higher
prevents the repo from cataloging all of the changes to the dependencies
and the build (so no `.gitignore` is needed).

Prior to building, update `VERSION` file to reflect the firmware version
number you want to assign to this build. Then run the following commands
to build and program the firmware onto the device.

> [!WARNING]
> You must perform a pristine build (use `-p` or remove the `build`
> directory) after changing the firmware version number in the `VERSION`
> file for the change to take effect.

``` text
$ (.venv) west build -p -b nrf52840dk/nrf52840 --sysbuild app
$ (.venv) west flash
```

Configure PSK-ID and PSK using the device shell based on your Golioth
credentials and reboot:

``` text
uart:~$ settings set golioth/psk-id <my-psk-id@my-project>
uart:~$ settings set golioth/psk <my-psk>
uart:~$ kernel reboot cold
```

## Hardware Variations


### Adafruit Feather nRF52840 Express

This reference design may be built for the Adafruit Feather nRF52840
Express board.

``` text
$ (.venv) west build -p -b adafruit_feather_nrf52840/nrf52840 --sysbuild app
$ (.venv) west flash
```

## Have Questions?

Please get in touch with Golioth engineers by starting a new thread on
the [Golioth Forum](https://forum.golioth.io/).
