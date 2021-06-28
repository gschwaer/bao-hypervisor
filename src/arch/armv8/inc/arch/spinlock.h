/*
 * Jailhouse AArch64 support
 *
 * Copyright (C) 2015 Huawei Technologies Duesseldorf GmbH
 *
 * Authors:
 *  Antonios Motakis <antonios.motakis@huawei.com>
 *
 * Spinlock implementation copied from
 * arch/arm64/include/asm/spinlock.h in Linux
 *
 * Copyright (C) 2012 ARM Ltd.
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 */

#ifndef _JAILHOUSE_ASM_SPINLOCK_H
#define _JAILHOUSE_ASM_SPINLOCK_H

#include <bao.h>

#define TICKET_SHIFT 16

/* TODO: fix this if we add support for BE */
typedef volatile struct {
    uint16_t owner;
    uint16_t next;
} spinlock_t __attribute__((aligned(4)));

#define SPINLOCK_INITVAL ((spinlock_t) {0})

/*
 * According to ARMv8 DDI 0487D.a, B2-108:
 * "The Load-Acquire, Load-AcquirePC, and Store-Release instructions
 *  can remove the requirement to use the explicit DMB instruction."
 *
 *  So no need explicit memory_barrier bound with spin_lock/unlock
 */
static inline void spin_lock(spinlock_t *lock)
{
    unsigned int tmp;
    spinlock_t lockval, newval;

    asm volatile(
        /* Atomically increment the next ticket. */
        "	prfm	pstl1strm, %3\n"
        "1:	ldaxr	%w0, %3\n"
        "	add	%w1, %w0, %w5\n"
        "	stxr	%w2, %w1, %3\n"
        "	cbnz	%w2, 1b\n"
        /* Did we get the lock? */
        "	eor	%w1, %w0, %w0, ror #16\n"
        "	cbz	%w1, 3f\n"
        /*
         * No: spin on the owner. Send a local event to avoid missing an
         * unlock before the exclusive load.
         */
        "	sevl\n"
        "2:	wfe\n"
        "	ldaxrh	%w2, %4\n"
        "	eor	%w1, %w2, %w0, lsr #16\n"
        "	cbnz	%w1, 2b\n"
        /* We got the lock. Critical section starts here. */
        "3:"
        : "=&r"(lockval), "=&r"(newval), "=&r"(tmp), "+Q"(*lock)
        : "Q"(lock->owner), "I"(1 << TICKET_SHIFT)
        : "memory");
}

/*
 * See spin_lock: This implementation implies a memory barrier.
 */
static inline void spin_unlock(spinlock_t *lock)
{
    asm volatile("	stlrh	%w1, %0\n"
                 : "=Q"(lock->owner)
                 : "r"(lock->owner + 1)
                 : "memory");
}

#endif /* !_JAILHOUSE_ASM_SPINLOCK_H */

///**
// * Bao, a Lightweight Static Partitioning Hypervisor
// *
// * Copyright (c) Bao Project (www.bao-project.org), 2019-
// *
// * Authors:
// *      Jose Martins <jose.martins@bao-project.org>
// *
// * Bao is free software; you can redistribute it and/or modify it under the
// * terms of the GNU General Public License version 2 as published by the Free
// * Software Foundation, with a special exception exempting guest code from
// such
// * license. See the COPYING file in the top-level directory for details.
// *
// */
//
///**
// * TODO: this is a naive implementation to get things going.
// * Optimizations needed. See ticket locks in ARMv8-A.
// */
//
//#ifndef __ARCH_SPINLOCK__
//#define __ARCH_SPINLOCK__
//
//#include <bao.h>
//
// typedef volatile uint32_t spinlock_t;
//
//#define SPINLOCK_INITVAL (0)
//
// static inline void spin_lock(spinlock_t* lock)
//{
//    uint32_t const ONE = 1;
//    spinlock_t tmp;
//
//    asm volatile(
//        "1:\n\t"
//        "ldaxr %w0, %1 \n\t"
//        "cbnz %w0, 1b \n\t"
//        "stxr %w0, %w2, %1 \n\t"
//        "cbnz %w0, 1b \n\t"
//        : "=&r"(tmp), "+Q"(*lock)
//        : "r"(ONE));
//}
//
// static inline void spin_unlock(spinlock_t* lock)
//{
//    asm volatile("stlr wzr, %0\n\t" ::"Q"(*lock));
//}
//
//#endif /* __ARCH_SPINLOCK__ */
