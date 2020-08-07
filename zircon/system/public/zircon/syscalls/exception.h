// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSROOT_ZIRCON_SYSCALLS_EXCEPTION_H_
#define SYSROOT_ZIRCON_SYSCALLS_EXCEPTION_H_

#include <zircon/compiler.h>
#include <zircon/types.h>

__BEGIN_CDECLS

// ask clang format not to mess up the indentation:
// clang-format off

// The following exception values were chosen for historical reasons.

// Architectural exceptions.
//
// Depending on the exception, further information can be found in
// |report.context.arch|.

#define ZX_EXCP_GENERAL                 ((uint32_t) 0x008u)
#define ZX_EXCP_FATAL_PAGE_FAULT        ((uint32_t) 0x108u)
#define ZX_EXCP_UNDEFINED_INSTRUCTION   ((uint32_t) 0x208u)
#define ZX_EXCP_SW_BREAKPOINT           ((uint32_t) 0x308u)
#define ZX_EXCP_HW_BREAKPOINT           ((uint32_t) 0x408u)
#define ZX_EXCP_UNALIGNED_ACCESS        ((uint32_t) 0x508u)

// Synthetic exceptions.

// These bits are set for synthetic exceptions to distinguish them from
// architectural exceptions.
#define ZX_EXCP_SYNTH                   ((uint32_t) 0x8000u)

// A thread is starting.
// This exception is sent to debuggers only (ZX_EXCEPTION_CHANNEL_TYPE_DEBUGGER).
// The thread that generates this exception is paused until it the debugger
// handles the exception.
#define ZX_EXCP_THREAD_STARTING         ((uint32_t) 0x008u | ZX_EXCP_SYNTH)

// A thread is exiting.
// This exception is sent to debuggers only (ZX_EXCEPTION_CHANNEL_TYPE_DEBUGGER).
// This exception is different from ZX_EXCP_GONE in that a debugger can
// still examine thread state.
// The thread that generates this exception is paused until it the debugger
// handles the exception.
#define ZX_EXCP_THREAD_EXITING          ((uint32_t) 0x108u | ZX_EXCP_SYNTH)

// This exception is generated when a syscall fails with a job policy error (for
// example, an invalid handle argument is passed to the syscall when the
// ZX_POL_BAD_HANDLE policy is enabled) and ZX_POL_ACTION_EXCEPTION is set for
// the policy. The thread that generates this exception is paused until it the
// debugger handles the exception. Additional data about the type of policy
// error can be found in the |synth_code| field of the report and will be a
// ZX_EXCP_POLICY_CODE_* value.
#define ZX_EXCP_POLICY_ERROR            ((uint32_t) 0x208u | ZX_EXCP_SYNTH)

#define ZX_EXCP_POLICY_CODE_BAD_HANDLE            0u
#define ZX_EXCP_POLICY_CODE_WRONG_OBJECT          1u
#define ZX_EXCP_POLICY_CODE_VMAR_WX               2u
#define ZX_EXCP_POLICY_CODE_NEW_ANY               3u
#define ZX_EXCP_POLICY_CODE_NEW_VMO               4u
#define ZX_EXCP_POLICY_CODE_NEW_CHANNEL           5u
#define ZX_EXCP_POLICY_CODE_NEW_EVENT             6u
#define ZX_EXCP_POLICY_CODE_NEW_EVENTPAIR         7u
#define ZX_EXCP_POLICY_CODE_NEW_PORT              8u
#define ZX_EXCP_POLICY_CODE_NEW_SOCKET            9u
#define ZX_EXCP_POLICY_CODE_NEW_FIFO              10u
#define ZX_EXCP_POLICY_CODE_NEW_TIMER             11u
#define ZX_EXCP_POLICY_CODE_NEW_PROCESS           12u
#define ZX_EXCP_POLICY_CODE_NEW_PROFILE           13u
#define ZX_EXCP_POLICY_CODE_AMBIENT_MARK_VMO_EXEC 14u
#define ZX_EXCP_POLICY_CODE_CHANNEL_FULL_WRITE    15u
#define ZX_EXCP_POLICY_CODE_PORT_TOO_MANY_PACKETS 16u
#define ZX_EXCP_POLICY_CODE_BAD_SYSCALL           17u

