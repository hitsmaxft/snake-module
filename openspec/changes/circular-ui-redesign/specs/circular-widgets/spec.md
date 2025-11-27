# Circular Widget System Specification

## ADDED Requirements

### Requirement: Circular Display System
The system SHALL implement a circular UI layout for the 240×240px ST7789V display with precise coordinate-based positioning of all UI elements.

#### Scenario:
When the system initializes, it SHALL create a circular coordinate system with center at (120, 120) and render all widgets using polar-to-cartesian coordinate conversion for exact positioning.

### Requirement: Central Speed Display
The system SHALL display current WPM/PWM speed as the central focal point using 48px font with real-time updates and animated gauge visualization.

#### Scenario:
When user typing speed changes, the central display SHALL update within 100ms showing the new WPM value with smooth animated transitions and visual gauge indicating relative speed levels.

### Requirement: Circular Battery Indicators
The system SHALL display battery levels for keyboard halves in circular quadrant segments around the display perimeter with gradient fills and percentage indicators.

#### Scenario:
When battery level changes, the corresponding circular segment SHALL update its fill percentage and color intensity to reflect the new battery state with smooth color transitions.

### Requirement: BLE Connection Status
The system SHALL display Bluetooth connectivity states using colored arc segments in the outer ring with dynamic colors indicating connection status, pairing mode, and signal strength.

#### Scenario:
When BLE connection state changes, the corresponding arc segment SHALL animate to the new color and display appropriate icons or patterns indicating the current connection status.

### Requirement: Performance Optimization
The system SHALL maintain efficient rendering performance suitable for nRF52 embedded constraints with <50ms refresh cycle and <80% CPU utilization during normal operation.

#### Scenario:
During continuous typing with rapid speed changes, the system SHALL maintain smooth animations without frame drops or input latency exceeding 10ms.

## MODIFIED Requirements

### Requirement: Widget Architecture
The existing modular widget system SHALL be adapted to support circular layout with new circular widget base class and coordinate transformation utilities.

#### Scenario:
When initializing widgets, the system SHALL load circular widget implementations instead of rectangular grid-based widgets while maintaining the same event system integration.

### Requirement: Theme System
The existing theme system SHALL be extended to support circular color schemes with radial gradients and arc-based color definitions.

#### Scenario:
When switching themes, circular elements SHALL apply new color schemes including gradient fills for arcs and radial color transitions for circular elements.

### Requirement: Event System Integration
The existing ZMK event system SHALL maintain compatibility with the new circular UI without changes to keyboard behavior or performance characteristics.

#### Scenario:
When ZMK events occur (key presses, layer changes, etc.), they SHALL be processed by the circular widget system with appropriate visual feedback in the circular layout.

## REMOVED Requirements

### Requirement: Grid-based Layout System
The existing rectangular grid positioning system SHALL be removed in favor of circular coordinate-based positioning for all UI elements.

#### Scenario:
When positioning widgets, the system SHALL use polar coordinates and circular positioning calculations instead of grid-based X,Y positioning.

### Requirement: Legacy Widget Implementations
Existing rectangular widget implementations SHALL be backed up and replaced with circular equivalents while maintaining functional parity.

#### Scenario:
When displaying battery status, the new system SHALL use circular arc indicators instead of rectangular progress bars with equivalent information content.

## Implementation Constraints

### Memory Management
- Maximum graphics buffer usage: 32KB for double buffering
- Maximum per-widget memory allocation: 4KB
- Circular calculations must use fixed-point arithmetic where possible

### Performance Requirements
- Maximum render cycle time: 50ms for full refresh
- Maximum partial update time: 10ms for individual widget updates
- Memory allocation must be static at compile time

### Hardware Constraints
- Display resolution: Fixed 240×240px circular constraint
- Color format: 16-bit RGB565 with optimized circular rendering
- Processing optimization for ARM Cortex-M4 architecture

## Validation Criteria

### Functional Testing
- All circular widgets render correctly with proper positioning
- Speed display updates within 100ms of WPM changes
- Battery indicators reflect accurate charge levels
- BLE status arcs show correct connection states
- Theme switching applies properly to circular elements

### Performance Testing
- System maintains <50ms refresh cycles under normal load
- CPU utilization remains <80% during peak activity
- Memory usage stays within allocated constraints
- No frame drops during rapid animation sequences

### Integration Testing
- ZMK event system functions correctly with circular UI
- Keyboard behavior remains unchanged
- Build system integration works properly
- Hardware compatibility maintained with existing configurations