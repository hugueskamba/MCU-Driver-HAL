#!/usr/bin/env python
# Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
import shutil
import subprocess
import sys
from enum import Enum
from pathlib import Path, PurePosixPath
from typing import Generator


log = logging.getLogger(__name__)

LOG_FORMAT = "%(asctime)s - %(levelname)s - %(message)s"
CMAKE_BUILD_DIR = "cmake_build"
SUPPORTED_TOOLCHAIN = ["ARM", "GCC_ARM"]
DEFAULT_BAUD_RATE = 115200


class ReturnCode(Enum):
    """Return codes."""

    SUCCESS = 0
    ERROR = 1
    INVALID_OPTIONS = 2


class UnsupportedToolchainError(Exception):
    """An exception for an invalid toolchain."""


class ConfigureBuildTestError(Exception):
    """An exception for a failure to configure a build for a test."""


class BuildTestError(Exception):
    """An exception for a failure to build a test."""


class RunTestError(Exception):
    """An exception for a failure to run a test."""


class ArgumentParserWithDefaultHelp(argparse.ArgumentParser):
    """Subclass that always shows the help message on invalid arguments."""

    def error(self, message):
        """Error handler."""
        sys.stderr.write("error: {}\n".format(message))
        self.print_help()
        raise SystemExit(ReturnCode.INVALID_OPTIONS.value)


def execute(cmd: list[str]) -> Generator[str, None, None]:
    """Execute a command and yield each line of the subprocess."""
    popen = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, universal_newlines=True
    )
    for stdout_line in iter(popen.stdout.readline, ""):
        yield stdout_line
    popen.stdout.close()
    return_code = popen.wait()
    if return_code:
        raise subprocess.CalledProcessError(return_code, cmd)


def run_configure_build_cmd(
    binary_tree: str, toolchain: str, profile: str
) -> None:
    """Run a subprocess to build a test."""
    cmd = [
        "cmake",
        "-S",
        f"{binary_tree}",
        "-B",
        f"{PurePosixPath(binary_tree).joinpath(CMAKE_BUILD_DIR)}",
        "-GNinja",
        "-DGREENTEA_CLIENT_STDIO=OFF",
        f"-DMBED_TOOLCHAIN={toolchain}",
        f"-DCMAKE_BUILD_TYPE={profile}",
        f"--log-level=ERROR",
    ]
    log.debug(f"command: '{cmd}'")

    try:
        for path in execute(cmd):
            print(path, end="")
    except OSError as error:
        raise ConfigureBuildTestError(f"Error: {error}")


def run_build_cmd(binary_tree: str) -> None:
    """Run a subprocess to build a test."""
    cmd = [
        "cmake",
        "--build",
        f"{PurePosixPath(binary_tree).joinpath(CMAKE_BUILD_DIR)}",
    ]
    log.debug("command: '{}'".format(cmd))

    try:
        for path in execute(cmd):
            print(path, end="")
    except (OSError, subprocess.CalledProcessError) as error:
        raise BuildTestError(f"Error: {error}")


def remove_subdirectories_with_name(root_dir: str, name: str) -> None:
    """Remove all directories with a given name."""
    for build_dir in Path(root_dir).glob(f"**/{name}"):
        log.debug(f"Deleting: {build_dir}")
        shutil.rmtree(build_dir)


def build_all_tests_action(args: argparse.Namespace) -> None:
    """Build all Greentea tests."""
    if args.toolchain not in SUPPORTED_TOOLCHAIN:
        raise UnsupportedToolchainError(
            f"`{args.toolchain}` is not supported. Use {SUPPORTED_TOOLCHAIN}"
        )

    if args.clean:
        remove_subdirectories_with_name(args.tests_directory, CMAKE_BUILD_DIR)

    for cmake_list_file in Path(args.tests_directory).glob("**/CMakeLists.txt"):
        test_root_dir = Path(cmake_list_file).parents[0]
        run_configure_build_cmd(test_root_dir, args.toolchain, args.profile)
        run_build_cmd(test_root_dir)


