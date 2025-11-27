# Project Context

## Purpose
ZMK Snake Module is a custom ZMK firmware module that provides an advanced status screen display for split mechanical keyboards with ST7789V TFT displays. The module implements a comprehensive UI system including widgets, theming, game functionality (Snake), and peripheral management for enhanced keyboard user experience.

## Tech Stack
- **Core Framework**: ZMK Firmware (Zephyr RTOS-based)
- **Programming Language**: C (embedded systems)
- **Build System**: CMake with Zephyr build system
- **Display**: ST7789V TFT LCD driver (240x240px)
- **Graphics**: LVGL (Light and Versatile Graphics Library)
- **Hardware Platform**: Nordic nRF52 microcontrollers
- **Configuration**: Devicetree overlays (.overlay files)
- **PWM Control**: Buzzer and backlight control via PWM
- **Communication**: SPI for display, GPIO for controls

## Project Conventions

### Code Style
- **Naming Convention**: snake_case for functions and variables, PascalCase for types
- **File Organization**: Separate .h headers for each .c implementation file
- **Module Structure**:
  - `src/` - Core behaviors and events
  - `drivers/display/` - Hardware display drivers
  - `boards/shields/snake_adapter/` - Shield-specific implementations
  - `boards/shields/snake_adapter/widgets/` - UI components
  - `include/` - Public headers
- **Function Prefixing**: Widget-specific prefixes (e.g., `battery_status_`, `snake_`, `theme_`)
- **Enum Naming**: ALL_CAPS with descriptive names (e.g., `SLOT_SIDE_LEFT`, `FONT_SIZE_3x5`)

### Architecture Patterns
- **Widget System**: Modular widget architecture with individual .c/.h files
- **Event-Driven**: ZMK event system for keyboard actions and state changes
- **Display Management**: Centralized display controller with widget rendering
- **Theme System**: Configurable color schemes with runtime switching
- **Hardware Abstraction**: Devicetree-based hardware configuration
- **Layer Architecture**:
  - Hardware layer (drivers, GPIO, SPI)
  - Widget layer (UI components)
  - Application layer (main screen management)
  - Configuration layer (theming, settings)

### Testing Strategy
- **Manual Testing**: Hardware-in-the-loop testing on actual keyboard hardware
- **Build Verification**: CMake configuration validation and compilation testing
- **Devicetree Validation**: Hardware configuration validation via Zephyr tools
- **Display Testing**: Visual verification of UI components and rendering
- **Integration Testing**: Full system testing with ZMK firmware integration
- **No Automated Tests**: Embedded system relies on manual hardware validation

### Git Workflow
- **Branching Strategy**: Feature branches for development, `main` for stable releases
- **Commit Convention**: Conventional commits with clear descriptions
- **PR Workflow**: Pull requests for code review and integration
- **Module Structure**: Independent module that can be merged into main ZMK config

## Domain Context

### ZMK Firmware Integration
- **Module System**: Custom ZMK module integrated via CMake and Zephyr module system
- **Behavior System**: Custom ZMK behaviors for keyboard functionality
- **Event System**: Custom events for inter-component communication
- **Shield Support**: Designed for split keyboard shields with displays
- **Power Management**: Deep sleep support for battery-powered devices

### Display and Graphics
- **ST7789V Driver**: Custom SPI-based display driver with optimized rendering
- **Font System**: Multiple font sizes (3x5, 5x7, 5x8, 3x6) for different UI elements
- **Color Management**: RGB565 color format with theme support
- **Rendering Pipeline**: Double buffering and optimized drawing operations
- **UI Layout**: Grid-based layout system for widget positioning

### Keyboard-Specific Features
- **Split Keyboard Support**: Left/right hand coordination and status display
- **Battery Monitoring**: Real-time battery percentage and status for both halves
- **Layer Display**: Current keyboard layer visualization
- **WPM Tracking**: Words-per-minute calculation and display
- **Connectivity Status**: USB/Bluetooth connection status indicators
- **Peripheral Management**: BLE device pairing and connection management

### Game System
- **Snake Game**: Fully functional Snake game implementation
- **Input Handling**: Keyboard matrix input processing
- **Score Tracking**: High score persistence using ZMK settings
- **Game Controls**: Custom behavior mappings for game controls

## Important Constraints

### Hardware Constraints
- **Memory Limited**: Embedded system with limited RAM (typical nRF52: 64KB-256KB)
- **Flash Storage**: Limited flash space for code and assets (typical: 512KB-1MB)
- **Power Constraints**: Battery-powered operation requiring efficient power management
- **Display Limitations**: 240x240px resolution, 16-bit color depth
- **CPU Performance**: ARM Cortex-M4 with limited processing power

### Real-Time Constraints
- **Keyboard Responsiveness**: Must maintain <1ms key scan latency
- **Display Refresh**: Limited refresh rate to avoid power drain
- **Event Processing**: Real-time event handling for keyboard input
- **Timing Critical**: SPI communication and PWM timing requirements

### Integration Constraints
- **ZMK Compatibility**: Must maintain compatibility with ZMK firmware updates
- **Devicetree Compliance**: Must follow Zephyr devicetree specifications
- **Build System**: Must integrate with Zephyr/CMake build system
- **API Stability**: ZMK APIs may change between versions

## External Dependencies

### Core Dependencies
- **ZMK Firmware**: Main keyboard firmware framework
- **Zephyr RTOS**: Real-time operating system and HAL
- **LVGL**: Graphics library for UI rendering (via ZMK)
- **Nordic SDK**: nRF52 series support libraries

### Hardware Dependencies
- **ST7789V Display**: Specific TFT LCD controller chip
- **nRF52 SoC**: Nordic microcontroller family
- **SPI Interface**: Display communication protocol
- **PWM Controller**: Buzzer and backlight control

### Build Dependencies
- **Zephyr Tools**: West build system and devicetree tools
- **ARM GCC Toolchain**: Cross-compilation toolchain
- **CMake**: Build system configuration
- **Python**: Various build and utility scripts

### Development Dependencies
- **OpenSpec**: Specification management system
- **Git Hooks**: Pre-commit validation and formatting
- **Documentation**: Markdown-based documentation system
