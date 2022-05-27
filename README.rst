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
- `The Laird BT510 <https://www.lairdconnect.com/iot-devices/iot-sensors/bt510-bluetooth-5-long-range-ip67-multi-sensor>`_.  

This demo can also worked on the `Nordic nRF52840-DK <https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk>`_., but will not have on-board sensors.

There are additional instructions around setting up RCP and OTBR on `our documentation page<https://golioth.github.io/golioth-openthread-demo-docs>`_.

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

- OPENTHREAD_NETWORK_NAME       - Name of your Thread network
- OPENTHREAD_NETWORKKEY         - Network Key of your Thread network

by changing these lines in the ``prj.conf`` configuration file, e.g.:

.. code-block:: cfg

   CONFIG_OPENTHREAD_NETWORKKEY="00:11:22:33:44:55:66:77:88:99:aa:bb:cc:dd:ee:ff"
   CONFIG_OPENTHREAD_NETWORK_NAME="OpenThreadDemo"

Build, Flash, Provision
***********************

Build and Flash
===============

.. code-block:: console
    
    west build -b bt510 app
    west flash

Note, this requires a board with a debugger, either on-board or on an external platform. 

For the Laird BT510, you will need the `Laird SWD USB programming kit<https://www.lairdconnect.com/wireless-modules/programming-kits/usb-swd-programming-kit>`_.

Provision
=========

If your device is not connecting to your OpenThread network using the info in your ``prj.conf``, use the following commands on the shell (connect to the device using the programmer)

.. code-block:: console
    
    uart:~$ ot ifconfig down
    uart:~$ ot dataset networkkey 00112233445566778899aabbccddeeff
    uart:~$ ot dataset networkname OpenThreadDemo
    uart:~$ ot dataset commit active
    uart:~$ ot ifconfig up
    uart:~$ ot thread start

Check your device is attempting to attach to the OTBR using the command ``ot state``

Finally, add your Golioth credentials using the settings shell. Connect over serial (programmer) to your device and then apply your Golioth PSK-ID / PSK

.. code-block:: console
    
    uart:~$ settings set golioth/psk-id <my-psk-id@my-project>
    uart:~$ settings set golioth/psk <my-psk>
    uart:~$ kernel reboot cold

These will persist after updates to your firmware, so you should only need to add them once.