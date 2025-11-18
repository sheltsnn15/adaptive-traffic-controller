<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [**Protocol Specification**](#protocol-specification)
  - [**1. Frame Format**](#1-frame-format)
  - [**2. Message Types**](#2-message-types)
  - [**3. Payload Structures**](#3-payload-structures)
    - [**LaneCounts** (TYPE=0x01)](#lanecounts-type0x01)
    - [**LightState** (TYPE=0x02)](#lightstate-type0x02)
    - [**Heartbeat** (TYPE=0x03)](#heartbeat-type0x03)
  - [**4. CRC Specification**](#4-crc-specification)
  - [**5. Frame Recovery**](#5-frame-recovery)
- [**State Definitions**](#state-definitions)
  - [**Traffic Light States:**](#traffic-light-states)
  - [**Decision Reasons:**](#decision-reasons)
- [**Complete Bidirectional Data Flow**](#complete-bidirectional-data-flow)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## **Protocol Specification**

### **1. Frame Format**

```
| SOF 2B=0xAA55 | LEN 2B | TYPE 1B | PAYLOAD (LEN-3) | CRC16 2B |
```

### **2. Message Types**

- `0x01` = LaneCounts (traffic data from SUMO → STM32)
- `0x02` = LightState (STM32 → SUMO feedback)
- `0x03` = Heartbeat (system health monitoring)

### **3. Payload Structures**

#### **LaneCounts** (TYPE=0x01)

```
uint32 ts_sec;          // UNIX seconds (little-endian)
uint8 junction_type;    // 0=X-junction, 1=Y-junction
uint8  n;               // North approach vehicle count
uint8  s;               // South approach vehicle count  
uint8  e;               // East approach vehicle count
uint8  w;               // West approach vehicle count
```

**Total**: 9 bytes

#### **LightState** (TYPE=0x02)

```
uint8 current_state;    // 0=NS_GREEN, 1=NS_YELLOW, 2=EW_GREEN, 3=EW_YELLOW, 4=ALL_RED
uint8 decision_reason;  // 0=fixed_time, 1=adaptive_extension, 2=emergency
uint16 phase_duration;  // Current phase duration in 100ms units (little-endian)
```

**Total**: 4 bytes

#### **Heartbeat** (TYPE=0x03)

```
uint32 uptime_ms; // System uptime in milliseconds (little-endian)
```

**Total**: 4 bytes

### **4. CRC Specification**

- **Algorithm**: CRC16-CCITT
- **Polynomial**: 0x1021
- **Initial Value**: 0xFFFF
- **Input Reflection**: No
- **Output Reflection**: No
- **Final XOR**: 0x0000

### **5. Frame Recovery**

- Invalid LEN or CRC → advance 1 byte and re-seek SOF
- Maximum frame length: 32 bytes

## **State Definitions**

### **Traffic Light States:**

```c
#define NS_GREEN     0  // North-South flow green
#define NS_YELLOW    1  // North-South flow yellow  
#define EW_GREEN     2  // East-West flow green
#define EW_YELLOW    3  // East-West flow yellow
#define ALL_RED      4  // All directions red
```

### **Decision Reasons:**

```c
#define FIXED_TIME          0  // Fixed timing schedule
#define ADAPTIVE_EXTENSION  1  // Extended due to vehicle detection
#define EMERGENCY           2  // Emergency vehicle or manual override
```

## **Complete Bidirectional Data Flow**

```
SUMO → Python Bridge → [LaneCounts:0x01] → STM32 → Traffic Logic
                                                      ↓
STM32 → UART Tx → [LightState:0x02] → Python Bridge → SUMO
```

**Frame Examples:**

- **LaneCounts**: `AA55 000C 01 [9-byte payload] [CRC16]`
- **LightState**: `AA55 0007 02 [4-byte payload] [CRC16]`  
- **Heartbeat**: `AA55 0007 03 [4-byte payload] [CRC16]`
