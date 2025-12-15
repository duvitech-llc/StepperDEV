set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Some default GCC settings
set(TOOLCHAIN_PREFIX arm-none-eabi-)

# Try to find the toolchain in common installation paths
if(WIN32)
    # Common Windows installation paths
    set(TOOLCHAIN_PATHS
        "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin"
        "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/13.2 Rel1/bin"
        "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/12.2 Rel1/bin"
        "C:/Program Files/GNU Arm Embedded Toolchain/10 2021.10/bin"
        "C:/Program Files/Arm GNU Toolchain arm-none-eabi/13.2 Rel1/bin"
        "C:/Program Files/Arm GNU Toolchain arm-none-eabi/12.2 Rel1/bin"
        "$ENV{ProgramFiles\(x86\)}/GNU Arm Embedded Toolchain/10 2021.10/bin"
        "$ENV{ProgramFiles}/GNU Arm Embedded Toolchain/10 2021.10/bin"
    )
    set(TOOLCHAIN_EXT ".exe")
else()
    # Unix-like systems
    set(TOOLCHAIN_PATHS
        "/usr/bin"
        "/usr/local/bin"
    )
    set(TOOLCHAIN_EXT "")
endif()

# Search for the compiler
find_program(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc${TOOLCHAIN_EXT}
    PATHS ${TOOLCHAIN_PATHS}
    NO_DEFAULT_PATH
)

find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++${TOOLCHAIN_EXT}
    PATHS ${TOOLCHAIN_PATHS}
    NO_DEFAULT_PATH
)

find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc${TOOLCHAIN_EXT}
    PATHS ${TOOLCHAIN_PATHS}
    NO_DEFAULT_PATH
)

find_program(CMAKE_AR ${TOOLCHAIN_PREFIX}ar${TOOLCHAIN_EXT}
    PATHS ${TOOLCHAIN_PATHS}
    NO_DEFAULT_PATH
)

find_program(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy${TOOLCHAIN_EXT}
    PATHS ${TOOLCHAIN_PATHS}
    NO_DEFAULT_PATH
)

find_program(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump${TOOLCHAIN_EXT}
    PATHS ${TOOLCHAIN_PATHS}
    NO_DEFAULT_PATH
)

find_program(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size${TOOLCHAIN_EXT}
    PATHS ${TOOLCHAIN_PATHS}
    NO_DEFAULT_PATH
)

find_program(CMAKE_DEBUGGER ${TOOLCHAIN_PREFIX}gdb${TOOLCHAIN_EXT}
    PATHS ${TOOLCHAIN_PATHS}
    NO_DEFAULT_PATH
)

# Check if compiler was found
if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "arm-none-eabi-gcc not found. Please install ARM GCC toolchain and ensure it's in your PATH or update TOOLCHAIN_PATHS in cmake/gcc-arm-none-eabi.cmake")
endif()

# Disable compiler checks
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

# Configure CMake to not search for programs in the target system root
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Set compiler flags for STM32
set(COMMON_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fdata-sections -ffunction-sections")

set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${COMMON_FLAGS}")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections -specs=nano.specs -specs=nosys.specs")