// A process is starting.
// This exception is sent to job debuggers only
// (ZX_EXCEPTION_CHANNEL_TYPE_JOB_DEBUGGER).
// The thread that generates this exception is paused until it the debugger
// handles the exception.
#define ZX_EXCP_PROCESS_STARTING        ((uint32_t) 0x308u | ZX_EXCP_SYNTH)

typedef uint32_t zx_excp_type_t;

// Assuming |excp| is an exception type, the following returns true if the
// type is architectural.
#define ZX_EXCP_IS_ARCH(excp)  (((excp) & ZX_EXCP_SYNTH) == 0)

typedef struct zx_x86_64_exc_data {
    uint64_t vector;
    uint64_t err_code;
    uint64_t cr2;
} zx_x86_64_exc_data_t;

typedef struct zx_arm64_exc_data_v1 {
    uint32_t esr;
    uint8_t padding1[4];
    uint64_t far;
} zx_arm64_exc_data_v1_t;

typedef struct zx_arm64_exc_data {
    uint32_t esr;
    uint8_t padding1[4];
    uint64_t far;
    uint8_t padding2[8];
} zx_arm64_exc_data_t;

typedef struct zx_riscv64_exc_data_v1 {
    uint32_t placeholder;
} zx_riscv64_exc_data_v1_t;

typedef struct zx_riscv64_exc_data {
    uint32_t placeholder;
    uint8_t padding2[20];
} zx_riscv64_exc_data_t;

typedef struct zx_exception_context_v1 {
    struct {
        union {
            zx_x86_64_exc_data_t x86_64;
            struct {
              zx_arm64_exc_data_v1_t arm_64;
              uint8_t padding1[8];
            };
            struct {
              zx_riscv64_exc_data_v1_t  riscv_64;
              uint8_t padding2[20];
            };
        } u;
    } arch;
} zx_exception_context_v1_t;

// data associated with an exception (siginfo in linux parlance)
// Things available from regsets (e.g., pc) are not included here.
// For an example list of things one might add, see linux siginfo.
typedef struct zx_exception_context {
    struct {
        union {
            zx_x86_64_exc_data_t x86_64;
            zx_arm64_exc_data_t arm_64;
            zx_riscv64_exc_data_t riscv_64;
        } u;
    } arch;
    uint32_t synth_code;
    uint32_t synth_data;
} zx_exception_context_t;

// The common header of all exception reports.
typedef struct zx_exception_header {
    // The actual size, in bytes, of the report (including this field).
    uint32_t size;

    zx_excp_type_t type;
} zx_exception_header_t;

// Data reported to an exception handler for most exceptions.
typedef struct zx_exception_report_v1 {
    zx_exception_header_t header;
    // The remainder of the report is exception-specific.
    zx_exception_context_v1_t context;
} zx_exception_report_v1_t;

typedef struct zx_exception_report {
    zx_exception_header_t header;
    zx_exception_context_t context;
} zx_exception_report_t;

// Basic info sent along with the handle over an exception channel.
typedef struct zx_exception_info {
    zx_koid_t pid;
    zx_koid_t tid;
    zx_excp_type_t type;
    uint8_t padding1[4];
} zx_exception_info_t;

// Options for zx_create_exception_channel.
// When creating an exception channel, use the task's debug channel.
#define ZX_EXCEPTION_CHANNEL_DEBUGGER ((uint32_t)1)

// The type of exception handler a thread may be waiting for a response from.
// These values are reported in zx_info_thread_t.wait_exception_channel_type.
#define ZX_EXCEPTION_CHANNEL_TYPE_NONE         ((uint32_t)0u)
#define ZX_EXCEPTION_CHANNEL_TYPE_DEBUGGER     ((uint32_t)1u)
#define ZX_EXCEPTION_CHANNEL_TYPE_THREAD       ((uint32_t)2u)
#define ZX_EXCEPTION_CHANNEL_TYPE_PROCESS      ((uint32_t)3u)
#define ZX_EXCEPTION_CHANNEL_TYPE_JOB          ((uint32_t)4u)
#define ZX_EXCEPTION_CHANNEL_TYPE_JOB_DEBUGGER ((uint32_t)5u)

__END_CDECLS

#endif // SYSROOT_ZIRCON_SYSCALLS_EXCEPTION_H_
