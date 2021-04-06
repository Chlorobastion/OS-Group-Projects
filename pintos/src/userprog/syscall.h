#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/thread.h"
#include "userprog/process.h"
typedef int pid_t;

struct lock fs_lock;
void syscall_init (void);
void close_file_by_owner (int);
#endif /* userprog/syscall.h */
