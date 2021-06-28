#include <fp_sched.h>

#include <vm.h>
#include <cpu.h>
#include <spinlock.h>
#include <interrupts.h>
#include <arch/gtimer.h_only>

#define TOKEN_NULL_OWNER    -1
#define TOKEN_NULL_PRIORITY -1

static spinlock_t memory_lock                     = SPINLOCK_INITVAL;
static volatile int64_t memory_requests[NUM_CPUS] = { [0 ... NUM_CPUS-1] = TOKEN_NULL_PRIORITY };
static volatile int64_t token_owner               = TOKEN_NULL_OWNER;
static volatile int64_t token_priority            = TOKEN_NULL_PRIORITY;

enum { INJECT_SGI = 1 };

void inter_vm_irq_handler(uint32_t event, uint64_t data) {
    if(event == INJECT_SGI) {
        interrupts_vm_inject(cpu.vcpu->vm, data);
    }
}
CPU_MSG_HANDLER(inter_vm_irq_handler, INTER_VM_IRQ);

#define CPU_MSG(handler,event,data) (&(cpu_msg_t){handler, event, data})

uint64_t fp_request_access(uint64_t dec_prio)
{
    uint64_t ts_entry = gtimer_get_ticks();

    int64_t priority;
    int got_token;
    uint64_t response;
    uint64_t ts_ipi_start = 0, ts_ipi_end = 0;
    uint64_t ts_start, ts_end;
    uint64_t o_hyptrap, o_lock, o_arbitration, o_send_ipi, o_unlock;
    bool is_higher_prio;

    // the guest mangles together ts_start and dec_prio
    ts_start = dec_prio & ((1ULL << 60) - 1); // unmangle ts_start
    dec_prio = dec_prio >> 60; // unmangle prio
    o_hyptrap = gtimer_ticks_to_nano_seconds(ts_entry - ts_start);
    ts_start = gtimer_get_ticks();

    // using increasing priorities for the rest of the code
    priority = (int64_t)(NUM_CPUS - dec_prio);

    spin_lock(&memory_lock);

    ts_end = gtimer_get_ticks();
    o_lock = gtimer_ticks_to_nano_seconds(ts_end - ts_start);
    ts_start = gtimer_get_ticks();

    memory_requests[cpu.id] = priority;

    is_higher_prio = priority > token_priority;
    if (is_higher_prio) {
        if (token_owner != TOKEN_NULL_OWNER &&
            (uint64_t)token_owner != cpu.id) {
//            INFO("Send interrupt to reclaim the token from %d and give to %d",
//                 token_owner,
//                 cpu.id);
            ts_ipi_start = gtimer_get_ticks();
            cpu_send_msg((uint64_t)token_owner,
                         CPU_MSG(INTER_VM_IRQ,
                                 INJECT_SGI,
                                 FP_IPI_PAUSE));
            ts_ipi_end = gtimer_get_ticks();
        }
        token_owner    = (int64_t)cpu.id;
        token_priority = priority;
    }

    got_token = (token_owner == (int64_t)cpu.id);
    response = got_token ? FP_REQ_RESP_ACK : FP_REQ_RESP_NACK;

    ts_end = gtimer_get_ticks();
    o_send_ipi = gtimer_ticks_to_nano_seconds(ts_ipi_end - ts_ipi_start);
    o_arbitration = gtimer_ticks_to_nano_seconds(ts_end - ts_start) - o_send_ipi;
    ts_start = gtimer_get_ticks();

    spin_unlock(&memory_lock);

    ts_end = gtimer_get_ticks();
    o_unlock = gtimer_ticks_to_nano_seconds(ts_end - ts_start);

    if (cpu.id == 2) {
        // o_hyptrap, o_lock, o_arbitration, o_send_ipi, o_unlock, has_higher_prio:
        printk("req, %lu, %lu, %lu, %lu, %lu, %u\n", o_hyptrap, o_lock, o_arbitration, o_send_ipi, o_unlock, is_higher_prio);
    }
    ts_start = gtimer_get_ticks();
    response = (response << 60) | (ts_start & ((1ULL << 60) - 1)); // mangle ts_start into response
    return response;
}

void fp_revoke_access()
{
    uint64_t ts_start, ts_end;
    uint64_t ts_ipi_start = 0, ts_ipi_end = 0;
    uint64_t o_lock, o_arbitration, o_send_ipi, o_unlock;

    ts_start = gtimer_get_ticks();
    spin_lock(&memory_lock);
    ts_end = gtimer_get_ticks();
    o_lock = gtimer_ticks_to_nano_seconds(ts_end - ts_start);
    ts_start = gtimer_get_ticks();

    memory_requests[cpu.id] = TOKEN_NULL_PRIORITY;

    if(token_owner == (int64_t)cpu.id)
    {
        token_owner    = TOKEN_NULL_OWNER;
        token_priority = TOKEN_NULL_PRIORITY;

        for (uint64_t cpu = 0; cpu < NUM_CPUS; ++cpu) {
            if (memory_requests[cpu] > token_priority) {
                token_priority = memory_requests[cpu];
                token_owner    = (int64_t)cpu;
            }
        }

        if (token_owner != TOKEN_NULL_OWNER) {
//            INFO("Send interrupt to pass the token from %d to %d", cpu.id, token_owner);
            ts_ipi_start = gtimer_get_ticks();
            cpu_send_msg((uint64_t)token_owner,
                         CPU_MSG(INTER_VM_IRQ,
                                 INJECT_SGI,
                                 FP_IPI_RESUME));
            ts_ipi_end = gtimer_get_ticks();
        }
    }

    ts_end = gtimer_get_ticks();
    o_send_ipi = gtimer_ticks_to_nano_seconds(ts_ipi_end - ts_ipi_start);
    o_arbitration = gtimer_ticks_to_nano_seconds(ts_end - ts_start) - o_send_ipi;
    ts_start = gtimer_get_ticks();
    spin_unlock(&memory_lock);
    ts_end = gtimer_get_ticks();
    o_unlock = gtimer_ticks_to_nano_seconds(ts_end - ts_start);

    if (cpu.id == 2) {
        // o_lock, o_arbitration, o_send_ipi, o_unlock:
        printk("rev, %lu, %lu, %lu, %lu\n", o_lock, o_arbitration, o_send_ipi, o_unlock);
    }
}
