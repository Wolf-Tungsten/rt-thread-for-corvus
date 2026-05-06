#include <rtthread.h>
#include <riscv_io.h>
#include <stdint.h>
#include "drv_satellitestation.h"

uint64_t sat_SBusCount;
uint64_t sat_MBusCount;

#define SAT_MSG_PACK(dst, payload) \
    (((uint64_t)(dst) << 48) | ((uint64_t)(payload) & 0xFFFFFFFFFFFFULL))
#define SAT_MSG_DST(msg)     (((msg) >> 48) & 0xFFFF)
#define SAT_MSG_PAYLOAD(msg) ((msg) & 0xFFFFFFFFFFFFULL)

static void sat_assert_valid_channel(uint64_t ch)
{
    RT_ASSERT(ch >= 0);
    RT_ASSERT(ch < sat_BusCount);
}

static void sat_assert_valid_send_channel(uint64_t ch, uint16_t target){
    sat_assert_valid_channel(ch);
    // Hart0 is not connected to SBus
    if ( target == 0 ) RT_ASSERT(ch < sat_MBusCount);
}

void sat_init(void) {
    // Don't use rt_hw_cpu_id()
    sat_MBusCount = readq_relaxed((void*)SAT_RS(1));
    sat_SBusCount = readq_relaxed((void*)SAT_RS(2));
    writeq_relaxed((1 << syncTreeflagWidth) - 1, (void*)(SAT_outSyncFlag())); // full 1 means Pending, full 0 means Dangling
    writeq_relaxed(__raw_hartid(), (void*)(SAT_nodeId()));
}

void sat_send(uint64_t ch, uint16_t target, uint64_t payload) {
    sat_assert_valid_send_channel(ch, target);
    uint64_t msg = SAT_MSG_PACK(target, payload);
    writeq_relaxed(msg, (void*)(SAT_WQ(ch)));
}

uint64_t sat_recv(uint64_t ch) {
    sat_assert_valid_channel(ch);
    uint64_t msg = readq_relaxed((void*)(SAT_RQ(ch)));
    //RT_ASSERT(readq_relaxed((void*)SAT_nodeId()) == SAT_MSG_DST(msg)); // Seems not necessary
    return SAT_MSG_PAYLOAD(msg);
}

uint64_t sat_receiveBufferCnt(uint64_t ch) {
    sat_assert_valid_channel(ch);
    return readq_relaxed((void*)(SAT_TOCORE_COUNT(ch)));
}

uint64_t sat_sendBufferCnt(uint64_t ch) {
    sat_assert_valid_channel(ch);
    return readq_relaxed((void*)(SAT_FROMCORE_COUNT(ch)));
}

void sat_clearBuffer(uint64_t ch) {
    sat_assert_valid_channel(ch);
    while (sat_receiveBufferCnt(ch) > 0) {
        sat_recv(ch);
    }
}

void sat_set_outSyncFlag(uint64_t flag){
    writeq_relaxed(flag, (void*)SAT_outSyncFlag());
}

uint64_t sat_get_inSyncFlag(){
    return readq_relaxed((void*)SAT_inSyncFlag());
}

rt_isr_handler_t sat_interrupt_install(rt_isr_handler_t handler, void* param){
    rt_hw_plic_irq_enable(satelliteIRQNum);
    return rt_hw_interrupt_install(satelliteIRQNum, handler, param, "Sat recvFull");
}

uint64_t SAT_MBUS_CH(uint64_t ch) {
    RT_ASSERT(ch < sat_MBusCount)
    return ch;
}
uint64_t SAT_SBUS_CH(uint64_t ch) {
    RT_ASSERT(ch < sat_SBusCount)
    return sat_MBusCount + ch;
}

void sat_dump_regs(void)
{
    uint64_t i;
    uint64_t rs_used = 3 + 2 * sat_BusCount;

    rt_kprintf("SAT register map:\n");
    rt_kprintf("  SAT_RS_BASE                : 0x%08lX\n", (unsigned long)SAT_RS_BASE);
    rt_kprintf("  SAT_WS_BASE                : 0x%08lX\n", (unsigned long)SAT_WS_BASE);
    rt_kprintf("  SAT_RQ_BASE                : 0x%08lX\n", (unsigned long)SAT_RQ_BASE);
    rt_kprintf("  SAT_WQ_BASE                : 0x%08lX\n", (unsigned long)SAT_WQ_BASE);

    rt_kprintf("  SAT_inSyncFlag()           : 0x%08lX\n", (unsigned long)SAT_inSyncFlag());
    rt_kprintf("  SAT_RS(1) [nMbus]          : 0x%08lX\n", (unsigned long)SAT_RS(1));
    rt_kprintf("  SAT_RS(2) [nSbus]          : 0x%08lX\n", (unsigned long)SAT_RS(2));
    for (i = 0; i < sat_BusCount; i++) {
        rt_kprintf("  SAT_TOCORE_COUNT(%lu)        : 0x%08lX\n", i,
                   (unsigned long)SAT_TOCORE_COUNT(i));
    }
    for (i = 0; i < sat_BusCount; i++) {
        rt_kprintf("  SAT_FROMCORE_COUNT(%lu)      : 0x%08lX\n", i,
                   (unsigned long)SAT_FROMCORE_COUNT(i));
    }
    for (i = rs_used; i < (uint64_t)SAT_N_RS; i++) {
        rt_kprintf("  SAT_RS(%lu) [reserved]       : 0x%08lX\n", i,
                   (unsigned long)SAT_RS(i));
    }

    rt_kprintf("  SAT_outSyncFlag()          : 0x%08lX\n", (unsigned long)SAT_outSyncFlag());
    rt_kprintf("  SAT_nodeId()               : 0x%08lX\n", (unsigned long)SAT_nodeId());

    for (i = 0; i < (uint64_t)SAT_N_RQ; i++) {
        rt_kprintf("  SAT_RQ(%lu)                  : 0x%08lX\n", i,
                   (unsigned long)SAT_RQ(i));
    }
    for (i = 0; i < (uint64_t)SAT_N_WQ; i++) {
        rt_kprintf("  SAT_WQ(%lu)                  : 0x%08lX\n", i,
                   (unsigned long)SAT_WQ(i));
    }
}