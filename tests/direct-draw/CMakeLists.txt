# Copyright (c) 2025 Noah Luskey <noah@xv.engineering>
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED)

project(terminal-display-direct-draw-tests)
target_sources(app PRIVATE main.c utils.c)