# Copyright (c) 2020-2021 Arm Limited
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# add_subdirectory(usb)

target_include_directories(mbed-core
    INTERFACE
        include
        include/hal
)

target_sources(mbed-core
    INTERFACE
        source/mbed_compat.c
        source/mbed_critical_section_api.c
        source/mbed_flash_api.c
        source/mbed_gpio.c
        source/mbed_gpio_irq.c
        # source/mbed_itm_api.c
        source/mbed_pinmap_common.c
        source/mbed_pinmap_default.cpp
        source/mbed_us_ticker_api.c
        source/mbed_lp_ticker_api.c
        # source/static_pinmap.cpp

        source/mpu/mbed_mpu_v7m.c
        source/mpu/mbed_mpu_v8m.c
)
