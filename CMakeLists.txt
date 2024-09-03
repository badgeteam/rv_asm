# SPDX-License-Identidier: MIT

cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(riscv-assembler)

add_executable(rvasm
    src/bss.c
    src/comp.c
    src/data.c
    src/main.c
    src/util.c
    src/export.c
    src/token.c
    src/relocation.c
    src/section.c
    src/symbol.c
    src/constants.c
    src/riscv.c
    src/riscv_zicsr.c
    src/riscv_c.c
)

target_include_directories(rvasm PRIVATE include)
