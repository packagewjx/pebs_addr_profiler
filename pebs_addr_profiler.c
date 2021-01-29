#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/percpu.h>
#include <linux/module.h>
#include <linux/version.h>
#include <asm/msr.h>
#include <asm/processor.h>
#include <asm/cpufeature.h>
#define CREATE_TRACE_POINTS
#include "pebs_addr.h"

struct pebs_addr {
    u64 __unused[19];
    u64 dla;
    u64 __unused2[5];
};

static unsigned pebs_record_size = sizeof(struct pebs_addr);

static DEFINE_PER_CPU(struct debug_store *, ds_base);

static int pebs_address_profiler(struct kprobe *kp, struct pt_regs *regs)
{
    struct debug_store *ds;
    void *pebs;

    ds = this_cpu_read(ds_base);
    if (!ds) {
        u64 dsval;
        rdmsrl(MSR_IA32_DS_AREA, dsval);
        ds = (struct debug_store *)dsval;
        this_cpu_write(ds_base, ds);
    }

    for (pebs = (void *)ds->pebs_buffer_base;
         pebs < (void *)ds->pebs_index;
         pebs = pebs + pebs_record_size) {
        struct pebs_addr *v = pebs;
        trace_pebs_addr(v->dla);
    }
    return 0;
}

static struct kprobe pebs_kp = {
        .symbol_name = "intel_pmu_drain_pebs_nhm",
        .pre_handler = pebs_address_profiler
};

static int init_pebs_addr_profiler(void)
{
    int err;
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

    if ((err = register_kprobe(&pebs_kp)) < 0) {
        pr_err("Cannot register kprobe: %d\n", err);
        return err;
    }
    return 0;
}

static void exit_pebs_addr_profiler(void)
{
    unregister_kprobe(&pebs_kp);
}

module_init(init_pebs_addr_profiler);
module_exit(exit_pebs_addr_profiler);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junxian Wu");
MODULE_DESCRIPTION("Get Data Linear Address from the PEBS Record");
