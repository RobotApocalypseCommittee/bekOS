#ifndef BEKOS_LIBC_SYSCALLS_H
#define BEKOS_LIBC_SYSCALLS_H

void write_str(char* string);
void delay_us(unsigned long microseconds);
int get_pid();
int open(char* path);
unsigned long read(unsigned long index, char* buffer, unsigned long length);
int close(unsigned long index);


#endif //BEKOS_LIBC_SYSCALLS_H
