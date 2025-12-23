# StepperDEV - STM32L476 Development Project

STM32L476 firmware project with TMC5240 stepper motor driver support.

## Prerequisites

### Common Requirements
- Git
- CMake (version 3.22 or higher)
- ARM GCC Toolchain (version 10 or higher)
- OpenOCD (for flashing and debugging)
- VS Code with recommended extensions:
  - C/C++ (ms-vscode.cpptools)
  - CMake Tools (ms-vscode.cmake-tools)
  - Cortex-Debug (marus25.cortex-debug) - optional but recommended

### Windows Installation

#### 1. Install ARM GCC Toolchain
Download and install from: https://developer.arm.com/downloads/-/gnu-rm

**Common installation paths:**
- `C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin`
- `C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\13.2 Rel1\bin`

Add the `bin` directory to your system PATH.

**Verify installation:**
```powershell
arm-none-eabi-gcc --version
```

#### 2. Install CMake
Download from: https://cmake.org/download/

Or use Chocolatey:
```powershell
choco install cmake
```

#### 3. Install Ninja Build System (Optional but recommended)
```powershell
choco install ninja
```

#### 4. Install OpenOCD
**Option 1: Using Chocolatey**
```powershell
choco install openocd
```

**Option 2: Using winget**
```powershell
winget install openocd
```

**Option 3: Manual Installation**
Download from: https://github.com/xpack-dev-tools/openocd-xpack/releases

Extract and add to PATH.

**Option 4: Use STM32CubeIDE's OpenOCD** (if installed)
```
C:\ST\STM32CubeIDE_[version]\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.openocd.win32_[version]\tools\bin\openocd.exe
```

**Verify installation:**
```powershell
openocd --version
```

### Linux Installation

#### Ubuntu/Debian

```bash
# Install ARM GCC Toolchain
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi

# Install build tools
sudo apt install cmake ninja-build

# Install OpenOCD
sudo apt install openocd

# Install STLink utilities (optional)
sudo apt install stlink-tools

# Verify installations
arm-none-eabi-gcc --version
cmake --version
openocd --version
```

#### Fedora/RHEL

```bash
# Install ARM GCC Toolchain
sudo dnf install arm-none-eabi-gcc-cs arm-none-eabi-binutils

# Install build tools
sudo dnf install cmake ninja-build

# Install OpenOCD
sudo dnf install openocd

# Verify installations
arm-none-eabi-gcc --version
cmake --version
openocd --version
```

#### Arch Linux

```bash
# Install toolchain and tools
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib
sudo pacman -S cmake ninja openocd

# Verify installations
arm-none-eabi-gcc --version
cmake --version
openocd --version
```

## Building the Project

### Clone the Repository
```bash
git clone <repository-url>
cd StepperDEV
```

### Configure and Build

#### Using VS Code
1. Open the project folder in VS Code
2. CMake will automatically configure the project
3. Press `Ctrl+Shift+B` to build (or use `CMake: Build` command)
4. Build output will be in `build/Debug/`

#### Using Command Line

**Linux:**
```bash
# Configure
cmake --preset Debug

# Build
cmake --build build/Debug --config Debug --target all -j 10
```

**Windows (PowerShell):**
```powershell
# Configure
cmake --preset Debug

# Build
cmake --build build/Debug --config Debug --target all -j 10
```

### Build Output
The build process generates:
- `build/Debug/StepperDEV` - ELF executable
- `build/Debug/StepperDEV.hex` - Intel HEX format
- `build/Debug/StepperDEV.bin` - Raw binary format

## Flashing and Debugging

### Hardware Setup
1. Connect ST-Link V2 debugger to your STM32L476 board
2. Ensure the board is powered
3. Connect ST-Link to your computer via USB

### Verify Connection
Test the connection with OpenOCD:

**Linux:**
```bash
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg
```

**Windows:**
```powershell
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg
```

You should see output indicating successful connection to the STM32L476.

### Flashing Firmware

