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

   * - .. image:: images/Adafruit_nRF52840_Feather.png
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
   CONFIG_OPENTHREAD_CHANNEL=26
   CONFIG_OPENTHREAD_PANID=34739

.. pull-quote::
   [!IMPORTANT]

   Make sure the Thread Network Name, Thread Network Key, Thread Channel and
   Thread PAN ID match your Border Router configuration.


Local set up
************

.. pull-quote::
   [!IMPORTANT]

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

Build the Zephyr sample application for the `Nordic nRF52840 DK`_
(``nrf52840dk_nrf52840``) from the top level of your project. After a
successful build you will see a new ``build`` directory. Note that any changes
(and git commits) to the project itself will be inside the ``app`` folder. The
``build`` and ``deps`` directories being one level higher prevents the repo from
cataloging all of the changes to the dependencies and the build (so no
``.gitignore`` is needed).

Prior to building, update ``VERSION`` file to reflect the firmware version number you want to assign
to this build. Then run the following commands to build and program the firmware onto the device.


.. pull-quote::
   [!IMPORTANT]

   You must perform a pristine build (use ``-p`` or remove the ``build`` directory)
   after changing the firmware version number in the ``VERSION`` file for the change to take effect.

.. code-block:: text

   $ (.venv) west build -p -b nrf52840dk/nrf52840 --sysbuild app
   $ (.venv) west flash

Configure PSK-ID and PSK using the device shell based on your Golioth
credentials and reboot:

.. code-block:: text

   uart:~$ settings set golioth/psk-id <my-psk-id@my-project>
   uart:~$ settings set golioth/psk <my-psk>
   uart:~$ kernel reboot cold

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

This app currently implements Over-the-Air (OTA) firmware updates, Settings
Service, Logging, RPC, and both LightDB State and LightDB Stream data.

Settings Service
----------------

The following settings should be set in the Device Settings menu of the
`Golioth Console`_.

``LOOP_DELAY_S``
   Adjusts the delay between sensor readings. Set to an integer value (seconds).

   Default value is ``60`` seconds.

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

LightDB State and LightDB Stream data
=====================================

Time-Series Data (LightDB Stream)
---------------------------------

An up-counting timer is periodically sent to the ``sensor/counter`` endpoint of the
LightDB Stream service to simulate sensor data.

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

This reference design may be built for a variety of different boards.

Prior to building, update ``VERSION`` file to reflect the firmware version number you want to assign
to this build. Then run the following commands to build and program the firmware onto the device.

Adafruit Feather nRF52840 Express
=================================

This reference design may be built for the Adafruit Feather nRF52840 Express board.

.. code-block:: text

   $ (.venv) west build -p -b adafruit_feather_nrf52840/nrf52840 --sysbuild app
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
