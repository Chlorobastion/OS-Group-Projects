#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"
#include "userprog/process.h"

/* Lock used to be sure that only one thread is modifying the file
system at a time. */
struct lock fs_lock;

void syscall_init (void);
bool is_valid_ptr(const void *);

#endif /* userprog/syscall.h */
