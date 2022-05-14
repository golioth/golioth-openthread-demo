Golioth OpenThread Demo
#########################

Overview
********

A demonstration of a Zephyr device connecting to Golioth over IPv6 via Thread Protocol.

This is a standalone repository that will download all required files and dependencies. This directory will take up a good amount of room, as it will contain the latest Nordic NCS in the ``deps`` repository..


Requirements
************

- Golioth device PSK credentials
- A running Thread Border Router with NAT64 (we will be using an OpenThread Border Router - OTBR)
- Thread network name and PSK key

There are additional instructions around setting up RCP and OTBR on our documentation page: 

Usage
*****

The firmware will wait for a serial console to attach over USB.

Once USB is attached, it will proceed with connecting to the pre-configured
thread network.

Upon successful connection, the green LED of the nRF52840 dongle will turn on.

When you press a button, a new log event will be sent to the Golioth Log device service.

A full Zephyr shell is available on the USB serial console, along with openthread commands.

You can list available openthread commands by running ``ot help`` on the console.


Download This Repository
************************

.. code-block:: console

    west init -m https://github.com/golioth/golioth-openthread-demo.git golioth-openthread
    cd golioth-openthread
    west update
    

Configure ``prj.conf``
********************

Configure the following Kconfig options

- GOLIOTH_SYSTEM_CLIENT_PSK_ID  - PSK ID of your Golioth registered device
- GOLIOTH_SYSTEM_CLIENT_PSK     - PSK of your Golioth registered device
- OPENTHREAD_NETWORK_NAME       - Name of your Thread network
- OPENTHREAD_NETWORKKEY         - Network Key of your Thread network

by changing these lines in the ``prj.conf`` configuration file, e.g.:

.. code-block:: cfg

   CONFIG_GOLIOTH_SYSTEM_CLIENT_PSK_ID="my-psk-id@my-project"
   CONFIG_GOLIOTH_SYSTEM_CLIENT_PSK="my-psk"
   CONFIG_OPENTHREAD_NETWORKKEY="00:11:22:33:44:55:66:77:88:99:aa:bb:cc:dd:ee:ff"
   CONFIG_OPENTHREAD_NETWORK_NAME="OpenThreadDemo"

Build and Flash
***************

Recommended build method
========================

There is a file called ``credentials.conf_example`` in this repo. Rename this to ``credentials.conf``. 
Place to your Golioth credentials from the Console in here (PSK/PSK_ID).

This is the recommended path because any subsequent commit to the repo (or a forked repo) will not push your
credentials to the git server. This requires a special command at build time.

.. code-block:: console

    west build -b bt510 app -D OVERLAY_CONFIG=credentials.conf

Alternative build method
========================

Some people prefer to use the prj.conf file to store their credentials. Understand this is a higher risk of exposing
your credentials in a repo. 

.. code-block:: console
    
    west build -b bt510 app


Both of the above methods builds for `the Laird BT510 <https://www.lairdconnect.com/iot-devices/iot-sensors/bt510-bluetooth-5-long-range-ip67-multi-sensor>`_., but has also worked on the `Nordic nRF52840-DK <https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk>`_.

Flash your board 
================
.. code-block:: console
   west flash

Note, this requires a board with a debugger, either on-board or on an external platform. 


Additional Steps For Runing on nRF52840 USB Dongle
**************************************************

Additional steps are required for the nRF52 dongle because of the reduced interfaces, the built in bootloader, and reliance on using USB.

Build
=====

.. code-block:: console

   west build -b nrf52840dongle_nrf52840 ./ -- -DOVERLAY_CONFIG="overlay-usb.conf" -DDTC_OVERLAY_FILE="usb.overlay"


Package
=======

Package as a ZIP archive for ``nrfutil``

.. code-block:: console

   nrfutil pkg generate --hw-version 52 --sd-req=0x00 \
    --application build/zephyr/zephyr.hex --application-version 1 build/zephyr/zephyr.zip


Flash
==================

.. code-block:: console

   nrfutil dfu usb-serial -pkg build/zephyr/zephyr.zip -p /dev/ttyACM0

or use the nRF Connect v3.7.1 Programmer tool.

You might need to replace /dev/ttyACM0 with the serial port (tty device) your device is using.
