#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"

/* System call functions */
static void halt (void);
static void exit (int);
static pid_t exec (const char *);
static int wait (pid_t);
static bool create (const char*, unsigned);
static bool remove (const char *);
static int open (const char *);
static int filesize (int);
//static int read (int, void *, unsigned);
static int write (int, const void *, unsigned);
static void seek (int, unsigned);
static unsigned tell (int);
static void close (int);
/* End of system call functions */

/* Helper functions*/
static struct file_descriptor *get_open_file (int);

static void syscall_handler (struct intr_frame *);

/* List of files opened by user process */
struct list open_files;

struct file_descriptor
{
  int fd_num;
  tid_t owner;
  struct file *file_struct;
  struct list_elem elem;
}

static uint32_t *esp;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init(&open_files);
  lock_init(&fs_lock);
}

static void
syscall_handler (struct intr_frame *f)
{
  esp = f->esp;

  if (!is_valid_ptr (esp) || !is_valid_ptr (esp + 1) || !is_valid_ptr (esp + 2) || !is_valid_ptr (esp + 3))
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
      //case SYS_READ:
      //  f->eax = read (*(esp + 1), (void *) *(esp + 2), *(esp + 3));
      //  break;
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

void halt(void)
{
  shutdown_power_off();
}

void exit(int status)
{
  struct child_status *child;
  struct thread *cur = thread_current();
  printf("%s: exit(%d)\n", cur->name, status);
  struct thread *parent = thread_get_by_id(cur->parent_id); // Get the parent of this thread if it exists
  if (parent != NULL)
  {
    struct list_elem *e = list_tail(&parent->children);
    while ((e = list_prev (e)) != list_head (&parent->children))
    {
      child = list_entry (e, struct child_status, elem_child_status);
      if (child->child_id == cur->tid) // Find our child on the parent's child list
      {
        lock_acquire (&parent->lock_child);
        child->is_exit_called = true; // Notify parent the child is exiting
        child->child_exit_status = status;
        lock_release (&parent->lock_child);
      }
    }
  }
  thread_exit(); // End this thread
}

pid_t exec(const char *cmd_line)
{
  /* a thread's id. When there is a user process within a kernel thread, we
   * use one-to-one mapping from tid to pid, which means pid = tid
   */
  tid_t tid;
  struct thread *cur;

  /* check if the user pointer is valid */
  if (!is_valid_ptr (cmd_line))
    {
      exit (-1); // If pointer is NOT valid, exit and make sure it's status reflects that it is invalid
    }

  cur = thread_current ();
  cur->child_load_status = 0;
  tid = process_execute (cmd_line);
  lock_acquire(&cur->lock_child);
  while (cur->child_load_status == 0) // Child has not been loaded yet
    cond_wait(&cur->cond_child, &cur->lock_child); // Used to wait the child
  if (cur->child_load_status == -1)
    tid = -1;
  lock_release(&cur->lock_child);
  return tid;
}

int wait(pid_t pid)
{
  return process_wait(pid);
}

bool create(const char *file, unsigned initial_size)
{
  bool status;
  if (!is_valid_ptr (file))
    exit (-1);

  lock_acquire (&fs_lock);
  status = filesys_create(file, initial_size); // Use file creation given by Pintos
  lock_release (&fs_lock);
  return status;
}

bool remove(const char *file)
{
  bool status;
  if (!is_valid_ptr (file))
    exit (-1);

  lock_acquire (&fs_lock);  
  status = filesys_remove (file); // Use file deletion given by Pintos
  lock_release (&fs_lock);
  return status;
}

int open(const char *file)
{
  struct file *f;
  struct file_descriptor *fd;
  int status = -1;
  
  if (!is_valid_ptr (file))
    exit (-1);

  lock_acquire (&fs_lock); 
 
  f = filesys_open (file); // Use file opening given by Pintos
  if (f != NULL)
    {
      fd = calloc (1, sizeof *fd);
      static int fd_current = 1; // Do we need to make sure this is not set multiple times? Or no?
      fd->fd_num = ++fd_current;
      fd->owner = thread_current ()->tid;
      fd->file_struct = f;
      list_push_back (&open_files, &fd->elem);
      status = fd->fd_num;
    }
  lock_release (&fs_lock);
  return status;
}

int filesize(int fd)
{
  struct file_descriptor *fd_struct;
  int status = -1;
  lock_acquire (&fs_lock); 
  fd_struct = get_open_file (fd);
  struct list_elem *e;
  struct file_descriptor *fd_search;
  e = list_tail (&open_files);
  fd_struct = NULL; // In case we do not find the file descriptor
  while ((e = list_prev (e)) != list_head (&open_files)) 
  {
    fd_search = list_entry (e, struct file_descriptor, elem);
    if (fd_search->fd_num == fd)
	    fd_struct = fd_search; // We found the file descriptor!
  }
  if (fd_struct != NULL)
    status = file_length (fd_struct->file_struct);
  lock_release (&fs_lock);
  return status;
}

/*
// COME BACK TO READ TO UNDERSTAND AFTER FIXING HASH TABLE STUFF
int read(int fd, void *buffer, unsigned size)
{
  struct file_descriptor *fd_struct;
  int status = 0;
  struct thread *t = thread_current ();

  unsigned buffer_size = size;
  void * buffer_tmp = buffer;

  // check the user memory pointing by buffer are valid
  while (buffer_tmp != NULL)
  {
    if (buffer_tmp != NULL && is_user_vaddr (buffer_tmp))
	      exit (-1); // Invalid user virtual adress
    if (pagedir_get_page (t->pagedir, buffer_tmp) == NULL)   
	  {
	    struct suppl_pte *spte;
	    spte = get_suppl_pte (&t->suppl_page_table, pg_round_down (buffer_tmp)); // HASH TABLE STUFF??? FIX ME
	    if (spte != NULL && !spte->is_loaded)
	      load_page (spte);
      else if (spte == NULL && buffer_tmp >= (esp - 32))
	      grow_stack (buffer_tmp);
	    else
	      exit (-1);
	  }
      
    // Advance
    if (buffer_size == 0)
	  {
	    // terminate the checking loop 
	    buffer_tmp = NULL;
	  }
    else if (buffer_size > PGSIZE)
	  {
	    buffer_tmp += PGSIZE;
	    buffer_size -= PGSIZE;
	  }
    else
	  {
	    // last loop
	    buffer_tmp = buffer + size - 1;
	    buffer_size = 0;
	  }
  }

  lock_acquire (&fs_lock);   
  if (fd == STDOUT_FILENO)
    status = -1;
  else if (fd == STDIN_FILENO)
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
    status = size - counter;
  }
  else 
  {
    fd_struct = get_open_file (fd);
    if (fd_struct != NULL)
	  status = file_read (fd_struct->file_struct, buffer, size);
  }
  lock_release (&fs_lock);
  return status;
}
*/

int write(int fd, const void *buffer, unsigned size)
{
  struct file_descriptor *fd_struct;  
  int status = 0;

  unsigned buffer_size = size;
  void *buffer_tmp = buffer;

  /* check the user memory pointing by buffer are valid */
  while (buffer_tmp != NULL)
  {
    if (!is_valid_ptr (buffer_tmp))
	    exit (-1);
      
    /* Advance */ 
    if (buffer_size > PGSIZE)
	  {
	    buffer_tmp += PGSIZE;
	    buffer_size -= PGSIZE;
	  }
    else if (buffer_size == 0)
	  {
	    /* terminate the checking loop */
	    buffer_tmp = NULL;
	  }
    else
	  {
	    /* last loop */
	    buffer_tmp = buffer + size - 1;
	    buffer_size = 0;
	  }
  }

  lock_acquire (&fs_lock); 
  if (fd == STDIN_FILENO)
  {
    status = -1;
  }
  else if (fd == STDOUT_FILENO)
  {
    putbuf (buffer, size);;
    status = size;
  }
  else 
  {
    fd_struct = get_open_file (fd);
    if (fd_struct != NULL)
    	status = file_write (fd_struct->file_struct, buffer, size); //  Use file writing given by Pintos
  }
  lock_release (&fs_lock);
  return status;
}

void seek(int fd, unsigned position)
{
  struct file_descriptor *fd_struct;
  lock_acquire (&fs_lock); 
  fd_struct = get_open_file (fd);
  if (fd_struct != NULL)
    file_seek (fd_struct->file_struct, position); // Use file seeking given by Pintos
  lock_release (&fs_lock);
  return;
}

unsigned tell(int fd)
{
  struct file_descriptor *fd_struct;
  int status = 0;
  lock_acquire (&fs_lock); 
  fd_struct = get_open_file (fd);
  if (fd_struct != NULL)
    status = file_tell (fd_struct->file_struct); // Use file telling given by Pintos
  lock_release (&fs_lock);
  return status;
}

void close(int fd)
{
  struct file_descriptor *fd_struct;
  lock_acquire (&fs_lock); 
  fd_struct = get_open_file (fd);
  if (fd_struct != NULL && fd_struct->owner == thread_current ()->tid)
    close_open_file (fd); // Use file closing given by Pintos
  lock_release (&fs_lock);
  return;
}



/* The kernel must be very careful about doing so, because the user can
 * pass a null pointer, a pointer to unmapped virtual memory, or a pointer
 * to kernel virtual address space (above PHYS_BASE). All of these types of
 * invalid pointers must be rejected without harm to the kernel or other
 * running processes, by terminating the offending process and freeing
 * its resources. (This is the helper function we decided to keep, because
 * it is used in several places in syscall.c).
 */
bool
is_valid_ptr (const void *usr_ptr)
{
  struct thread *cur = thread_current ();
  if (usr_ptr != NULL && is_user_vaddr(usr_ptr))
    {
      return (pagedir_get_page (cur->pagedir, usr_ptr)) != NULL;
    }
  return false;
}

struct file_descriptor *
get_open_file (int fd)
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