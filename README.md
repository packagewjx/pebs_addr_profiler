# pebs_addr_profiler
Grab memory access address from PEBS record. A modified version of [pmu-tools/pebs-grabber](https://github.com/andikleen/pmu-tools/tree/master/pebs-grabber).

# Installation

```
make
insmod pebs_addr_profiler.ko # root previleges required
```

Check `dmesg`, you should see a new message like `PEBS version #`, where `#` is the PEBS record version number.

# Using it with perf

This tool added a trace event `pebs_addr:pebs_addr`. This trace event will be triggered when you use perf to record any PEBS-enabled events,
whose description in `perf list` says `support Precise`, and used with ":P". This tool is used to collect memory access addresses, 
so natually you should use it with memory events such as "mem_inst_retired.all_loads:P".

## Example

```
perf record -e mem_inst_retired.all_loads:P -e pebs_addr:pebs_addr -c <overflow_count> <command>
perf script # check out the output to find the address collected
```

# Uninstall

With root previleges, run `rmmod pebs_addr_profiler`.