#### Using VS Code
Use the built-in task:
1. Press `Ctrl+Shift+P`
2. Type "Tasks: Run Task"
3. Select "Flash Firmware"

#### Using Command Line

**Linux:**
```bash
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
  -c "program build/Debug/StepperDEV.hex reset exit"
```

**Windows:**
```powershell
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg `
  -c "program build/Debug/StepperDEV.hex reset exit"
```

### Debugging in VS Code

#### Available Debug Configurations:

1. **Debug (OpenOCD)** - Recommended
   - Flashes firmware using cppdbg debugger
   - Alternative to Cortex-Debug
   - Stops at entry point

3. **Attach (OpenOCD)**
   - Attach to already-running firmware
   - Useful when firmware is already flashed

#### Start Debugging:
1. Set breakpoints in your code
2. Press `F5` or select a debug configuration
3. Use standard debug controls (Step Over, Step Into, Continue, etc.)

## Project Structure

```
StepperDEV/
├── .vscode/              # VS Code configuration
│   ├── c_cpp_properties.json
│   ├── launch.json       # Debug configurations
│   ├── settings.json
│   └── tasks.json        # Build and flash tasks
├── cmake/
│   ├── gcc-arm-none-eabi.cmake  # Toolchain file
│   └── stm32cubemx/
│       └── CMakeLists.txt
├── Core/
│   ├── Inc/              # Header files
│   │   ├── main.h
│   │   ├── tmc5240.h
│   │   └── ...
│   └── Src/              # Source files
│       ├── main.c
│       ├── tmc5240.c
│       └── ...
├── Drivers/              # STM32 HAL drivers
│   ├── CMSIS/
│   └── STM32L4xx_HAL_Driver/
├── build/                # Build output (ignored by git)
├── CMakeLists.txt        # Main CMake configuration
├── CMakePresets.json     # CMake presets
└── STM32L476XX_FLASH.ld  # Linker script
```

## Adding New Source Files

When you add new `.c` files to `Core/Src/`:

1. Open `cmake/stm32cubemx/CMakeLists.txt`
2. Add your file to the `MX_Application_Src` list:
   ```cmake
   set(MX_Application_Src
       ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/main.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/your_new_file.c
       # ... other files
   )
   ```
3. Reconfigure CMake (VS Code does this automatically)

Header files in `Core/Inc/` are automatically included.

## Troubleshooting

### Windows: "arm-none-eabi-gcc not found"
Ensure the ARM GCC toolchain `bin` directory is in your system PATH. Restart VS Code/terminal after modifying PATH.

### Linux: Permission Denied for ST-Link
Add udev rules:
```bash
sudo tee /etc/udev/rules.d/99-stlink.rules > /dev/null <<'EOF'
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger
```

Unplug and reconnect your ST-Link.

### Build Errors: Undefined reference to `_estack`
The linker script is not being used. This should be fixed automatically by the toolchain file. If issues persist, clean and rebuild:
```bash
rm -rf build/
cmake --preset Debug
cmake --build build/Debug
```

### OpenOCD: "Error: libusb_open() failed with LIBUSB_ERROR_NOT_SUPPORTED"
On Windows, install ST-Link drivers from: https://www.st.com/en/development-tools/stsw-link009.html

### CMake: "Bad CMake executable"
Ensure CMake is installed and accessible:
```bash
cmake --version
```

In VS Code, check `settings.json`:
```json
{
    "cmake.cmakePath": "cmake"
}
```

# TMC5240 – Bare Minimum Configuration for Moving a Fixed Number of Steps

This document describes the **minimum required register configuration** for the **TMC5240** stepper motor driver to rotate a motor by a specific number of steps using the **internal motion controller** (position mode).

The goal is:
- ✔ Move the motor to a target position
- ✔ Set sane motor current limits
- ✔ Avoid unnecessary features (StealthChop2, tuning, power optimizations)
- ✔ Keep bring-up simple and reliable

---

## Design Philosophy

- **No StealthChop2**  
  Use default **SpreadCycle** for robustness.
- **No fine chopper tuning**  
  Only a valid configuration is required.
- **No power optimizations**  
  These can be added later.
- **Position-based move**  
  Command motion using `XTARGET`.

---

## Minimum Required Registers

Only the following registers are required to move the motor:

```

