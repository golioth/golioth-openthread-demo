..
   Copyright (c) 2024 Golioth, Inc.
   SPDX-License-Identifier: Apache-2.0

Golioth OpenThread Demo
#######################

This repository contains the firmware source code and `pre-built release
firmware images <releases_>`_ for the Golioth OpenThread Demo.

The full project details are available on the `Golioth Thread Demo Project Page`_.


Requirements
************

- Golioth device PSK credentials
- A running Thread Border Router with NAT64 translation (we will be using the
  commercially available off-the-shelf `GL-S200 Thread Border Router`_)
- Thread Network Name and Network Key
.. pull-quote::
   [!IMPORTANT]

   Do not clone this repo using git. Zephyr's ``west`` meta tool should be used to
   set up your local workspace.

Supported Hardware
******************

This firmware can be built for a variety of supported hardware platforms.

.. pull-quote::
   [!IMPORTANT]

   In Zephyr, each of these different hardware variants is given a unique
   "board" identifier, which is used by the build system to generate firmware
   for that variant.

   When building firmware using the instructions below, make sure to use the
   correct Zephyr board identifier that corresponds to your hardware platform.

.. list-table:: **Nordic Semiconductor Hardware**
   :header-rows: 1

   * - Development Borad
     - Zephyr Board

   * - .. image:: images/nRF52840_DK.png
          :width: 240
     - ``nrf52840dk_nrf52840``

.. list-table:: **Adafruit Hardware**
   :header-rows: 1

   * - Development Board
     - Zephyr Board

   * - .. image:: images/Adafurit_nRF52840_Feather.png
          :width: 240
     - ``adafruit_feather_nrf52840``

Firmware Overview
*****************
This is a Reference Design for a Thread Protocol enabled device using Zephyr
and connecting to Golioth over IPv6.

Configure ``prj.conf``
======================

Configure the following Kconfig options:

- OPENTHREAD_NETWORK_NAME       - Name of your Thread network
- OPENTHREAD_NETWORKKEY         - Network Key of your Thread network

by changing these lines in the ``prj.conf`` configuration file, e.g.:

.. code-block:: cfg

   CONFIG_OPENTHREAD_NETWORKKEY="00:11:22:33:44:55:66:77:88:99:aa:bb:cc:dd:ee:ff"
   CONFIG_OPENTHREAD_NETWORK_NAME="golioth-thread"

``CONFIG_OPENTHREAD_CHANNEL`` is set to ``26``.

.. pull-quote::
   [!IMPORTANT]

   Make sure the Thread Network Name, Thread Network Key and Thread Channel
   match your Border Router configuration.

Add Pipeline to Golioth
***********************

Golioth uses `Pipelines`_ to route stream data. This gives you flexibility to change your data
routing without requiring updated device firmware.

Whenever sending stream data, you must enable a pipeline in your Golioth project to configure how
that data is handled. Add the contents of ``pipelines/cbor-to-lightdb.yml`` as a new pipeline as
follows (note that this is the default pipeline for new projects and may already be present):

   1. Navigate to your project on the Golioth web console.
   2. Select ``Pipelines`` from the left sidebar and click the ``Create`` button.
   3. Give your new pipeline a name and paste the pipeline configuration into the editor.
   4. Click the toggle in the bottom right to enable the pipeline and then click ``Create``.

All data streamed to Golioth in CBOR format will now be routed to LightDB Stream and may be viewed
using the web console. You may change this behavior at any time without updating firmware simply by
editing this pipeline entry.

Golioth Features
****************

Supported Golioth Zephyr SDK Features
=====================================

This firmware implements the following features from the Golioth Zephyr SDK:

- `Device Settings Service <https://docs.golioth.io/firmware/zephyr-device-sdk/device-settings-service>`_
- `LightDB State Client <https://docs.golioth.io/firmware/zephyr-device-sdk/light-db/>`_
- `LightDB Stream Client <https://docs.golioth.io/firmware/zephyr-device-sdk/light-db-stream/>`_
- `Logging Client <https://docs.golioth.io/firmware/zephyr-device-sdk/logging/>`_
- `Over-the-Air (OTA) Firmware Upgrade <https://docs.golioth.io/firmware/device-sdk/firmware-upgrade>`_
- `Remote Procedure Call (RPC) <https://docs.golioth.io/firmware/zephyr-device-sdk/remote-procedure-call>`_

