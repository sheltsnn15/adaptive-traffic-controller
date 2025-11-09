## Overview

Hard real-time traffic controller that processes vehicle data from SUMO simulations and controls traffic lights with deterministic timing (< 5ms jitter).

## Project Structure

```
traffic-light-sim/
├── stm32/          # FreeRTOS application (STM32F4)
├── sim/            # SUMO configurations (X & Y junctions)
├── tools/python_bridge/  # Python MQTT/TraCI bridge
├── proto/          # TLV protocol library
├── linux/          # Linux tools and utilities
└── docs/           # Documentation and protocols
```

## Key Features

- **Real-time Control**: 100ms control period with ≤5ms jitter
- **Adaptive Algorithm**: Extends green time based on traffic load
- **TLV Protocol**: Compact binary protocol for sensor data
- **SUMO Integration**: Real traffic simulation with Python bridge
- **FreeRTOS**: Deterministic task scheduling on STM32

## Quick Start

1. **Run SUMO Simulation**:

   ```bash
   sumo-gui -c sim/x_junction/x_junction.sumocfg
   ```

2. **Start Python Bridge**:

   ```bash
   cd tools/python_bridge
   python main.py
   ```

3. **STM32 Controller**: Build and flash the STM32 project using STM32CubeIDE

## Protocol

Uses TLV (Type-Length-Value) frames:

- `0x01`: Lane counts (vehicle data)
- `0x03`: Heartbeat (system status)

## Requirements

- SUMO (Simulation of Urban MObility)
- Python 3.8+
- STM32CubeIDE
- STM32F446RE Nucleo board
