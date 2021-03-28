#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "devices/input.h"

struct file_descriptor
{
  int fd_num;
  tid_t owner;
  struct file *file_struct;
  struct list_elem elem;
};

/* List of open files */
struct list open_files; 

/* To ensure only one thread at a time is accessing the file system */
struct lock fs_lock;
static void syscall_handler (struct intr_frame *);

/* System call functions */
static void halt (void);
static void exit (int);
static pid_t exec (const char *);
static int wait (pid_t);
static bool create (const char*, unsigned);
static bool remove (const char *);
static int open (const char *);
static int filesize (int);
static int read (int, void *, unsigned);
static int write (int, const void *, unsigned);
static void seek (int, unsigned);
static unsigned tell (int);
static void close (int);
/* End of system call functions */

static struct file_descriptor *get_open_file (int);
static void close_open_file (int);
bool is_valid_ptr (const void *);
static int allocate_fd (void);
void close_file_by_owner (tid_t);

void syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init (&open_files);
  lock_init (&fs_lock);
}

static void syscall_handler (struct intr_frame *f)
{
  uint32_t *esp;
  esp = f->esp;

  if (!is_valid_ptr (esp) || !is_valid_ptr (esp + 1) ||
      !is_valid_ptr (esp + 2) || !is_valid_ptr (esp + 3))
    {
      exit (-1);
    }
  else
    {
      int syscall_number = *esp;
      switch (syscall_number)
        {
        case SYS_HALT:
          halt ();
          break;
        case SYS_EXIT:
          exit (*(esp + 1));
          break;
        case SYS_EXEC:
          f->eax = exec ((char *) *(esp + 1));
          break;
        case SYS_WAIT:
          f->eax = wait (*(esp + 1));
          break;
        case SYS_CREATE:
          f->eax = create ((char *) *(esp + 1), *(esp + 2));
          break;
        case SYS_REMOVE:
          f->eax = remove ((char *) *(esp + 1));
          break;
        case SYS_OPEN:
          f->eax = open ((char *) *(esp + 1));
          break;
        case SYS_FILESIZE:
	        f->eax = filesize (*(esp + 1));
	        break;
        case SYS_READ:
          f->eax = read (*(esp + 1), (void *) *(esp + 2), *(esp + 3));
          break;
        case SYS_WRITE:
          f->eax = write (*(esp + 1), (void *) *(esp + 2), *(esp + 3));
          break;
        case SYS_SEEK:
          seek (*(esp + 1), *(esp + 2));
          break;
        case SYS_TELL:
          f->eax = tell (*(esp + 1));
          break;
        case SYS_CLOSE:
          close (*(esp + 1));
          break;
        default:
          break;
        }
    }
}

/* Terminates Pintos by calling shutdown_power_off(). */
void halt (void)
{
  shutdown_power_off ();
}

/* Terminates the current user program, returning status to the kernel. 
A status of 0 indicates success and nonzero values indicate errors. */
void exit (int status)
{
  struct child_status *child;
  struct thread *cur = thread_current ();
  printf ("%s: exit(%d)\n", cur->name, status);
  struct thread *parent = thread_get_by_id (cur->parent_id);
  if (parent != NULL) 
    {
      struct list_elem *e = list_tail(&parent->children);
      while ((e = list_prev (e)) != list_head (&parent->children))
        {
          child = list_entry (e, struct child_status, elem_child_status);
          if (child->child_id == cur->tid)
          {
            lock_acquire (&parent->lock_child);
            child->is_exit_called = true;
            child->child_exit_status = status;
            lock_release (&parent->lock_child);
          }
        }
    }
  thread_exit ();
}
/* Runs the executable whose name is given in cmd_line, passing any given arguments,
 and returns the new process's program id (pid). */
pid_t exec (const char *cmd_line)
{
  lock_acquire (&fs_lock);
	char * fn_cp = malloc (strlen(cmd_line)+1);
	strlcpy(fn_cp, cmd_line, strlen(cmd_line)+1);
	  
	char * save_ptr;
	fn_cp = strtok_r(fn_cp," ",&save_ptr);

	struct file* f = filesys_open (fn_cp);

	if(f==NULL)
	{
	  lock_release (&fs_lock);
	  return -1;
	}
	else
	{
	  file_close(f);
	  lock_release (&fs_lock);
	  return process_execute(cmd_line);
	}
}
/* Waits for a child process pid and retrieves the child's exit status. */
int  wait (pid_t pid)
{ 
  return process_wait(pid);
}

/* Creates a new file called file initially initial_size bytes in size. 
Returns true if successful, false otherwise. */
bool create (const char *file_name, unsigned size)
{
  bool status;

  if (!is_valid_ptr (file_name))
    exit (-1);

  lock_acquire (&fs_lock);
  status = filesys_create(file_name, size);  
  lock_release (&fs_lock);
  return status;
}

/* Deletes the file called file. Returns true if successful, false otherwise. */
bool 
remove (const char *file_name)
{
  bool status;
  if (!is_valid_ptr (file_name))
    exit (-1);

  lock_acquire (&fs_lock);  
  status = filesys_remove (file_name);
  lock_release (&fs_lock);
  return status;
}

/* Opens the file called file. Returns a nonnegative integer handle called a "file descriptor" (fd), 
or -1 if the file could not be opened. */
int
open (const char *file_name)
{
  struct file *f;
  struct file_descriptor *fd;
  int status = -1;
  
  if (!is_valid_ptr (file_name))
    exit (-1);

  lock_acquire (&fs_lock); 
 
  f = filesys_open (file_name);
  if (f != NULL)
  {
    fd = calloc (1, sizeof *fd);
    fd->fd_num = allocate_fd ();
    fd->owner = thread_current ()->tid;
    fd->file_struct = f;
    list_push_back (&open_files, &fd->elem);
    status = fd->fd_num;
  }
  lock_release (&fs_lock);
  return status;
}

