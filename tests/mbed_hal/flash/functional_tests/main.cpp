/* Copyright (c) 2017-2021 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"
#include "greentea-custom_io/custom_io.h"
#include "bootstrap/mbed_mpu_mgmt.h"

#include "bootstrap/mbed_critical.h"
#include "bootstrap/mbed_toolchain.h"
#include "hal/flash_api.h"
#include "hal/us_ticker_api.h"

#if defined(TOOLCHAIN_GCC_ARM)
    extern uint32_t __etext;
    extern uint32_t __data_start__;
    extern uint32_t __data_end__;
    #define FLASHIAP_APP_ROM_END_ADDR (((uint32_t) &__etext) + ((uint32_t) &__data_end__) - ((uint32_t) &__data_start__))
#elif defined(TOOLCHAIN_ARM)
    extern uint32_t Load$$LR$$LR_IROM1$$Limit[];
    #define FLASHIAP_APP_ROM_END_ADDR ((uint32_t)Load$$LR$$LR_IROM1$$Limit)
#endif

using namespace utest::v1;

#define TEST_CYCLES         10000000

#define ALLOWED_DRIFT_PPM   (1000000U/5000U)    //0.5%

#define US_TICKER_OV_LIMIT 35000

/*
    return values to be checked are documented at:
        http://arm-software.github.io/CMSIS_5/Pack/html/algorithmFunc.html#Verify
*/

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a) ((x)& ~((a) - 1))
#endif

static uint32_t timer_diff_start;


#if  defined ( __GNUC__ ) ||  defined(__ARMCC_VERSION)
MBED_NOINLINE
static void delay_loop(uint32_t count)
{
    __asm__ volatile(
        "%=:\n\t"
#if defined(__thumb__) && !defined(__thumb2__) && !defined(__ARMCC_VERSION)
        "SUB  %0, #1\n\t"
#else
        "SUBS %0, %0, #1\n\t"
#endif
        "BCS  %=b\n\t"
        : "+l"(count)
        :
        : "cc"
    );
}
#endif

/* Since according to the ticker requirements min acceptable counter size is
 * - 12 bits for low power timer - max count = 4095,
 * - 16 bits for high frequency timer - max count = 65535
 * then all test cases must be executed in this time windows.
 * HAL ticker layer handles counter overflow and it is not handled in the target
 * ticker drivers. Ensure we have enough time to execute test case without overflow.
 */
void overflow_protect()
{
    uint32_t time_window = US_TICKER_OV_LIMIT;
    const ticker_interface_t *intf = get_us_ticker_data()->interface;

    const uint32_t ticks_now = intf->read();
    const ticker_info_t *p_ticker_info = intf->get_info();

    const uint32_t max_count = ((1 << p_ticker_info->bits) - 1);

    if ((max_count - ticks_now) > time_window) {
        return;
    }

    while (intf->read() >= ticks_now);
}

MBED_NOINLINE
static uint32_t time_cpu_cycles(uint32_t cycles)
{
    core_util_critical_section_enter();

    uint32_t start = (us_ticker_read() * 1000000) / us_ticker_get_info()->frequency;

    delay_loop(cycles);

    uint32_t end = (us_ticker_read() * 1000000) / us_ticker_get_info()->frequency;

    core_util_critical_section_exit();

    return (end - start);
}

void flash_init_test()
{
    overflow_protect();

    timer_diff_start = time_cpu_cycles(TEST_CYCLES);

    flash_t test_flash;
    int32_t ret = flash_init(&test_flash);
    TEST_ASSERT_EQUAL_INT32(0, ret);
    ret = flash_free(&test_flash);
    TEST_ASSERT_EQUAL_INT32(0, ret);
}

void flash_mapping_alignment_test()
{
    flash_t test_flash;
    int32_t ret = flash_init(&test_flash);
    TEST_ASSERT_EQUAL_INT32(0, ret);

    const uint32_t page_size = flash_get_page_size(&test_flash);
    const uint32_t flash_start = flash_get_start_address(&test_flash);
    const uint32_t flash_size = flash_get_size(&test_flash);
    TEST_ASSERT_TRUE(page_size != 0UL);

    uint32_t sector_size = flash_get_sector_size(&test_flash, flash_start);
    for (uint32_t offset = 0; offset < flash_size;  offset += sector_size) {
        const uint32_t sector_start = flash_start + offset;
        sector_size = flash_get_sector_size(&test_flash, sector_start);
        const uint32_t sector_end = sector_start + sector_size - 1;
        const uint32_t end_sector_size = flash_get_sector_size(&test_flash, sector_end);

        // Sector size must be a valid value
        TEST_ASSERT_NOT_EQUAL(MBED_FLASH_INVALID_SIZE, sector_size);
        // Sector size must be greater than zero
        TEST_ASSERT_NOT_EQUAL(0, sector_size);
        // All flash sectors must be a multiple of page size
        TEST_ASSERT_EQUAL(0, sector_size % page_size);
        // Sector address must be a multiple of sector size
        TEST_ASSERT_EQUAL(0, sector_start % sector_size);
        // All address in a sector must return the same sector size
        TEST_ASSERT_EQUAL(sector_size, end_sector_size);
    }

    // Make sure unmapped flash is reported correctly
    TEST_ASSERT_EQUAL(MBED_FLASH_INVALID_SIZE, flash_get_sector_size(&test_flash, flash_start - 1));
    TEST_ASSERT_EQUAL(MBED_FLASH_INVALID_SIZE, flash_get_sector_size(&test_flash, flash_start + flash_size));

    ret = flash_free(&test_flash);
    TEST_ASSERT_EQUAL_INT32(0, ret);
}

