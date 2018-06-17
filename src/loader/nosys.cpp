#include <errno.h>
#include <sys/stat.h>
#undef errno
#include <cstring>
#include <cstdio>
    
extern "C" {
    extern int errno;

    void _exit(int status) {
        /**/
    }

    int close(int file) {
        return -1;
    }

    char *__env[1] = { 0 };
    char **environ = __env;

    int execve(char *name, char **argv, char **env) {
        errno = ENOMEM;
        return -1;
    }

    int fork(void) {
        errno = EAGAIN;
        return -1;
    }

    int fstat(int file, struct stat *st) {
        st->st_mode = S_IFCHR;
        return 0;
    }

    int getpid(void) {
        return 1;
    }

    int isatty(int file) {
        return 1;
    }

    int kill(int pid, int sig) {
        errno = EINVAL;
        return -1;
    }

    int link(char *old, const char *newp) {
        errno = EMLINK;
        return -1;
    }

    int lseek(int file, int ptr, int dir) {
        return 0;
    }

    int open(const char *name, int flags, int mode) {
        return -1;
    }

    int read(int file, char *ptr, int len) {
        return 0;
    }
    
    extern char end;
    extern char stack_bottom;

    static inline void outb(uint16_t port, uint8_t value) {
        __asm__ __volatile__ ("out %0, %1" :: "a"(value), "Nd"(port));
    }

    int write(int file, const char *ptr, int len) {
        int todo;

        for (todo = 0; todo < len; todo++) {
            //outb(0x3f8, *ptr++);
            outb(0xe9, *ptr++);
        }
        return len;
    }

    void *sbrk(int incr) {
        static char *heap_end;
        char *prev_heap_end;
         
        if (heap_end == 0) {
            heap_end = &end;
        }
        prev_heap_end = heap_end;
        if (heap_end + incr > &stack_bottom) {
            write (1, "Heap and stack collision\n", 25);
            asm volatile ("cli; hlt");
        }

        heap_end += incr;
        return prev_heap_end;
    }

    int stat(const char *file, struct stat *st) {
        st->st_mode = S_IFCHR;
        return 0;
    }

    int times(struct tms *buf) {
        return -1;
    }

    int unlink(const char *name) {
        errno = ENOENT;
        return -1; 
    }

    int wait(int *status) {
        errno = ECHILD;
        return -1;
    }
}