
## **Architecture**

**Strengths I see:**

- **Clean interfaces** (IStateMachine, ICommunicator) - great for testing
- **Proper abstraction layers** - hardware details hidden
- **RTOS integration** - TaskManager handles FreeRTOS specifics  
- **Protocol separation** - TLV parser is self-contained
- **Type safety** - strong enums and packed structs
- **Adaptive logic ready** - extension conditions built in

## **Key Design Questions to Think Through**

### **1. C++ vs C in Embedded Context**

- FreeRTOS is C-based - how will C++ tasks integrate?
- Memory allocation - are you using dynamic allocation or static?
- Exception safety - will you disable exceptions?

### **2. Real-time Performance**

- Virtual functions in IStateMachine - any performance concerns?
- TLV parser state machine - is it efficient enough for ISR context?
- Queue operations - blocking vs non-blocking strategies

### **3. Data Flow Between Components**

```
UartManager → TaskManager (queues) → TrafficStateMachine → TaskManager → UartManager
```

- How do these components discover each other?
- Who owns the queues? (static in TaskManager makes sense)

### **4. Error Handling Strategy**

- What happens when queues are full?
- How are parser errors communicated?
- Watchdog timeout recovery mechanism

## **Implementation Thinking Exercise**

**Let's think about the TaskManager implementation:**

If you were to implement `TaskManager::initialize()`, what would need to happen?

1. **Queue creation** - what sizes and types?
2. **Task creation** - what stack sizes and priorities?  
3. **Component wiring** - how do tasks get their dependencies?
4. **Startup sequence** - what order should tasks start?

**For the TrafficStateMachine:**

- How will you track the "2 consecutive cycles" for extension?
- What's the state transition logic from extended states?
- How does emergency mode interact with adaptive logic?

## **My Approach Questions**

**Would you prefer:**

- **Top-down**: Start with TaskManager and work downward
- **Bottom-up**: Start with traffic_types and work upward  
- **Vertical slices**: Get one complete flow working first
