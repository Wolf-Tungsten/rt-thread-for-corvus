#ifndef __DRV_SATELLITESTATION_H__
#define __DRV_SATELLITESTATION_H__

#include <stdint.h>
#include "interrupt.h"

/* Begin Configs */
#define SAT_BASE        0x30000000UL
#define simCoreDBusDataWidth 64
#define satelliteIRQNum 5
#define syncTreeflagWidth 2
/* End Configs */
#define sat_BusCount (sat_MBusCount + sat_SBusCount)

#if simCoreDBusDataWidth!=64
#error Only support DBusDataWidth == 64
#endif

static inline unsigned int sat_pow2ceil(unsigned int v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

#define SAT_N_RS (sat_pow2ceil(1U + 2U + 2U * sat_BusCount))
#define SAT_N_WS 2
#define SAT_N_RQ (sat_BusCount)
#define SAT_N_WQ (sat_BusCount)

#define SAT_RS_BASE (SAT_BASE)
#define SAT_WS_BASE (SAT_RS_BASE + SAT_N_RS * 8UL)
#define SAT_RQ_BASE (SAT_WS_BASE + SAT_N_WS * 8UL)
#define SAT_WQ_BASE (SAT_RQ_BASE + SAT_N_RQ * 8UL)

/// Read only control registers.
/// 0: inSyncFlag
/// 1: nMbus
/// 2: nSbus
/// 3             ~ 3 + nStateBus - 1: toCore receive queue count
/// 3 + nStateBus ~ 3 + 2 * nStateBus - 1: fromCore send queue count
/// Remaining entries up to SAT_N_RS are 0.
#define SAT_RS(offset) (SAT_RS_BASE + (offset) * 8)
/// Writable Control Registers, 0 to 2
/// 0: outSyncFlag
/// 1: nodeId
#define SAT_WS(offset) (SAT_WS_BASE + (offset) * 8)
/// Receive buffer, pop when read, 0 to SAT_N_RQ - 1
/// Each for a StateBus
#define SAT_RQ(offset) (SAT_RQ_BASE + (offset) * 8)
/// Send buffer, push when written, 0 to SAT_N_WQ - 1
/// Each for a StateBus
#define SAT_WQ(offset) (SAT_WQ_BASE + (offset) * 8)

#define SAT_TOCORE_COUNT(offset)   SAT_RS(3 + (offset))
#define SAT_FROMCORE_COUNT(offset) SAT_RS(3 + sat_BusCount + (offset))

#define SAT_inSyncFlag()  SAT_RS(0)
#define SAT_outSyncFlag() SAT_WS(0)
#define SAT_nodeId()      SAT_WS(1)

/// Must init before any usage, doesn't enable interrupt.
void sat_init(void);
/// Send to `target` with `payload[47:0]` though StateBus `n`. Block when full
void sat_send(uint64_t ch, uint16_t target, uint64_t payload);
/// Get a payload from StateBus `n`. Block when empty
uint64_t sat_recv(uint64_t ch);
/// Get queued receive packet count for StateBus `n`.
uint64_t sat_receiveBufferCnt(uint64_t ch);
/// Get queued send packet count for StateBus `n`.
uint64_t sat_sendBufferCnt(uint64_t ch);
void sat_clearBuffer(uint64_t ch);
void sat_dump_regs(void);
void sat_set_outSyncFlag(uint64_t flag);
uint64_t sat_get_inSyncFlag();
uint64_t SAT_MBUS_CH(uint64_t ch);
uint64_t SAT_SBUS_CH(uint64_t ch);
/// Install interrupt server for recvFull, return old server. Also enable interrupt.
rt_isr_handler_t sat_interrupt_install(rt_isr_handler_t handler, void* param);
/// MBus for this core, available after `sat_init()`
extern uint64_t sat_MBusCount;
/// SBus for this core, available after `sat_init()`. For core0 this is 0.
extern uint64_t sat_SBusCount;
#endif // __DRV_SATELLITESTATION_H__
