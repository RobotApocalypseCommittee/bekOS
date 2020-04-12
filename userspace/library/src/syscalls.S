.globl write_str
write_str:
    svc #0
    ret

.globl delay_us
delay_us:
    svc #1
    ret

.globl get_pid
get_pid:
    svc #2
    ret