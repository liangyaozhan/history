start qemu-system-arm.exe -M mini2440 -s -S -show-cursor -serial telnet:127.0.0.1:1200,server -sd SDCARD.vfd -serial file:virtualkbd
start putty.exe telnet://127.0.0.1:1200/

arm-none-eabi-gdb --command=gdbcmd.txt

