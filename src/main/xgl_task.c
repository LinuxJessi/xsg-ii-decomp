#include "common.h"
#include "shared.h"

typedef struct XglTaskPrefix XglTaskPrefix;
typedef struct XglTaskScheduler XglTaskScheduler;

/*
 * Only the link/callback prefix is evidenced by this allocation.  The
 * complete task records are allocated with a 0x80-byte stride, and callers
 * write task-specific payload beyond this 0x10-byte prefix.  This declaration
 * must not be read as the complete task layout or allocation size.
 */
struct XglTaskPrefix {
    XglTaskScheduler *scheduler;
    XglTaskPrefix *next;
    XglTaskPrefix *previous;
    int (*callback)(XglTaskPrefix *task);
};

struct XglTaskScheduler {
    XglTaskPrefix *free_tasks;
    XglTaskPrefix *active_head;
    XglTaskPrefix *active_tail;
    XglTaskPrefix *next_to_visit;
};

extern XglTaskPrefix *xglTaskEntryNext(XglTaskScheduler *scheduler,
                                int (*callback)(XglTaskPrefix *task),
                                XglTaskPrefix *entry);

int xglTaskWaitRemove(XglTaskPrefix *task);

void xglTaskExecute(XglTaskScheduler *scheduler);
void *xglTaskInitial(void *manager, int capacity, int reset_gs);

int xglTaskRemove(XglTaskPrefix *task);

extern XglTaskPrefix *xglTaskEntryPrev(XglTaskScheduler *scheduler,
                                int (*callback)(XglTaskPrefix *task),
                                XglTaskPrefix *entry);

extern void *xglTaskInitial(void *manager, int capacity, int reset_gs);

/* Every task slot of the pool reserves this many bytes. */
#define XGL_TASK_SLOT_SIZE 0x80

extern int sceGsResetGraph(int interlace, int omode, int ffmode, int errorjump);

/*
 * manager is a caller-allocated block: an XglTaskScheduler header followed
 * immediately by a capacity-element task pool, each element reserving
 * 0x80 bytes although only the leading XglTaskPrefix of it is modeled
 * (include/shared.h). This links every element into the scheduler's free
 * list and returns the address right after the pool. A nonzero reset_gs
 * resets the GS graph first (sceGsResetGraph, interlaced).
 */
void *xglTaskInitial(void *manager, int capacity, int reset_gs)
{
    XglTaskScheduler *scheduler;
    XglTaskPrefix *task;
    XglTaskPrefix *next;
    void *afterPool;
    int remaining;

    afterPool = 0;
    scheduler = (XglTaskScheduler *) manager;
    task = (XglTaskPrefix *)((char *) manager + sizeof(XglTaskScheduler));
    if (capacity != 0)
    {
        if (reset_gs != 0)
        {
            sceGsResetGraph(1, 0, 0, 0);
        }
        remaining = capacity - 1;
        scheduler->free_tasks = task;
        if (remaining > 0)
        {
            do
            {
                next = (XglTaskPrefix *)((char *) task + XGL_TASK_SLOT_SIZE);
                remaining -= 1;
                task->next = next;
                task = next;
            }
            while (remaining != 0);
        }
        task->next = 0;
        afterPool = (char *) task + XGL_TASK_SLOT_SIZE;
        scheduler->active_head = 0;
        scheduler->active_tail = 0;
    }
    return afterPool;
}

XglTaskPrefix *xglTaskEntryPrev(XglTaskScheduler *scheduler,
                                int (*callback)(XglTaskPrefix *task),
                                XglTaskPrefix *entry)
{
    XglTaskPrefix *task;
    XglTaskPrefix *previous;

    if (scheduler == 0 || (task = scheduler->free_tasks) == 0)
        return 0;

    task->callback = callback;
    scheduler->free_tasks = task->next;
    task->scheduler = scheduler;

    task->next = entry;
    if (entry == 0) {
        task->previous = 0;
        scheduler->active_tail = task;
        scheduler->active_head = task;
    } else {
        previous = entry->previous;
        task->previous = previous;
        if (previous == 0) {
            scheduler->active_head = task;
        } else {
            entry->previous->next = task;
        }
        entry->previous = task;
    }
    return task;
}

XglTaskPrefix *xglTaskEntryNext(XglTaskScheduler *scheduler,
                                int (*callback)(XglTaskPrefix *task),
                                XglTaskPrefix *entry)
{
    XglTaskPrefix *task;
    XglTaskPrefix *next;

    if (scheduler == 0 || (task = scheduler->free_tasks) == 0)
        return 0;

    task->callback = callback;
    scheduler->free_tasks = task->next;
    task->scheduler = scheduler;

    task->previous = entry;
    if (entry == 0) {
        task->next = 0;
        scheduler->active_tail = task;
        scheduler->active_head = task;
    } else {
        next = entry->next;
        task->next = next;
        if (next == 0) {
            scheduler->active_tail = task;
        } else {
            entry->next->previous = task;
        }
        entry->next = task;
    }
    return task;
}

int xglTaskRemove(XglTaskPrefix *task)
{
    XglTaskScheduler *scheduler = task->scheduler;
    XglTaskPrefix *free_task;

    if (task == scheduler->next_to_visit)
        scheduler->next_to_visit = task->next;

    if (task->next != 0)
        task->next->previous = task->previous;
    else
        scheduler->active_tail = task->previous;

    if (task->previous != 0)
        task->previous->next = task->next;
    else
        scheduler->active_head = task->next;

    free_task = scheduler->free_tasks;
    task->callback = 0;
    task->next = free_task;
    scheduler->free_tasks = task;
    return 0;
}

/*
 * Only replace the callback.  xglTaskExecute has already saved the successor
 * before invoking a callback, so self-marking normally takes effect when the
 * task is visited on the next execution pass.  A different task that has not
 * yet been visited can be removed later in the same pass.
 */
int xglTaskWaitRemove(XglTaskPrefix *task)
{
    task->callback = xglTaskRemove;
    return 0;
}

/*
 * next_to_visit is the cursor the scheduler advances during callbacks, rather
 * than the task whose callback is currently running.
 */
void xglTaskExecute(XglTaskScheduler *scheduler)
{
    XglTaskPrefix *task;

    if (scheduler != 0) {
        task = scheduler->active_head;
        while (task != 0) {
            scheduler->next_to_visit = task->next;
            if (task->callback != 0)
                task->callback(task);
            task = scheduler->next_to_visit;
        }
    }
}
