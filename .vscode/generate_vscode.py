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

# Embedded templates — the generator will write these files with platform values
EMBED_TEMPLATES = {
    'c_cpp_properties.json': '''{
    "configurations": [
        {
            "name": "Auto",
            "includePath": [
                "${workspaceFolder}/**"
            ],
            "defines": [
                "STM32L476xx",
                "USE_HAL_DRIVER"
            ],
            "compilerPath": "${TOOLCHAIN_GCC}",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "gcc-arm",
            "compileCommands": "${COMPILE_COMMANDS}",
            "configurationProvider": "ms-vscode.cmake-tools"
        }
    ],
    "version": 4
}
''',

    'launch.json': '''{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug (OpenOCD)",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/${BUILD_DIR}/StepperDEV",
            "args": [],
            "stopAtEntry": true,
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "debugServerArgs": "-f interface/stlink.cfg -f target/stm32l4x.cfg",
            "serverStarted": "Info : Listening on port [0-9]+ for gdb connection",
            "filterStderr": true,
            "filterStdout": false,
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "description": "Connect to target",
                    "text": "target extended-remote localhost:3333",
                    "ignoreFailures": false
                },
                {
                    "description": "Reset and halt target",
                    "text": "monitor reset halt",
                    "ignoreFailures": false
                }
            ],
            "preLaunchTask": "Flash Firmware",
            "postDebugTask": "",
            "miDebuggerPath": "${GDB_PATH}",
            "debugServerPath": "${OPENOCD_PATH}"
        },
        {
            "name": "Attach (OpenOCD)",
            "type": "cppdbg",
            "request": "attach",
            "program": "${workspaceFolder}/${BUILD_DIR}/StepperDEV",
            "MIMode": "gdb",
            "miDebuggerServerAddress": "localhost:3333",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "miDebuggerPath": "${GDB_PATH}"
        }
    ]
}
''',

    'tasks.json': '''{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "CMake: Configure (Debug)",
            "type": "shell",
            "command": "cmake",
            "args": [
                "--preset",
                "Debug"
            ],
            "group": "build",
            "problemMatcher": []
        },
        {
            "label": "CMake: Build (Debug)",
            "type": "shell",
            "command": "cmake",
            "args": [
                "--build",
                "${workspaceFolder}/${BUILD_DIR}",
                "--config",
                "Debug",
                "--target",
                "all",
                "-j",
                "10",
                "--verbose"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": [
                "$gcc"
            ],
            "dependsOn": [
                "CMake: Configure (Debug)"
            ]
        },
        {
            "label": "CMake: Clean",
            "type": "shell",
            "command": "cmake",
            "args": [
                "--build",
                "${workspaceFolder}/${BUILD_DIR}",
                "--target",
                "clean"
            ],
            "group": "build",
            "problemMatcher": []
        },
        {
            "label": "CMake: Build (Release)",
            "type": "shell",
            "command": "cmake",
            "args": [
                "--build",
                "${workspaceFolder}/build/Release",
                "--config",
                "Release",
                "--target",
                "all",
                "-j",
                "10"
            ],
            "group": "build",
            "problemMatcher": [
                "$gcc"
            ]
        },
        {
            "label": "Flash Firmware",
            "type": "shell",
            "command": "${OPENOCD_PATH}",
            "args": [
                "-f",
                "interface/stlink.cfg",
                "-f",
                "target/stm32l4x.cfg",
                "-c",
                "program ${BUILD_DIR}/StepperDEV.hex reset exit"
            ],
            "group": "build",
            "problemMatcher": [],
            "dependsOn": [
                "CMake: Build (Debug)"
            ],
            "options": {
                "cwd": "${workspaceFolder}"
            }
        }
    ]
}
'''
}

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

    # Ensure .vscode directory exists
    os.makedirs(vscode_dir, exist_ok=True)

    for name in TEMPLATES:
        template = EMBED_TEMPLATES.get(name)
        if template is None:
            print('No embedded template for', name)
            continue
        new_text = replace_placeholders(template, mapping)
        path = os.path.join(vscode_dir, name)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new_text)
        print('Wrote', path)

    print('Generation complete. Reload window in VS Code if necessary.')

if __name__ == '__main__':
    main()