void flash_erase_sector_test()
{
    flash_t test_flash;
    int32_t ret = flash_init(&test_flash);
    TEST_ASSERT_EQUAL_INT32(0, ret);

    uint32_t addr_after_last = flash_get_start_address(&test_flash) + flash_get_size(&test_flash);
    uint32_t last_sector_size = flash_get_sector_size(&test_flash, addr_after_last - 1);
    uint32_t last_sector = addr_after_last - last_sector_size;
    TEST_ASSERT_EQUAL_INT32(0, last_sector % last_sector_size);

    utest_printf("ROM ends at 0x%lx, test starts at 0x%lx\n", FLASHIAP_APP_ROM_END_ADDR, last_sector);
    TEST_SKIP_UNLESS_MESSAGE(last_sector >= FLASHIAP_APP_ROM_END_ADDR, "Test skipped. Test region overlaps code.");

    ret = flash_erase_sector(&test_flash, last_sector);
    TEST_ASSERT_EQUAL_INT32(0, ret);

    ret = flash_free(&test_flash);
    TEST_ASSERT_EQUAL_INT32(0, ret);
}

// Erase sector, write one page, erase sector and write new data
void flash_program_page_test()
{
    flash_t test_flash;
    int32_t ret = flash_init(&test_flash);
    TEST_ASSERT_EQUAL_INT32(0, ret);

    uint32_t test_size = flash_get_page_size(&test_flash);
    uint8_t *data = new uint8_t[test_size];
    uint8_t *data_flashed = new uint8_t[test_size];
    for (uint32_t i = 0; i < test_size; i++) {
        data[i] = 0xCE;
    }

    // the one before the last page in the system
    uint32_t address = flash_get_start_address(&test_flash) + flash_get_size(&test_flash) - (2 * test_size);

    // sector size might not be same as page size
    uint32_t erase_sector_boundary = ALIGN_DOWN(address, flash_get_sector_size(&test_flash, address));
    utest_printf("ROM ends at 0x%lx, test starts at 0x%lx\n", FLASHIAP_APP_ROM_END_ADDR, erase_sector_boundary);
    TEST_SKIP_UNLESS_MESSAGE(erase_sector_boundary >= FLASHIAP_APP_ROM_END_ADDR, "Test skipped. Test region overlaps code.");

    ret = flash_erase_sector(&test_flash, erase_sector_boundary);
    TEST_ASSERT_EQUAL_INT32(0, ret);

    ret = flash_program_page(&test_flash, address, data, test_size);
    TEST_ASSERT_EQUAL_INT32(0, ret);

    ret = flash_read(&test_flash, address, data_flashed, test_size);
    TEST_ASSERT_EQUAL_INT32(0, ret);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, data_flashed, test_size);

    // sector size might not be same as page size
    erase_sector_boundary = ALIGN_DOWN(address, flash_get_sector_size(&test_flash, address));
    ret = flash_erase_sector(&test_flash, erase_sector_boundary);
    TEST_ASSERT_EQUAL_INT32(0, ret);

    // write another data to be certain we are re-flashing
    for (uint32_t i = 0; i < test_size; i++) {
        data[i] = 0xAC;
    }
    ret = flash_program_page(&test_flash, address, data, test_size);
    TEST_ASSERT_EQUAL_INT32(0, ret);

    ret = flash_read(&test_flash, address, data_flashed, test_size);
    TEST_ASSERT_EQUAL_INT32(0, ret);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, data_flashed, test_size);

    ret = flash_free(&test_flash);
    TEST_ASSERT_EQUAL_INT32(0, ret);
    delete[] data;
    delete[] data_flashed;
}

// check the execution speed at the start and end of the test to make sure
// cache settings weren't changed
void flash_clock_and_cache_test()
{
    overflow_protect();

    const uint32_t timer_diff_end = time_cpu_cycles(TEST_CYCLES);
    const uint32_t acceptable_range = timer_diff_start / (ALLOWED_DRIFT_PPM);
    TEST_ASSERT_UINT32_WITHIN(acceptable_range, timer_diff_start, timer_diff_end);
}

Case cases[] = {
    Case("Flash - init", flash_init_test),
    Case("Flash - mapping alignment", flash_mapping_alignment_test),
    Case("Flash - erase sector", flash_erase_sector_test),
    Case("Flash - program page", flash_program_page_test),
    Case("Flash - clock and cache test", flash_clock_and_cache_test),
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases)
{
    mbed_mpu_manager_lock_ram_execution();
    mbed_mpu_manager_lock_rom_write();

    us_ticker_init();

    GREENTEA_SETUP(20, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

void greentea_test_teardown(const size_t passed, const size_t failed, const failure_t failure)
{
    mbed_mpu_manager_unlock_ram_execution();
    mbed_mpu_manager_unlock_rom_write();

    greentea_test_teardown_handler(passed, failed, failure);
}

Specification specification(greentea_test_setup, cases, greentea_test_teardown);

int main()
{
    greentea_init_custom_io();
    Harness::run(specification);
}
