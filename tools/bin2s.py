#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 the starfleet2-3ds authors
"""Embed a binary file for the ARM build (replaces devkitPro's bin2s for the UI graphics).

usage: bin2s.py INPUT.bgr OUT.s OUT.h
Symbols follow bin2s: <name>, <name>_end, <name>_size, where name is the file name with
'.' replaced by '_' (ctr_bottom_idle.bgr -> ctr_bottom_idle_bgr).
"""
import os
import sys

src, out_s, out_h = sys.argv[1], sys.argv[2], sys.argv[3]
data = open(src, 'rb').read()
name = os.path.basename(src).replace('.', '_')
if name[0].isdigit():
    name = '_' + name
with open(out_s, 'w') as f:
    f.write('  .section .rodata\n  .balign 4\n')
    f.write(f'  .global {name}\n  .global {name}_end\n  .global {name}_size\n')
    f.write(f'{name}:\n')
    for i in range(0, len(data), 16):
        f.write('  .byte ' + ','.join(str(b) for b in data[i:i + 16]) + '\n')
    f.write(f'{name}_end:\n  .balign 4\n{name}_size: .int {len(data)}\n')
with open(out_h, 'w') as f:
    f.write('#pragma once\n')
    f.write(f'extern const unsigned char {name}[];\n')
    f.write(f'extern const unsigned char {name}_end[];\n')
    f.write(f'extern const unsigned int {name}_size;\n')
