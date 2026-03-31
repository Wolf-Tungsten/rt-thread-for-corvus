#ifndef __DRV_SATELLITESTATION_H__
#define __DRV_SATELLITESTATION_H__

#include <stdint.h>
#include "interrupt.h"

/* Begin Configs */
#define SAT_BASE        0x30000000UL
#define simCoreDBusDataWidth 64
#define satelliteIRQNum 5
#define nStateBus 4
/* End Configs */

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

#define SAT_N_RS (sat_pow2ceil(1 + nStateBus))
#define SAT_N_WS 2
#define SAT_N_RQ nStateBus
#define SAT_N_WQ nStateBus

#define SAT_RS_BASE (SAT_BASE)
#define SAT_WS_BASE (SAT_RS_BASE + SAT_N_RS * 8)
#define SAT_RQ_BASE (SAT_WS_BASE + SAT_N_WS * 8)
#define SAT_WQ_BASE (SAT_RQ_BASE + SAT_N_RQ * 8)

/// Read only Control Registers, 0 to pow2ceil(1 + nStateBus) bytes
/// 0: inSyncFlag
/// 1~ nStateBus: Receive buffer count
///  ~ pow2ceil(1 + nStateBus): 0
#define SAT_RS(offset) (SAT_RS_BASE + (offset) * 8)
/// Writable Control Registers, 0 to 2
/// 0: outSyncFlag
/// 1: nodeId
#define SAT_WS(offset) (SAT_WS_BASE + (offset) * 8)
/// Receive buffer, pop when read, 0 to nStateBus
/// Each for a StateBus
#define SAT_RQ(offset) (SAT_RQ_BASE + (offset) * 8)
/// Send buffer, push when written, 0 to nStateBus
/// Each for a StateBus
#define SAT_WQ(offset) (SAT_WQ_BASE + (offset) * 8)

#define SAT_inSyncFlag()  SAT_RS(0)
#define SAT_outSyncFlag() SAT_WS(0)
#define SAT_nodeId()      SAT_WS(1)

/// Must init before any usage, doesn't enable interrupt.
void sat_init(void);
/// Send to `target` with `payload[47:0]` though StateBus `n`. Block when full
void sat_send(int ch, uint16_t target, uint64_t payload);
/// Get a payload from StateBus `n`. Block when empty
uint64_t sat_recv(int ch);
uint64_t sat_bufferCnt(int ch);
void sat_clearBuffer(int ch);
void sat_set_outSyncFlag(uint64_t flag);
uint64_t sat_get_inSyncFlag();
/// Install interrupt server for recvFull, return old server. Also enable interrupt.
rt_isr_handler_t sat_interrupt_install(rt_isr_handler_t handler, void* param);

#endif // __DRV_SATELLITESTATION_H__
