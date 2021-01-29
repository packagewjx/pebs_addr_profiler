#undef TRACE_SYSTEM
#define TRACE_SYSTEM pebs_addr

#if !defined(_TRACE_PEBS_ADDR_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_PEBS_ADDR_H

#include <linux/tracepoint.h>

TRACE_EVENT(pebs_addr,
        TP_PROTO(u64 dla),
        TP_ARGS(dla),
        TP_STRUCT__entry(
            __field(u64, dla)
        ),
        TP_fast_assign(
            __entry->dla = dla;
        ),
        TP_printk("%llx",
            __entry->dla)
        );

#endif //_TRACE_PEBS_H

#include <trace/define_trace.h>