def clean_all_tests_action(args: argparse.Namespace) -> None:
    """Remove all Greentea test build subdirectories."""
    remove_subdirectories_with_name(*args.tests_directory, CMAKE_BUILD_DIR)


def run_all_tests_action(args: argparse.Namespace) -> None:
    """..."""
    for binary in Path(args.tests_directory).glob("**/sdfx-arm-test-*.bin"):
        cmd = [
            "mbedhtrun",
            "--image-path",
            f"{binary}",
            "--enum-host-tests",
            f"{args.enum_host_tests}",
            "--disk",
            f"{args.disk}",
            "--port",
            f"{args.port}:{args.baud}",
        ]
        if args.skip_flashing:
            cmd.extend("--skip-flashing")

        log.debug("command: '{}'".format(cmd))

        try:
            for path in execute(cmd):
                print(path, end="")
        except (OSError, subprocess.CalledProcessError) as error:
            raise RunTestError(f"Error: {error}")


def parse_args() -> argparse.Namespace:
    """Parse the command line args."""
    parser = ArgumentParserWithDefaultHelp(
        description="MBL application manager",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "tests_directory",
        type=str,
        help="root of directory containg subdirectories with tests CMakeLists.txt files to run.",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="increase verbosity of status information",
    )

    command_group = parser.add_subparsers(
        description="The commands to perform certain tasks"
    )

    build_all = command_group.add_parser(
        "build_all", help="build all Greentea test binaries."
    )
    build_all.add_argument(
        "-t",
        "--toolchain",
        type=str,
        required=True,
        help=f"the toolchain {SUPPORTED_TOOLCHAIN} you are using to build the Greentea tests.",
    )
    build_all.add_argument(
        "-b",
        "--profile",
        type=str,
        default="develop",
        help="the build type (release, develop or debug).",
    )
    build_all.add_argument(
        "-c", "--clean", action="store_true", help="perform a clean build."
    )
    build_all.set_defaults(func=build_all_tests_action)

    clean_all = command_group.add_parser(
        "clean_all", help="remove all Greentea test binaries."
    )
    clean_all.set_defaults(func=clean_all_tests_action)

    run_all = command_group.add_parser(
        "run_all", help="run all Greentea test binaries."
    )
    run_all.add_argument(
        "-e",
        "--enum-host-tests",
        type=str,
        required=True,
        help="directory with local host tests.",
    )
    run_all.add_argument(
        "-p",
        "--port",
        type=str,
        required=True,
        help="serial port of the target hardware.",
    )
    run_all.add_argument(
        "-d", "--disk", type=str, required=True, help="target mount point."
    )
    run_all.add_argument(
        "-b",
        "--baud",
        type=int,
        default=DEFAULT_BAUD_RATE,
        help=f"baud rate for serial port connected to the target hardware (defaut={DEFAULT_BAUD_RATE}).",
    )
    run_all.add_argument(
        "--skip-flashing",
        action="store_true",
        help="path to copy the binaries found on the target hardware.",
    )
    run_all.set_defaults(func=run_all_tests_action)

    args_namespace = parser.parse_args()

    # We want to fail gracefully, with a consistent
    # help message, in the no argument case.
    # So here's an obligatory hasattr hack.
    if not hasattr(args_namespace, "func"):
        parser.error("No arguments given!")
    else:
        return args_namespace


def set_log_verbosity(increase_verbosity: bool) -> None:
    """Set the verbosity of the log output."""
    log_level = logging.DEBUG if increase_verbosity else logging.INFO

    log.setLevel(log_level)
    logging.basicConfig(level=log_level, format=LOG_FORMAT)


def run_helper() -> None:
    """Application main algorithm."""
    args = parse_args()

    set_log_verbosity(args.verbose)

    log.debug("Starting helper")
    log.debug("Command line arguments:{}".format(args))

    args.func(args)


def _main() -> int:
    """Run test helper."""
    try:
        run_helper()
    except Exception as error:
        print(error)
        return ReturnCode.ERROR.value
    else:
        return ReturnCode.SUCCESS.value


if __name__ == "__main__":
    sys.exit(_main())
