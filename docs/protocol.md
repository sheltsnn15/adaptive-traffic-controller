# Traffic Lab Protocol Specification

## 1. Frame Format

```
| SOF 2B=0xAA55 | LEN 2B | TYPE 1B | PAYLOAD (LEN-3) | CRC16 2B |
```

## 2. Message Types

- `0x01` = LaneCounts (traffic data from SUMO)
- `0x03` = Heartbeat (system health monitoring)

## 3. Payload Structures

### LaneCounts (TYPE=0x01)

```
uint32 ts_sec;          // UNIX seconds (little-endian)
uint8 junction_type;    // 0=X-junction, 1=Y-junction
uint8  n;               // North approach vehicle count
uint8  s;               // South approach vehicle count  
uint8  e;               // East approach vehicle count
uint8  w;               // West approach vehicle count
```

**Total**: 9 bytes

### Junction Types

- `0x00` = X-junction (4 arms: n,s,e,w all active)
- `0x01` = Y-junction (3 arms: n,s,e active, w ignored)

### Heartbeat (TYPE=0x03)

```
uint32 uptime_ms; // System uptime in milliseconds (little-endian)
```

**Total**: 4 bytes

## 4. CRC Specification

- **Algorithm**: CRC16-CCITT
- **Polynomial**: 0x1021
- **Initial Value**: 0xFFFF
- **Input Reflection**: No
- **Output Reflection**: No
- **Final XOR**: 0x0000
- **Coverage**: TYPE + PAYLOAD bytes

## 5. Frame Recovery

- Invalid LEN or CRC → advance 1 byte and re-seek SOF
- Maximum frame length: 32 bytes
