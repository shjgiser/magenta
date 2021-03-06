// Copyright 2016 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <err.h>
#include <string.h>
#include <sys/types.h>
#include <arch/arm64.h>
#include <arch/debugger.h>
#include <kernel/thread.h>
#include <magenta/syscalls/debug.h>

uint arch_num_regsets(void)
{
    return 1; // TODO(dje): Just the general regs for now.
}

static status_t arch_get_general_regs(struct thread *thread, mx_arm64_general_regs_t *out, uint32_t *buf_size)
{
    uint32_t provided_buf_size = *buf_size;
    *buf_size = sizeof(*out);

    // Do "buffer too small" checks first. No point in prohibiting the caller
    // from finding out the needed size just because the thread is currently
    // running.
    if (provided_buf_size < sizeof(*out))
        return ERR_BUFFER_TOO_SMALL;

    if (!thread_stopped_in_exception(thread))
        return ERR_BAD_STATE;

    struct arm64_iframe_long *in = thread->exception_context->frame;

    // TODO: We could get called while processing a synthetic exception where
    // there is no frame.
    if (in == NULL)
        return ERR_NOT_SUPPORTED;

    static_assert(sizeof(in->r) == sizeof(out->r), "");
    memcpy(&out->r[0], &in->r[0], sizeof(in->r));
    out->lr = in->lr;
    out->sp = in->usp;
    out->pc = in->elr;
    out->cpsr = in->spsr;

    return NO_ERROR;
}

static status_t arch_set_general_regs(struct thread *thread, const mx_arm64_general_regs_t *in, uint32_t buf_size)
{
    if (buf_size != sizeof(*in))
        return ERR_INVALID_ARGS;

    if (!thread_stopped_in_exception(thread))
        return ERR_BAD_STATE;

    struct arm64_iframe_long *out = thread->exception_context->frame;

    // TODO: We could get called while processing a synthetic exception where
    // there is no frame.
    if (out == NULL)
        return ERR_NOT_SUPPORTED;

    static_assert(sizeof(out->r) == sizeof(in->r), "");
    memcpy(&out->r[0], &in->r[0], sizeof(in->r));
    out->lr = in->lr;
    out->usp = in->sp;
    out->elr = in->pc;
    out->spsr = in->cpsr;

    return NO_ERROR;
}

status_t arch_get_regset(struct thread *thread, uint regset, void *regs, uint32_t *buf_size)
{
    switch (regset)
    {
    case 0:
        return arch_get_general_regs(thread, (mx_arm64_general_regs_t *)regs, buf_size);
    default:
        return ERR_INVALID_ARGS;
    }
}

status_t arch_set_regset(struct thread *thread, uint regset, const void *regs, uint32_t buf_size, bool priv)
{
    switch (regset)
    {
    case 0:
        return arch_set_general_regs(thread, (mx_arm64_general_regs_t *)regs, buf_size);
    default:
        return ERR_INVALID_ARGS;
    }
}
