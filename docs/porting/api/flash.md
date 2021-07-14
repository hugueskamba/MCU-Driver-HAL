# Flash

Steps to update target to support bootloader:
1. Update linker script.
1. Implement `mbed_start_application`.
1. Implement flash HAL API.
1. Verify changes with tests.

## Linker script updates

When building a bootloader application or an application that uses a bootloader, the MCU-Driver-HAL build system automatically defines values for the start of application flash (`MBED_APP_START`) and size of application flash (`MBED_APP_SIZE`) when pre-processing the linker script.  
When updating a target to support this functionality, linker scripts must place all flash data in a location starting at `MBED_APP_START` and must limit the size of that data to `MBED_APP_SIZE`. This change must occur for the linker scripts of all supported toolchains.

Please refer to existing SiP implementation for reference.

Use these 2 defines in place of flash start and size for a target:
- `MBED_APP_START` - defines an address where an application space starts.
- `MBED_APP_SIZE` - the size of the application.

---
**Note:**

When an application does not use any of the bootloader functionality, then `MBED_APP_START` and `MBED_APP_SIZE` are not defined. For this reason, the linker script must define default values that match flash start and flash size..</

---

An example of how a target could define `MBED_APP_START` and `MBED_APP_SIZE` in the linker script file:

```assembly
#if !defined(MBED_APP_START)
  #define MBED_APP_START MBED_ROM_START
#endif

#if !defined(MBED_APP_SIZE)
  #define MBED_APP_SIZE MBED_ROM_SIZE
#endif
```

---

**Warning:**

As these defines move the application flash sections, you should move any sections within flash sectors accordingly.

---

---

**Note:**

The VTOR must be relative to the region in which it is placed. To confirm, search for `NVIC_FLASH_VECTOR_ADDRESS` and `SCB->VTOR`, and ensure the flash address is not hardcoded.

---

Problematic declaration of flash VTOR address:

```assembly
#define NVIC_RAM_VECTOR_ADDRESS   (0x20000000)
#define NVIC_FLASH_VECTOR_ADDRESS (0x00000000)
```

Bootloader-ready declaration of flash VTOR address:

```assembly
#define NVIC_RAM_VECTOR_ADDRESS   (0x20000000)
#if defined(__ICCARM__)
    #pragma section=".intvec"
    #define NVIC_FLASH_VECTOR_ADDRESS   ((uint32_t)__section_begin(".intvec"))
#elif defined(__CC_ARM)
    extern uint32_t Load$$LR$$LR_IROM1$$Base[];
    #define NVIC_FLASH_VECTOR_ADDRESS   ((uint32_t)Load$$LR$$LR_IROM1$$Base)
#elif defined(__GNUC__)
    extern uint32_t vectors[];
    #define NVIC_FLASH_VECTOR_ADDRESS   ((uint32_t)vectors)
#else
    #error "Flash vector address not set for this toolchain"
#endif
```

## Start application

The `mbed_start_application` implementation exists only for Cortex-M3, Cortex-M4 and Cortex-M7. You can find it in [the Arm Mbed_application code file](https://github.com/mcu-driver-hal/MCU-Driver-HAL/blob/main/bootstrap/mbed_application.c). If `mbed_start_application` does not support your target, you must implement this function in the target HAL.

## Flash HAL

For a bootloader to perform updates, you must implement the flash API. This consists of implementing the function in [flash_api.h](https://github.com/mcu-driver-hal/MCU-Driver-HAL/blob/main/hal/include/hal/flash_api.h).

### Implement your own HAL driver

Functions to implement:

```c
int32_t flash_init(flash_t *obj);
int32_t flash_free(flash_t *obj);
int32_t flash_erase_sector(flash_t *obj, uint32_t address);
int32_t flash_program_page(flash_t *obj, uint32_t address, const uint8_t *data, uint32_t size);
uint32_t flash_get_sector_size(const flash_t *obj, uint32_t address);
uint32_t flash_get_page_size(const flash_t *obj);
uint32_t flash_get_start_address(const flash_t *obj);
uint32_t flash_get_size(const flash_t *obj);
```

## Tests

The tests for the `FlashIAP` flash HAL is available at `tests/mbed_hal/flash`.

It test all flash API functionality.
Steps to run the tests will be provided in the future.

## Troubleshooting

- For targets with `VTOR`, a target might have a `VTOR` address defined to a hardcoded address as mentioned in the [Linker script updates](#linker-script-updates) section.

- Using Flash IAP might introduce latency as it might disable interrupts for longer periods of time.

- Program and erase functions might operate on different sized blocks - page size might not equal to a sector size. The function erase erases a sector, the program function programs a page. Use accessor methods to get the values for a sector or a page.

- Sectors might have different sizes within a device.
