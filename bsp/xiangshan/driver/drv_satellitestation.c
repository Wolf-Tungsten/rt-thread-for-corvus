#include <rtthread.h>
#include <riscv_io.h>
#include <stdint.h>
#include "drv_satellitestation.h"


#define SAT_MSG_PACK(dst, payload) \
    (((uint64_t)(dst) << 48) | ((uint64_t)(payload) & 0xFFFFFFFFFFFFULL))
#define SAT_MSG_DST(msg)     (((msg) >> 48) & 0xFFFF)
#define SAT_MSG_PAYLOAD(msg) ((msg) & 0xFFFFFFFFFFFFULL)


void sat_init(void) {
    // Don't use rt_hw_cpu_id()
    writeq_relaxed(0, (void*)(SAT_outSyncFlag()));
    writeq_relaxed(__raw_hartid(), (void*)(SAT_nodeId()));
}

void sat_send(int ch, uint16_t target, uint64_t payload) {
    RT_ASSERT(ch < nStateBus);
    uint64_t msg = SAT_MSG_PACK(target, payload);
    writeq_relaxed(msg, (void*)(SAT_WQ(ch)));
}

uint64_t sat_recv(int ch) {
    RT_ASSERT(ch < nStateBus);
    uint64_t msg = readq_relaxed((void*)(SAT_RQ(ch)));
    //RT_ASSERT(readq_relaxed((void*)SAT_nodeId()) == SAT_MSG_DST(msg)); // Seems not necessary
    return SAT_MSG_PAYLOAD(msg);
}

uint64_t sat_bufferCnt(int ch) {
    RT_ASSERT(ch < nStateBus);
    return readq_relaxed((void*)(SAT_RS(ch + 1)));
}

void sat_clearBuffer(int ch) {
    RT_ASSERT(ch < nStateBus);
    while (sat_bufferCnt(ch) > 0) {
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