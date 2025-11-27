# Current Widget System Analysis

## Widget System Architecture

### Main Controller
**File**: `boards/shields/snake_adapter/custom_status_screen.c`
- **Purpose**: Central widget initialization and coordination
- **Key Functions**:
  - `zmk_display_status_screen()` - Main entry point, initializes all widgets
  - `timer_splash()` - Manages splash screen transition to main UI
  - Widget lifecycle management (init → start → stop)

### Widget Initialization Pattern
All widgets follow consistent initialization pattern:
```c
// During system startup
zmk_widget_[name]_init();           // Initialize widget structures
// Later in timer_splash()
initialize_[widget_name]();         // Set up widget state
start_[widget_name]();              // Begin widget operation
```

## Widget Inventory and Functionality

### 1. WPM (Words Per Minute) Widget
**Files**: `widgets/wpm.h/.c`
- **Primary Function**: Real-time typing speed tracking and display
- **Key Data**: `struct wpm_speed_state { uint8_t wpm; }`
- **Display**: 3x5 font, positioned at (14, 127), scale factor 4
- **Events**: Responds to `wpm_state_changed` ZMK events
- **Integration**: ZMK WPM calculation system

### 2. Battery Status Widget
**Files**: `widgets/battery_status.h/.c`
- **Primary Function**: Battery level monitoring for peripheral devices
- **Key Data**: `struct peripheral_battery_state` for multiple devices
- **Display**: Custom battery symbols, configurable font scaling
- **Features**:
  - Dual battery support (device 0 and 1)
  - Battery state tracking with previous values
  - USB/Bluetooth power source indication
- **Events**: `battery_state_changed` ZMK events

### 3. Output Status Widget
**Files**: `widgets/output_status.h/.c`
- **Primary Function**: Connection status display (USB/Bluetooth)
- **Key Data**: Connection state tracking
- **Display**: Status symbols indicating current output mode
- **Events**: `usb_conn_state_changed` and BLE events

### 4. Layer Status Widget
**Files**: `widgets/layer_status.h/.c`
- **Primary Function**: Current keyboard layer visualization
- **Display**: Layer number or name
- **Events**: ZMK layer change events

### 5. Action Button Widget
**Files**: `widgets/action_button.h/.c`
- **Primary Function**: Interactive UI button for navigation
- **Features**: Menu navigation, game control
- **Input**: Touch/button input handling

### 6. Snake Game Widget
**Files**: `widgets/snake.h/.c`, `widgets/snake_image.h`
- **Primary Function**: Fully functional Snake game implementation
- **Features**: Score tracking, game controls, high score persistence
- **Assets**: Large bitmap data (75KB) for game graphics
- **Input**: Keyboard matrix input processing

### 7. Theme System
**Files**: `widgets/theme.h/.c`
- **Primary Function**: Runtime color scheme management
- **Key Data**: `themes_colors[][6]` array for color definitions
- **Features**: Theme switching, color configuration
- **Integration**: Affects all widget colors

### 8. Splash Screen Widget
**Files**: `widgets/splash.h/.c`
- **Primary Function**: Startup screen animation
- **Timing**: 50-frame animation with configurable duration
- **Transition**: Smooth transition to main UI

### 9. Logo Widget
**Files**: `widgets/logo.h/.c`
- **Primary Function**: Logo/graphic display
- **Features**: Animation support

### 10. Modifier Widget
**Files**: `widgets/modifier.h/.c`
- **Primary Function**: Modifier key status display
- **Features**: Visual feedback for active modifiers

### 11. Configuration Widget
**Files**: `widgets/configuration.h/.c`
- **Primary Function**: Settings management interface
- **Features**: Configuration navigation and modification

### 12. Peripheral Status Widget
**Files**: `widgets/peripheral_status.h/.c`
- **Primary Function**: BLE peripheral device management
- **Features**: Connection tracking, device status

## Helper System (`widgets/helpers/`)

### Display Management (`display.h/.c`)
- **Size**: 60KB (largest component)
- **Purpose**: Core display rendering and LVGL integration
- **Features**:
  - Display buffer management
  - Font rendering utilities
  - Graphics primitive operations
  - Screen coordinate systems

### PWM Control (`pwm.h/.c`)
- **Purpose**: Pulse Width Modulation for buzzer and backlight
- **Features**: Hardware PWM abstraction

### Buzzer System (`buzzer.h/.c`)
- **Purpose**: Audio feedback system
- **Features**: Sound effects, tone generation

### Settings (`settings.h/.c`)
- **Purpose**: Persistent data storage
- **Features**: ZMK settings integration, data persistence

### List (`list.h/.c`)
- **Purpose**: Data structure utilities
- **Features**: Linked list operations for widget management

## Key Technical Patterns

### Event System Integration
All widgets use ZMK event system:
```c
#include <zmk/event_manager.h>
#include <zmk/events/[event_type].h>
```

### Display Integration
Consistent display management:
```c
#include "helpers/display.h"
// Use common display functions
```

### State Management
Widget lifecycle pattern:
- `_initialized` flag
- `_running` flag
- State structures for data
- Previous value tracking for updates

### Memory Management
- Static allocation for embedded constraints
- Scaled bitmaps for font rendering
- Efficient buffer usage

## Performance Characteristics

### Memory Usage
- **Total System**: ~360KB widget code
- **Graphics**: Snake game (75KB) + Display management (60KB)
- **Per Widget**: 1-10KB typical size

### Performance Optimizations
- Custom font scaling
- Efficient redraw cycles
- Minimal memory allocations
- Hardware acceleration usage

## Integration Points for Circular UI Migration

### ZMK Compatibility
- **Event System**: Maintain all existing ZMK event subscriptions
- **State Data**: Preserve data structures and state tracking
- **Performance**: Maintain or improve current performance metrics

### Display Pipeline
- **LVGL Integration**: Continue using LVGL for rendering
- **Font System**: Adapt existing font scaling for circular context
- **Color Management**: Extend theme system for circular elements

### Widget Migration Strategy
1. **WPM Widget** → Central speed display with gauge
2. **Battery Widget** → Circular battery indicators
3. **Output Status** → BLE connection status arcs
4. **Theme System** → Extended for circular gradients
5. **Layer Status** → Circular indicator integration

### Hardware Integration
- **Display**: Maintain ST7789V SPI interface
- **Input**: Preserve existing input handling
- **Power**: Maintain power efficiency optimizations