Device Settings Service
-----------------------

The following settings should be set in the Device Settings menu of the
`Golioth Console`_.

``LOOP_DELAY_S``
   Adjusts the delay between sensor readings. Set to an integer value (seconds).

   Default value is ``60`` seconds.

LightDB Stream Service
----------------------

An up-counting timer is periodically sent to the ``sensor/counter`` endpoint of the
LightDB Stream service to simulate sensor data.

LightDB State Service
---------------------

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

Remote Procedure Call (RPC) Service
-----------------------------------

The following RPCs can be initiated in the Remote Procedure Call menu of the
`Golioth Console`_.

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

Local set up
************

Do not clone this repo using git. Zephyr's ``west`` meta tool should be used to
set up your local workspace.

Install the Python virtual environment (recommended)
====================================================

.. code-block:: shell

   cd ~
   mkdir golioth-openthread-demo
   python -m venv golioth-openthread-demo/.venv
   source golioth-openthread-demo/.venv/bin/activate
   pip install wheel west

Use ``west`` to initialize the workspace and install dependencies
=================================================================

.. code-block:: console

   cd ~/golioth-openthread-demo
   west init -m git@github.com:golioth/golioth-openthread-demo.git .
   west update
   west zephyr-export
   pip install -r deps/zephyr/scripts/requirements.txt

Building the application
************************

Build the Zephyr sample application from the top-level workspace of your project.
After a successful build you will see a new ``build/`` directory.

Note that this git repository was cloned into the ``app`` folder, so any changes
you make to the application itself should be committed inside this repository.
The ``build`` and ``deps`` directories in the root of the workspace are managed
outside of this git repository by the ``west`` meta-tool.
.. code-block:: text

.. code-block:: text

   $ (.venv) west flash

OTA Firmware Update
*******************

This application includes the ability to perform Over-the-Air (OTA) firmware updates:

1. Update the version number in the `VERSION` file and perform a pristine (important) build to
   incorporate the version change.
2. Upload the `build/app/zephyr/zephyr.signed.bin` file as an artifact for your Golioth project
   using `main` as the package name.
3. Create and roll out a release based on this artifact.

Visit `the Golioth Docs OTA Firmware Upgrade page`_ for more info.

External Libraries
******************

In order for the device to securely authenticate with the Golioth Cloud, we need
to provision the device with a pre-shared key (PSK). This key will persist
across reboots and only needs to be set once after the device firmware has been
programmed. In addition, flashing new firmware images with ``west flash`` should
not erase these stored settings unless the entire device flash is erased.

Configure the PSK-ID and PSK using the device UART shell and reboot the device:

.. code-block:: text

   uart:~$ settings set golioth/psk-id <my-psk-id@my-project>
   uart:~$ settings set golioth/psk <my-psk>
   uart:~$ kernel reboot cold


Pulling in updates from the Reference Design Template
*****************************************************

This reference design was forked from the `Reference Design Template`_ repo. We
recommend the following workflow to pull in future changes:

* Setup

  * Create a ``template`` remote based on the Reference Design Template
    repository

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
.. _GL-S200 Thread Border Router: https://www.gl-inet.com/products/gl-s200/
.. _Nordic nRF52840 DK: https://www.nordicsemi.com/Products/Development-hardware/nRF52840-DK
.. _Golioth Thread Demo Project Page: https://projects.golioth.io/reference-designs/openthread-demo/
.. _releases: https://github.com/golioth/
.. _Zephyr Getting Started Guide: https://docs.zephyrproject.org/latest/develop/getting_started/
.. _Developer Training: https://training.golioth.io
.. _SemVer: https://semver.org
.. _Reference Design Template: https://github.com/golioth/reference-design-template
