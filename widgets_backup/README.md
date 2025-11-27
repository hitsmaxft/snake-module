# Widget System Backup

## Backup Information
- **Backup Date**: 2025-11-28
- **Original Location**: `boards/shields/snake_adapter/widgets/`
- **Purpose**: Preserve existing widget functionality during circular UI redesign
- **Total Size**: 360KB (verified integrity)

## Widget Inventory

### Main Widgets
- `action_button.c/.h` - Interactive button widget (5.8KB)
- `battery_status.c/.h` - Battery level display widget (7.8KB)
- `configuration.c/.h` - Configuration management widget (5.0KB)
- `layer_status.c/.h` - Current keyboard layer display (4.7KB)
- `logo.c/.h` - Logo/graphic display widget (8.9KB)
- `modifier.c/.h` - Modifier key status widget (6.1KB)
- `output_status.c/.h` - Output connection status (11KB)
- `peripheral_status.c/.h` - Peripheral device management (1.4KB)
- `snake.c/.h` - Snake game implementation (21KB)
- `snake_image.h` - Snake game graphics data (75KB)
- `splash.c/.h` - Startup splash screen (11KB)
- `theme.c/.h` - Theme management system (3.7KB)
- `wpm.c/.h` - Words-per-minute tracking (3.4KB)

### Helper System (`helpers/`)
- `buzzer.c/.h` - PWM buzzer control (18KB)
- `display.c/.h` - Core display management (60KB)
- `list.c/.h` - List data structures (1.7KB)
- `pwm.c/.h` - PWM management utilities (8.9KB)
- `settings.c/.h` - Settings persistence (2.2KB)

## Key Functionality Preserved

### Performance Features
- Real-time WPM tracking and display
- Battery monitoring for both keyboard halves
- Peripheral device connection management
- Performance metrics visualization

### Interactive Elements
- Snake game with score tracking
- Action buttons for UI navigation
- Configuration management interface
- Theme switching functionality

### System Integration
- ZMK event system integration
- Display rendering pipeline
- Theme system with runtime switching
- Settings persistence using ZMK storage

## Restoration Notes
- All functionality preserved exactly as in original implementation
- Can be restored by copying back to original location
- Integration with existing build system maintained
- Compatible with current ZMK version

## Migration Reference
This backup serves as functional reference for implementing circular equivalents:
- Battery status → Circular battery indicators
- WPM display → Central speed gauge
- Peripheral status → Circular peripheral management
- Theme system → Extended circular theming
- Event integration → Maintain ZMK compatibility