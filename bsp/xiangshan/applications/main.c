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
#include <stdint.h>
#include "drv_satellitestation.h"

void test_sender(void);
void test_receiver(void);
int main(void)
{
    rt_kprintf("Hello RISC-V\n");
    rt_kprintf("Sat Test\n");
    rt_kprintf("This is core %u\n", __raw_hartid());
    if(__raw_hartid() == 1){
        test_sender();
    }else if(__raw_hartid() == 2){
        test_receiver();
    }
    rt_kprintf("\n****MAIN FINISH****\n");
    return 0;
}

void test_sender(void){
    rt_kprintf("SENDER\n");
    sat_init();
    rt_kprintf("Sending Message 1  0xFFFFFFAF\n");
    sat_send(0, 2, 0xFFFFFFAF);
    rt_kprintf("Sending Message 2  0x123456789A\n");
    sat_send(0, 2, 0x123456789A);
    rt_kprintf("Sending Message 3  0x12345678\n");
    sat_send(0, 2, 0x12345678ULL);
    rt_kprintf("Sending Message 4  0xDDDDD1\n");
    sat_send(0, 2, 0xDDDDD1ULL);
    rt_kprintf("DONE\n");
}

static void sat_recv_handler(int vector, void *param){
    if(vector != 5) {rt_kprintf("WHAT?\n"); return;}
    rt_kprintf("\nThis is reciver handler\n");

    rt_kprintf("cnt is %lld\n", sat_bufferCnt(0));
    while (sat_bufferCnt(0) > 0) {
        rt_kprintf("payload is 0x%llX\n",sat_recv(0));
    }
    rt_kprintf("DONE\n");
    return;
}

void test_receiver(void) {
    rt_kprintf("RECEIVER\n");
    sat_init();
    sat_interrupt_install(sat_recv_handler, NULL);
    rt_thread_delay(30);
}
