#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/percpu.h>
#include <linux/module.h>
#include <linux/version.h>
#include <asm/msr.h>
#include <asm/processor.h>
#include <asm/cpufeature.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
//#include </home/wjx/linux-5.4.0/arch/x86/events/perf_event.h>

#define CREATE_TRACE_POINTS

#include "pebs_addr.h"

struct pebs_addr {
    u64 __unused[19];
    u64 dla;
    u64 __unused2[5];
};

struct cpu_hw_events;

// copy of <arch/x86/events/perf_event.h>, kernel version 5.4.0
struct x86_pmu {
    /*
     * Generic x86 PMC bits
     */
    const char *name;
    int version;

    int (*handle_irq)(struct pt_regs *);

    void (*disable_all)(void);

    void (*enable_all)(int added);

    void (*enable)(struct perf_event *);

    void (*disable)(struct perf_event *);

    void (*add)(struct perf_event *);

    void (*del)(struct perf_event *);

    void (*read)(struct perf_event *event);

    int (*hw_config)(struct perf_event *event);

    int (*schedule_events)(struct cpu_hw_events *cpuc, int n, int *assign);

    unsigned eventsel;
    unsigned perfctr;

    int (*addr_offset)(int index, bool eventsel);

    int (*rdpmc_index)(int index);

    u64 (*event_map)(int);

    int max_events;
    int num_counters;
    int num_counters_fixed;
    int cntval_bits;
    u64 cntval_mask;
    union {
        unsigned long events_maskl;
        unsigned long events_mask[BITS_TO_LONGS(ARCH_PERFMON_EVENTS_COUNT)];
    };
    int events_mask_len;
    int apic;
    u64 max_period;

    struct event_constraint *
    (*get_event_constraints)(struct cpu_hw_events *cpuc,
                             int idx,
                             struct perf_event *event);

    void (*put_event_constraints)(struct cpu_hw_events *cpuc,
                                  struct perf_event *event);

    void (*start_scheduling)(struct cpu_hw_events *cpuc);

    void (*commit_scheduling)(struct cpu_hw_events *cpuc, int idx, int cntr);

    void (*stop_scheduling)(struct cpu_hw_events *cpuc);

    struct event_constraint *event_constraints;
    struct x86_pmu_quirk *quirks;
    int perfctr_second_write;

    u64 (*limit_period)(struct perf_event *event, u64 l);

    /* PMI handler bits */
    unsigned int late_ack: 1,
            counter_freezing: 1;
    /*
     * sysfs attrs
     */
    int attr_rdpmc_broken;
    int attr_rdpmc;
    struct attribute **format_attrs;

    ssize_t (*events_sysfs_show)(char *page, u64 config);

    const struct attribute_group **attr_update;

    unsigned long attr_freeze_on_smi;

    /*
     * CPU Hotplug hooks
     */
    int (*cpu_prepare)(int cpu);

    void (*cpu_starting)(int cpu);

    void (*cpu_dying)(int cpu);

    void (*cpu_dead)(int cpu);

    void (*check_microcode)(void);

    void (*sched_task)(struct perf_event_context *ctx,
                       bool sched_in);

    /*
     * Intel Arch Perfmon v2+
     */
    u64 intel_ctrl;
//    union perf_capabilities intel_cap;
    u64 intel_cap;

    /*
     * Intel DebugStore bits
     */
    unsigned int bts: 1,
            bts_active: 1,
            pebs: 1,
            pebs_active: 1,
            pebs_broken: 1,
            pebs_prec_dist: 1,
            pebs_no_tlb: 1,
            pebs_no_isolation: 1;
    int pebs_record_size;
    int pebs_buffer_size;
    int max_pebs_events;

    void (*drain_pebs)(struct pt_regs *regs);
};

static unsigned pebs_record_size = sizeof(struct pebs_addr);

static DEFINE_PER_CPU(struct debug_store *, ds_base);

static struct x86_pmu* pmu;

static void (*original_pebs_drain)(struct pt_regs *iregs);

static void pebs_address_profiler(struct pt_regs *regs)
{
    struct debug_store *ds;
    void *pebs;

    ds = this_cpu_read(ds_base);
    if (!ds) {
        u64 dsval;
        rdmsrl(MSR_IA32_DS_AREA, dsval);
        ds = (struct debug_store *) dsval;
        this_cpu_write(ds_base, ds);
    }

    for (pebs = (void *)ds->pebs_buffer_base;
         pebs < (void *)ds->pebs_index;
         pebs = pebs + pebs_record_size) {
        struct pebs_addr *v = pebs;
        trace_pebs_addr(current->pid, v->dla);
    }
}

static int init_pebs_addr_profiler(void)
{
    u64 eax, cap;
    unsigned int pebs_version;

    if (!boot_cpu_has(X86_FEATURE_ARCH_PERFMON)) {
        pr_err("Arch perfmon not supported\n");
        return -EIO;
    }

    eax = cpuid_eax(10);
    if ((eax & 0xff) < 2) {
        pr_err("Need at least version 2 of arch_perfmon, not %llu\n",
               eax & 0xff);
        return -EIO;
    }

    rdmsrl(MSR_IA32_PERF_CAPABILITIES, cap);
    pebs_version = (cap >> 8) & 0xf;
    pr_info("PEBS version %u\n", pebs_version);

    if (pebs_version != 3) {
        pr_err("Unsupported PEBS version  %u\n", pebs_version);
        return -EIO;
    }

    pmu = (struct x86_pmu*) kallsyms_lookup_name("x86_pmu");
    original_pebs_drain = pmu->drain_pebs;
    pmu->drain_pebs = pebs_address_profiler;

    return 0;
}

static void exit_pebs_addr_profiler(void)
{
    pmu->drain_pebs = original_pebs_drain;
}

module_init(init_pebs_addr_profiler);
module_exit(exit_pebs_addr_profiler);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junxian Wu");
MODULE_DESCRIPTION("Get Data Linear Address from the PEBS Record");
