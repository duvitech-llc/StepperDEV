#!/usr/bin/env python3
"""
Simple generator to populate .vscode JSON templates with platform-specific values
based on .vscode/platform.json. It replaces placeholders in the existing
.vscode JSON files and writes updated files back.

Placeholders supported:
- ${TOOLCHAIN_GCC}
- ${GDB_PATH}
- ${OPENOCD_PATH}
- ${BUILD_DIR}
- ${COMPILE_COMMANDS}

Run from the workspace root: python3 .vscode/generate_vscode.py
"""
import json
import os
import platform
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
PLATFORM_FILE = os.path.join(ROOT, '.vscode', 'platform.json')
TEMPLATES = ['c_cpp_properties.json', 'launch.json', 'tasks.json']

def load_platform_config():
    with open(PLATFORM_FILE, 'r', encoding='utf-8') as f:
        cfg = json.load(f)
    sysname = platform.system().lower()
    if 'linux' in sysname:
        key = 'linux'
    elif 'windows' in sysname:
        key = 'windows'
    else:
        key = 'linux'
    return cfg.get(key, cfg.get('linux'))

def replace_placeholders(text, mapping):
    for k, v in mapping.items():
        text = text.replace('${' + k + '}', v)
    return text

def main():
    cfg = load_platform_config()
    mapping = {
        'TOOLCHAIN_GCC': cfg.get('toolchain_gcc', ''),
        'GDB_PATH': cfg.get('gdb', ''),
        'OPENOCD_PATH': cfg.get('openocd', ''),
        'BUILD_DIR': cfg.get('build_dir', 'build/Debug'),
        'COMPILE_COMMANDS': cfg.get('compile_commands', '${workspaceFolder}/build/Debug/compile_commands.json')
    }

    vscode_dir = os.path.join(ROOT, '.vscode')

    for name in TEMPLATES:
        path = os.path.join(vscode_dir, name)
        if not os.path.exists(path):
            print('Skipping missing template:', name)
            continue
        with open(path, 'r', encoding='utf-8') as f:
            text = f.read()
        new_text = replace_placeholders(text, mapping)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new_text)
        print('Wrote', path)

    print('Generation complete. Reload window in VS Code if necessary.')

if __name__ == '__main__':
    main()
