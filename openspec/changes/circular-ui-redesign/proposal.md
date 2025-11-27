# Circular UI Redesign Proposal

## Why
The current ZMK 7789 widget system uses a traditional grid-based layout that doesn't fully leverage the circular display capabilities of the ST7789V screen. This redesign creates an innovative circular interface that prioritizes speed and performance metrics while exploring new interaction patterns for embedded keyboard displays.

## Overview
Experimental redesign of the ZMK 7789 widget system to implement a completely new circular UI focusing on speed and performance metrics.

## Change Details
- **Scope**: Complete widget system replacement with circular design
- **Target**: ST7789V 240×240px display with circular UI specification
- **Approach**: Backup current widgets and implement new circular architecture
- **Focus**: Speed & performance with PWM/WPM as primary display elements

## Implementation Strategy
1. **Migration**: Backup existing widgets to `widgets_backup/`
2. **Architecture**: New circular widget system based on `docs/ui_design.md`
3. **Performance**: Optimized for embedded nRF52 system constraints
4. **Integration**: Maintain ZMK event system and theme compatibility

## Expected Outcomes
- Innovative circular UI with experimental design elements
- Speed-focused display with animated gauges and performance metrics
- Maintained ZMK compatibility and system performance
- Modular circular component system for future extensions

## Files Modified
- `boards/shields/snake_adapter/widgets/` - Complete widget system replacement
- `src/custom_status_screen.c` - Main controller integration
- `include/` - New circular widget headers
- Build system integration for new components

## Testing Strategy
- Hardware-in-the-loop testing on actual keyboard
- Performance validation on embedded system
- Visual verification of circular UI elements
- Integration testing with ZMK event system