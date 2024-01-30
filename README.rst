..
   Copyright (c) 2022-2023 Golioth, Inc.
   SPDX-License-Identifier: Apache-2.0

Golioth Reference Design Template
#################################

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

This demo can also work on the `Nordic nRF52840-DK <https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk>`_, but will not have on-board sensors. All build commands will explicitly call out the BT510.

There are additional instructions around setting up RCP and OTBR on `our documentation page <https://golioth.github.io/golioth-openthread-demo-docs>`_.

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

For the Laird BT510, you will need the `Laird SWD USB programming kit <https://www.lairdconnect.com/wireless-modules/programming-kits/usb-swd-programming-kit>`_.

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
=======
Use this repo as a template when beginning work on a new Golioth Reference
Design. It is set up as a standalone repository, with all Golioth features
implemented in basic form. Search the project for the word ``template`` and
``rd_template`` and update those occurrences with your reference design's name.

Local set up
************

Do not clone this repo using git. Zephyr's ``west`` meta tool should be used to
set up your local workspace.

Install the Python virtual environment (recommended)
====================================================

.. code-block:: shell

   cd ~
   mkdir golioth-reference-design-template
   python -m venv golioth-reference-design-template/.venv
   source golioth-reference-design-template/.venv/bin/activate
   pip install wheel west

Use ``west`` to initialize and install
======================================

.. code-block:: shell

   cd ~/golioth-reference-design-template
   west init -m git@github.com:golioth/reference-design-template.git .
   west update
   west zephyr-export
   pip install -r deps/zephyr/scripts/requirements.txt

Building the application
************************

Build Zephyr sample application for Golioth Aludel-Mini
(``aludel_mini_v1_sparkfun9160_ns``) from the top level of your project. After a
successful build you will see a new ``build`` directory. Note that any changes
(and git commits) to the project itself will be inside the ``app`` folder. The
``build`` and ``deps`` directories being one level higher prevents the repo from
cataloging all of the changes to the dependencies and the build (so no
``.gitignore`` is needed)

During building, replace ``<your.semantic.version>`` to utilize the DFU
functionality on this Reference Design.

.. code-block:: text

   $ (.venv) west build -p -b aludel_mini_v1_sparkfun9160_ns app -- -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\"<your.semantic.version>\"
   $ (.venv) west flash

Configure PSK-ID and PSK using the device shell based on your Golioth
credentials and reboot:

.. code-block:: text

   uart:~$ settings set golioth/psk-id <my-psk-id@my-project>
   uart:~$ settings set golioth/psk <my-psk>
   uart:~$ kernel reboot cold

Golioth Features
****************

This app currently implements Over-the-Air (OTA) firmware updates, Settings
Service, Logging, RPC, and both LightDB State and LightDB Stream data.

Settings Service
================

The following settings should be set in the Device Settings menu of the
`Golioth Console`_.

``LOOP_DELAY_S``
   Adjusts the delay between sensor readings. Set to an integer value (seconds).

   Default value is ``60`` seconds.

Remote Procedure Call (RPC) Service
===================================

The following RPCs can be initiated in the Remote Procedure Call menu of the
`Golioth Console`_.

``get_network_info``
   Query and return network information.

``reboot``
   Reboot the system.

``set_log_level``
   Set the log level.

   The method takes a single parameter which can be one of the following integer
   values:

   * ``0``: ``LOG_LEVEL_NONE``
   * ``1``: ``LOG_LEVEL_ERR``
   * ``2``: ``LOG_LEVEL_WRN``
   * ``3``: ``LOG_LEVEL_INF``
   * ``4``: ``LOG_LEVEL_DBG``

LightDB State and LightDB Stream data
=====================================

Time-Series Data (LightDB Stream)
---------------------------------

An up-counting timer is periodically sent to the ``sensor/counter`` endpoint of the
LightDB Stream service to simulate sensor data. If your board includes a
battery, voltage and level readings will be sent to the ``battery`` endpoint.

Stateful Data (LightDB State)
-----------------------------

The concept of Digital Twin is demonstrated with the LightDB State
``example_int0`` and ``example_int1`` variables that are members of the ``desired``
and ``state`` endpoints.

* ``desired`` values may be changed from the cloud side. The device will recognize
  these, validate them for [0..65535] bounding, and then reset these endpoints
  to ``-1``

* ``state`` values will be updated by the device whenever a valid value is
  received from the ``desired`` endpoints. The cloud may read the ``state``
  endpoints to determine device status, but only the device should ever write to
  the ``state`` endpoints.

Further Information in Header Files
===================================

Please refer to the comments in each header file for a service-by-service
explanation of this template.

Hardware Variations
*******************

Nordic nRF9160 DK
=================

This reference design may be built for the `Nordic nRF9160 DK`_.

Use the following commands to build and program. (Use the same console commands
from above to provision this board after programming the firmware.)

.. code-block:: text

   $ (.venv) west build -p -b nrf9160dk_nrf9160_ns app -- -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\"<your.semantic.version>\"
   $ (.venv) west flash

External Libraries
******************

The following code libraries are installed by default. If you are not using the
custom hardware to which they apply, you can safely remove these repositories
from ``west.yml`` and remove the includes/function calls from the C code.

* `golioth-zephyr-boards`_ includes the board definitions for the Golioth
  Aludel-Mini
* `libostentus`_ is a helper library for controlling the Ostentus ePaper
  faceplate
* `zephyr-network-info`_ is a helper library for querying, formatting, and returning network
  connection information via Zephyr log or Golioth RPC

Using this template to start a new project
******************************************

Fork this template to create your own Reference Design. After checking out your fork, we recommend
the following workflow to pull in future changes:

* Setup

  * Create a ``template`` remote based on the Reference Design Template repository

* Merge in template changes

  * Fetch template changes and tags
  * Merge template release tag into your ``main`` (or other branch)
  * Resolve merge conflicts (if any) and commit to your repository

.. code-block:: shell

   # Setup
   git remote add template https://github.com/golioth/reference-design-template.git
   git fetch template --tags

   # Merge in template changes
   git fetch template --tags
   git checkout your_local_branch
   git merge template_v1.0.0

   # Resolve merge conflicts if necessary
   git add resolved_files
   git commit

.. _Golioth Console: https://console.golioth.io
.. _Nordic nRF9160 DK: https://www.nordicsemi.com/Products/Development-hardware/nrf9160-dk
.. _golioth-zephyr-boards: https://github.com/golioth/golioth-zephyr-boards
.. _libostentus: https://github.com/golioth/libostentus
.. _zephyr-network-info: https://github.com/golioth/zephyr-network-info
