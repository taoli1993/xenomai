/*
 * Copyright (C) 2001,2002 IDEALX (http://www.idealx.com/).
 * Written by Gilles Chanteperdrix <gilles.chanteperdrix@laposte.net>.
 * Copyright (C) 2003 Philippe Gerum <rpm@xenomai.org>.
 *
 * Xenomai is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _vxworks_defs_h
#define _vxworks_defs_h

#include <nucleus/xenomai.h>
#include <nucleus/registry.h>
#include <vxworks/vxworks.h>

#define WIND_MAGIC(n) (0x8383##n##n)
#define WIND_TASK_MAGIC WIND_MAGIC(01)
#define WIND_SEM_MAGIC  WIND_MAGIC(02)
#define WIND_WD_MAGIC   WIND_MAGIC(03)
#define WIND_MSGQ_MAGIC WIND_MAGIC(04)

/* Given a handle 'h', return a pointer to the control block of an
   object of type 't' whose magic word should be 'm'. */

#define wind_h2obj_active(h,m,t) \
    ((h) && (((void *)h) != (void *)ERROR) && ((t *)(h))->magic == (m) ? ((t *)(h)) : NULL)

/* Same as previously, but check for a deleted object, just returning
   a boolean value since the object would not be accessible if
   destroyed. The following test will remain valid until the destroyed
   object memory has been recycled for another usage. */

#define wind_mark_deleted(t) ((t)->magic = 0)

struct wind_sem;

typedef struct wind_sem wind_sem_t;

typedef struct sem_vtbl {

    STATUS (*take) (wind_sem_t *, xnticks_t);
    STATUS (*give) (wind_sem_t *);
    STATUS (*flush) (wind_sem_t *);
    const char *type;

} sem_vtbl_t;

struct wind_sem {

    unsigned int magic;

    xnholder_t link;

#define link2wind_sem(laddr) \
((wind_sem_t *)(((char *)laddr) - (int)(&((wind_sem_t *)0)->link)))

    xnsynch_t synchbase;

#define synch2wind_sem(saddr) \
((wind_sem_t *)(((char *)saddr) - (int)(&((wind_sem_t *)0)->synchbase)))

#ifdef CONFIG_XENO_OPT_REGISTRY
    xnhandle_t handle;
    char name[XNOBJECT_NAME_LEN];
#endif /* CONFIG_XENO_OPT_REGISTRY */

    /*
     * count has a different meaning for the different kinds of
     * semaphores : binary semaphore : binary state of the semaphore,
     * counting semaphore: the semaphore count, mutex: the recursion
     * count.
     */
    unsigned count;
    
    xnthread_t *owner;

    const sem_vtbl_t * vtbl;
};

typedef struct wind_msg {

    xnholder_t link;

#define link2wind_msg(laddr) \
((wind_msg_t *)(((char *)laddr) - (int)(&((wind_msg_t *)0)->link)))

    unsigned int length;
    
    char buffer[0];

} wind_msg_t;

typedef struct wind_msgq {

    unsigned int magic;
    
    UINT msg_length;

    xnholder_t * free_list;     /* simply linked list of free messages */

    xnqueue_t msgq;             /* queue of messages available for reading */

    xnholder_t link;            /* link in wind_msgq_t */

#define link2wind_msgq(laddr) \
((wind_msgq_t *)(((char *)laddr) - (int)(&((wind_msgq_t *)0)->link)))

    xnsynch_t synchbase;        /* pended readers or writers */

#ifdef CONFIG_XENO_OPT_REGISTRY
    xnhandle_t handle;
    char name[XNOBJECT_NAME_LEN];
#endif /* CONFIG_XENO_OPT_REGISTRY */

} wind_msgq_t;

typedef struct wind_tcb wind_task_t;

typedef struct wind_wd {

    unsigned magic;   /* Magic code - must be first */

    xnholder_t link;

#define link2wind_wd(laddr) \
((wind_wd_t *)(((char *)laddr) - (int)(&((wind_wd_t *)0)->link)))

    xntimer_t timerbase;

#ifdef CONFIG_XENO_OPT_REGISTRY
    xnhandle_t handle;
    char name[XNOBJECT_NAME_LEN];
#endif /* CONFIG_XENO_OPT_REGISTRY */

#if defined(__KERNEL__) && defined(CONFIG_XENO_OPT_PERVASIVE)
    xnsynch_t synchbase;
    wind_wd_utarget_t wdt;
#endif /* __KERNEL__ && CONFIG_XENO_OPT_PERVASIVE */

} wind_wd_t;

