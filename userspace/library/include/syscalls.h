#ifndef BEKOS_LIBC_SYSCALLS_H
#define BEKOS_LIBC_SYSCALLS_H

void write_str(char* string);
void delay_us(unsigned long microseconds);
int get_pid();
int open(char* path);
int read(int index, char* buffer, int length);
int close(int index);


#endif //BEKOS_LIBC_SYSCALLS_H
