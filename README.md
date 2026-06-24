# Traffic Lab v2

**Real-time embedded traffic control system** - STM32 FreeRTOS state machine with SUMO simulation bridge.

## Core Features

- **≤5ms jitter** state machine (FreeRTOS + SystemView)
- **Minimal TLV protocol** with CRC16 validation
- **Linux diagnostic tool** (`tlv_dump` with frame statistics)
- **Adaptive algorithm** vs fixed-time throughput comparison

## Architecture

```
STM32    FreeRTOS                         Python
UartRx -> TrafficController -> UartTx   <->   sumo_bridge.py <-> SUMO
         (State Machine)                    (TraCI + TLV)
```

## TLV Protocol (2 Messages)

**Frame:** `| SOF 0xAA55 | LEN | TYPE | PAYLOAD | CRC16 |`

- **`0x01 LaneCounts`** (SUMO->STM32): `[ts_sec, n, s, e, w]`
- **`0x02 LightState`** (STM32->SUMO): `[current_state]`

## State Machine

```
NS_GREEN (8-25s) -> NS_YELLOW (3s) -> ALL_RED (1s) ->
EW_GREEN (8-25s) -> EW_YELLOW (3s) -> ALL_RED (1s)
```

**Adaptive rule:** Extend green if opposite queue > current + 3 for 2+ cycles

## Quick Demo

```bash
# 1. Run bridge to SUMO
python tools/python_bridge/main.py

# 2. Monitor TLV traffic
cat /dev/ttyUSB0 | ./linux/tlv_dump

# 3. View: Real-time control + throughput improvement
```

## Validation

- **SystemView traces** proving ≤5ms timing jitter
- **Throughput comparison** showing adaptive vs fixed-time performance
- **Frame statistics** via `tlv_dump` diagnostic tool

## Requirements

- SUMO
- Python 3.8+
- STM32CubeIDE
- STM32F446RE Nucleo
