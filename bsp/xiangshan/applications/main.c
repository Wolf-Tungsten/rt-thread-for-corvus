/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <rtthread.h>
#include <rthw.h>
#include <riscv_io.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "drv_satellitestation.h"

void test_sender(void);
void test_receiver(void);
void test_master(void);

#define SAT_TEST_TARGET 2
#define SAT_TEST_MSG_COUNT 4

static const uint64_t sat_test_mbus_payloads[SAT_TEST_MSG_COUNT] = {
    0xFFFFFFAFULL,
    0x123456789AULL,
    0x12345678ULL,
    0xDDDDD1ULL,
};

static const uint64_t sat_test_sbus_payloads[SAT_TEST_MSG_COUNT] = {
    0xABCD0001ULL,
    0xABCD0002ULL,
    0xABCD0003ULL,
    0xABCD0004ULL,
};

static void send_payloads(const char *bus_name, uint64_t ch, const uint64_t *payloads, uint64_t count, uint16_t target) {
    for (size_t i = 0; i < count; ++i) {
        rt_kprintf("Sending %s message %zu on channel %lu payload 0x%11lX to %hu\n",
                   bus_name,
                   (i + 1),
                   ch,
                   payloads[i],
                   target);
        sat_send(ch, target, payloads[i]);
    }
}

static bool drain_channel(uint64_t ch) {
    const char *bus_name = ch < sat_MBusCount ? "MBus" : "SBus";
    uint64_t count = sat_receiveBufferCnt(ch);

    if (count == 0) {
        return false;
    }

    rt_kprintf("channel %lu (%s) cnt is %lu\n", ch, bus_name, count);
    while (sat_receiveBufferCnt(ch) > 0) {
        rt_kprintf("payload is 0x%lX\n", sat_recv(ch));
    }
    return true;
}

int main(void) {
    rt_kprintf("Hello RISC-V\n");
    rt_kprintf("Sat Test\n");
    rt_kprintf("This is core %u\n", __raw_hartid());
    if (__raw_hartid() == 1) {
        test_sender();
    } else if (__raw_hartid() == 2) {
        test_receiver();
    } else if (__raw_hartid() == 0) {
        test_master();
    }
    rt_kprintf("\n****MAIN FINISH****\n");
    return 0;
}

void test_master(void){
    rt_kprintf("MASTER\n");
    sat_init();
    rt_kprintf("Local StateBus count is %lu (MBus=%lu, SBus=%lu)\n",
               sat_BusCount,
               sat_MBusCount,
               sat_SBusCount);
    sat_dump_regs();
    while(sat_receiveBufferCnt(SAT_MBUS_CH(0)) != SAT_TEST_MSG_COUNT){};
    drain_channel(SAT_MBUS_CH(0));
    rt_kprintf("DONE\n");
    return;
}

void test_sender(void) {
    rt_kprintf("SENDER\n");
    sat_init();
    rt_kprintf("Local StateBus count is %lu (MBus=%lu, SBus=%lu)\n",
               sat_BusCount,
               sat_MBusCount,
               sat_SBusCount);
    send_payloads("MBus", SAT_MBUS_CH(0), sat_test_mbus_payloads, SAT_TEST_MSG_COUNT,0);
    send_payloads("MBus", SAT_MBUS_CH(0), sat_test_mbus_payloads, SAT_TEST_MSG_COUNT,SAT_TEST_TARGET);
    send_payloads("SBus", SAT_SBUS_CH(0), sat_test_sbus_payloads, SAT_TEST_MSG_COUNT,SAT_TEST_TARGET);
    //send_payloads("SBus", SAT_SBUS_CH(0), sat_test_sbus_payloads, SAT_TEST_MSG_COUNT,0); // This is not allowed
    rt_kprintf("DONE\n");
}

static void sat_recv_handler(int vector, void *param) {
    (void)param;
    if(vector != satelliteIRQNum) {rt_kprintf("WHAT?\n"); return;}
    rt_kprintf("\nThis is reciver handler\n");

    for (uint64_t ch = 0; ch < sat_BusCount; ++ch) {
        if(drain_channel(ch)){break;}
    }
    rt_kprintf("DONE\n");
    return;
}

void test_receiver(void) {
    rt_kprintf("RECEIVER\n");
    sat_init();
    rt_kprintf("Local StateBus count is %lu (MBus=%lu, SBus=%lu)\n",
               sat_BusCount,
               sat_MBusCount,
               sat_SBusCount);
    sat_interrupt_install(sat_recv_handler, NULL);
    rt_thread_delay(30);
}