GCONF
DRV_CONF
IHOLD_IRUN
CHOPCONF
RAMPMODE
XACTUAL
AMAX
DMAX
VMAX
XTARGET

````

Everything else is optional.

---

## 1. Core Driver Configuration

### Enable Driver and Motion Controller

```c
writeRegister(IC, TMC5240_GCONF, 0x00000008);
````

* Enables the internal motion controller
* Leaves all optional features disabled

---

### Current Range Selection (Hardware Scaling)

```c
writeRegister(IC, TMC5240_DRV_CONF, 0x00000020);
```

* Selects the current range based on your **RREF resistor**
* This sets the **absolute maximum motor current**
* All run/hold currents are scaled from this

---

## 2. Motor Current Configuration

```c
// IHOLD = 3, IRUN = 10, IHOLDDELAY = 7
writeRegister(IC, TMC5240_IHOLD_IRUN, 0x00070A03);
```

* **IRUN**: current while the motor is moving
* **IHOLD**: current when the motor is stopped
* **IHOLDDELAY**: delay before switching to hold current

These values are expressed as a **percentage of full-scale current**.

---

## 3. Minimal Chopper Configuration (SpreadCycle)

```c
writeRegister(IC, TMC5240_CHOPCONF, 0x00010053);
```

Why this value:

* Valid **SpreadCycle** configuration
* No StealthChop2 dependency
* Works reliably for bring-up
* No tuning required

---

## 4. Motion Mode and Kinematics

### Select Positioning Mode

```c
writeRegister(IC, TMC5240_RAMPMODE, 0x00000000);
```

* Enables **position mode**
* Motor moves to `XTARGET`

---

### Reset Logical Position

```c
writeRegister(IC, TMC5240_XACTUAL, 0x00000000);
```

* Defines the current position as zero
* Recommended at startup

---

### Acceleration and Speed Limits

```c
writeRegister(IC, TMC5240_AMAX, 0x00000F8D);
writeRegister(IC, TMC5240_DMAX, 0x00000F8D);
writeRegister(IC, TMC5240_VMAX, 0x00002710);
```

* `AMAX` – acceleration
* `DMAX` – deceleration
* `VMAX` – maximum velocity

Exact values are application-dependent but these are safe defaults.

---

## 5. Commanding a Move

To rotate the motor by a fixed number of microsteps:

```c
writeRegister(IC, TMC5240_XTARGET, 2000);
```

The motor will:

1. Accelerate
2. Reach cruise velocity
3. Decelerate
4. Stop exactly at the target position

Negative values move in the opposite direction.

---

## What Is Intentionally Omitted

The following registers are **not required** for basic motion:

| Register               | Reason                             |
| ---------------------- | ---------------------------------- |
| `GLOBAL_SCALER`        | Defaults are sufficient            |
| `TPOWERDOWN`           | Only affects idle power saving     |
| `TVMAX`                | Not used in basic position mode    |
| Tuned `CHOPCONF`       | Only needed for noise optimization |
| StealthChop2 registers | Optional, not required             |

---

## Recommended Bring-Up Strategy

1. ✅ Use this minimal configuration
2. ✅ Verify direction, speed, and positioning
3. ✅ Validate motor current and temperature
4. ⏭ Add StealthChop2 **only if noise matters**
5. ⏭ Add power optimizations later

---

## Summary

This configuration is the **smallest reliable setup** that:

* Moves a motor by a known number of steps
* Uses safe current control
* Avoids unnecessary complexity

Perfect for:

* Initial firmware bring-up
* Motion verification
* Deterministic positioning applications

---

```
