# MCU-Driver-HAL Tests

This directory contains the Greentea tests to evaluate the implementation of the MCU-Drive-HAL implementation on hardware.

Each test build is to be provided access to hardware specific source code and to be linked with as a library to an executable.

A Python script has been provided to facilitate building and running the tests on hardware.
The script is located [here](./greentea_helper.py).

## Building the test

1. Run the following command:
    ```
    $ python /path/to/greentea_helper.py -v /path/to/tests/executable/root/directory build_all -t <TOOLCHAIN> --clean
    ```

Outcome: All tests for which an executable CMake build instructions have been provided in the  implementation repository are built.

## Running the test

1. Run the following command:
    ```
    $ python /path/to/greentea_helper.py -v /path/to/tests/executable/root/directory run_all -d . -p /path/to/device/mount/point -e /path/to/local/host/tests/
    ```

Outcome: All the test binaries previously built are programmed into the device and ran one by one producing an output with the test results.

Add the optional argument `--skip-flashing` to not program the binaries on the harware under test.
