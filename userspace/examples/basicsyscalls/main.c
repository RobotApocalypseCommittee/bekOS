#include <syscalls.h>


int main() {
    char mstr[2] = {0, 0};
    mstr[0] = get_pid() + 48;
    while (1) {
        delay_us(2000000);

        auto x = open("greet/hi.txt");

        if (x >= 0) {
            write_str("successfully opened\n");
        }

        char text[100] = {0,};
        read(x, text, 100);

        write_str(text);
        write_str("\n");

        close(x);



    }
}
