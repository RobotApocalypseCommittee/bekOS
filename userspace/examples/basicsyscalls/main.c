#include <syscalls.h>

int main() {
    char mstr[2] = {0, 0};
    mstr[0] = get_pid() + 48;
    while (1) {
        delay_us(1000000);
        write_str("hello world from ");
        write_str(&mstr);
        write_str("\n");
    }
}