/* Returns the size, in bytes, of the file open as fd. */
int filesize (int fd)
{
  struct file_descriptor *fd_struct;
  int status = -1;
  lock_acquire (&fs_lock); 
  fd_struct = get_open_file (fd);
  if (fd_struct != NULL)
    status = file_length (fd_struct->file_struct);
  lock_release (&fs_lock);
  return status;
}

/* Reads size bytes from the file open as fd into buffer. 
Returns the number of bytes actually read (0 at end of file), or -1 if the file could not be read. */
int
read (int fd, void *buffer, unsigned size)
{
  struct file_descriptor *fd_struct;
  int status = 0; 

  if (!is_valid_ptr (buffer) || !is_valid_ptr (buffer + size - 1))
    exit (-1);

  lock_acquire (&fs_lock); 
  
  if (fd == STDOUT_FILENO)
    {
      lock_release (&fs_lock);
      return -1;
    }

  if (fd == STDIN_FILENO)
  {
    uint8_t c;
    unsigned counter = size;
    uint8_t *buf = buffer;
    while (counter > 1 && (c = input_getc()) != 0)
    {
      *buf = c;
      buffer++;
      counter--; 
    }
    *buf = 0;
    lock_release (&fs_lock);
    return (size - counter);
  } 
  
  fd_struct = get_open_file (fd);
  if (fd_struct != NULL)
    status = file_read (fd_struct->file_struct, buffer, size);

  lock_release (&fs_lock);
  return status;
}

/* Writes size bytes from buffer to the open file fd. 
Returns the number of bytes actually written, which may be less than size if some bytes could not be written. */
int write (int fd, const void *buffer, unsigned size)
{
  struct file_descriptor *fd_struct;  
  int status = 0;

  if (!is_valid_ptr (buffer) || !is_valid_ptr (buffer + size - 1))
    exit (-1);
  lock_acquire (&fs_lock); 

  if (fd == STDIN_FILENO)
    {
      lock_release(&fs_lock);
      return -1;
    }
  if (fd == STDOUT_FILENO)
    {
      putbuf (buffer, size);
      lock_release(&fs_lock);
      return size;
    }
  fd_struct = get_open_file (fd);
  if (fd_struct != NULL)
    status = file_write (fd_struct->file_struct, buffer, size);
  lock_release (&fs_lock);
  return status;
}

/* Changes the next byte to be read or written in open file fd to position, 
expressed in bytes from the beginning of the file. */
void seek (int fd, unsigned position)
{
  struct file_descriptor *fd_struct;
  lock_acquire (&fs_lock); 
  fd_struct = get_open_file (fd);
  if (fd_struct != NULL)
    file_seek (fd_struct->file_struct, position);
  lock_release (&fs_lock);
  return;
}

/* Returns the position of the next byte to be read or written in open file fd, 
expressed in bytes from the beginning of the file. */
unsigned tell (int fd)
{
  struct file_descriptor *fd_struct;
  int status = 0;
  lock_acquire (&fs_lock); 
  fd_struct = get_open_file (fd);
  if (fd_struct != NULL)
    status = file_tell (fd_struct->file_struct);
  lock_release (&fs_lock);
  return status;
}

/* Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open file descriptors, 
as if by calling this function for each one. */
void close (int fd)
{
  struct file_descriptor *fd_struct;
  lock_acquire (&fs_lock); 
  fd_struct = get_open_file (fd);
  if (fd_struct != NULL && fd_struct->owner == thread_current ()->tid)
    close_open_file (fd);
  lock_release (&fs_lock);
  return ; 
}

/* Helper method to get open file */
struct file_descriptor * get_open_file (int fd)
{
  struct list_elem *e;
  struct file_descriptor *fd_struct; 
  e = list_tail (&open_files);
  while ((e = list_prev (e)) != list_head (&open_files)) 
  {
    fd_struct = list_entry (e, struct file_descriptor, elem);
    if (fd_struct->fd_num == fd)
	  return fd_struct;
  }
  return NULL;
}

/* Helper method to close the file */
void close_open_file (int fd)
{
  struct list_elem *e;
  struct list_elem *prev;
  struct file_descriptor *fd_struct; 
  e = list_end (&open_files);
  while (e != list_head (&open_files)) 
  {
    prev = list_prev (e);
    fd_struct = list_entry (e, struct file_descriptor, elem);
    if (fd_struct->fd_num == fd)
	  {
	    list_remove (e);
      file_close (fd_struct->file_struct);
	    free (fd_struct);
	    return ;
	  }
    e = prev;
  }
  return ;
}


/* Checks if is valid pointer */
bool is_valid_ptr (const void *usr_ptr)
{
  struct thread *cur = thread_current ();
  if (usr_ptr != NULL && is_user_vaddr (usr_ptr))
    {
      return (pagedir_get_page (cur->pagedir, usr_ptr)) != NULL;
    }
  return false;
}

int allocate_fd ()
{
  static int fd_current = 1;
  return ++fd_current;
}

/* Helper method to close the file */
void
close_file_by_owner (tid_t tid)
{
  struct list_elem *e;
  struct list_elem *next;
  struct file_descriptor *fd_struct; 
  e = list_begin (&open_files);
  while (e != list_tail (&open_files)) 
  {
    next = list_next (e);
    fd_struct = list_entry (e, struct file_descriptor, elem);
    if (fd_struct->owner == tid)
	  {
	    list_remove (e);
	    file_close (fd_struct->file_struct);
      free (fd_struct);
	  }
    e = next;
  }
}