/* Internal flag marking a user-space task. */
#define VX_SHADOW (0x8000)

#define WIND_TASK_OPTIONS_MASK \
(VX_FP_TASK|VX_PRIVATE_ENV|VX_NO_STACK_FILL|VX_UNBREAKABLE|VX_SHADOW) 

#define wind_current_task() (thread2wind_task(xnpod_current_thread()))

/* The following macros return normalized or native priority values
   for the underlying pod. The core pod providing user-space support
   uses an ascending [0-256] priority scale (include/nucleus/core.h),
   whilst the VxWorks personality exhibits a decreasing scale
   [255-0]. Normalization is not needed when the underlying pod
   supporting the VxWorks skin is standalone, i.e. pure kernel, UVM or
   simulation modes. */

#if defined(__KERNEL__) && defined(CONFIG_XENO_OPT_PERVASIVE)
#define wind_normalized_prio(prio)	(XNCORE_MAX_PRIO - (prio) - 1)
#define wind_denormalized_prio(prio)	(255 - (prio))
#else /* !(__KERNEL__ && CONFIG_XENO_OPT_PERVASIVE) */
#define wind_normalized_prio(prio)	prio
#define wind_denormalized_prio(prio)	prio
#endif /* __KERNEL__ && CONFIG_XENO_OPT_PERVASIVE */

int *wind_errno_location(void);

static inline void wind_errnoset (int err)
{
    *xnthread_get_errno_location() = err;
}

static inline int wind_errnoget (void)
{
    return *xnthread_get_errno_location();
}

#define error_check(cond, status, action) do    \
{                                               \
    if ((cond))                                 \
        {                                       \
        wind_errnoset(status);                  \
        action;                                 \
        }                                       \
} while (0)


#define check_NOT_ISR_CALLABLE(action) do               \
{                                                       \
    if(xnpod_asynch_p())                                \
        {                                               \
        wind_errnoset(S_intLib_NOT_ISR_CALLABLE);       \
        action;                                         \
        }                                               \
} while(0)


#define check_alloc(type, ptr, action) do               \
{                                                       \
    ptr = (type *) xnmalloc (sizeof(type));             \
    if(!ptr)                                            \
        {                                               \
        wind_errnoset(S_memLib_NOT_ENOUGH_MEMORY);      \
        action;                                         \
        }                                               \
} while(0)


#define check_OBJ_ID_ERROR(id,type,ptr,magic,action) do \
{                                                       \
    ptr = wind_h2obj_active(id, magic, type);           \
    if(!ptr)                                            \
        {                                               \
        wind_errnoset(S_objLib_OBJ_ID_ERROR);           \
        action;                                         \
        }                                               \
} while(0)


/* Must be called with nklock locked, interrupts off. */
static inline void taskSafeInner (wind_task_t *cur)
{
    cur->safecnt++;
}

/* Must be called with nklock locked, interrupts off.
   Returns :
   - ERROR if the current context is invalid
   - OK if the safe count was zero or decremented but no rescheduling is needed.
   - 1 if the safe count was decremented and rescheduling is needed.
*/
static inline int taskUnsafeInner (wind_task_t *cur)
{
    if(!xnpod_primary_p())
        return ERROR;
    
    if (cur->safecnt == 0)
	return OK;

    if(--cur->safecnt == 0)
        return xnsynch_flush(&cur->safesync,0) == XNSYNCH_RESCHED;

    return OK;
}


/* modules initialization and cleanup: */
#ifdef __cplusplus
extern "C" {
#endif

    int wind_sysclk_init(u_long init_rate);

    void wind_sysclk_cleanup(void);
    

    void wind_task_init(void);

    void wind_task_cleanup(void);


    void wind_task_hooks_init(void);
    
    void wind_task_hooks_cleanup(void);


    void wind_sem_init(void);

    void wind_sem_cleanup(void);


    void wind_wd_init(void);

    void wind_wd_cleanup(void);


    void wind_msgq_init(void);

    void wind_msgq_cleanup(void);


    void wind_set_rrperiod( xnticks_t ticks );

    
#ifdef __cplusplus
}
#endif

#endif /* !_vxworks_defs_h */