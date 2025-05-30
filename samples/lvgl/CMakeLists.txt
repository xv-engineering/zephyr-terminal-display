# Copyright (c) 2025, Noah Luskey <noah@xv.engineering>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(terminal-display-tests)
target_sources(app PRIVATE
    main.c
    particle.c
)
