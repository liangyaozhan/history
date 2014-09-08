all : lcd.bin
lcd.bin : start.S  main.c nand_cp.c sunflower.c
	arm-none-eabi-gcc -g -c -o start.o start.S
	arm-none-eabi-gcc -g -c -o main.o main.c
	arm-none-eabi-gcc -g -c -o nand_cp.o nand_cp.c
	arm-none-eabi-gcc -g -c -o sunflower.o sunflower.c
	arm-none-eabi-ld -Tlcd.lds -Ttext 0x33F80000 start.o main.o nand_cp.o sunflower.o -o lcd_elf
	arm-none-eabi-objcopy -O binary -S lcd_elf lcd.bin
	arm-none-eabi-objdump -D -m arm  lcd_elf > lcd.dis
clean:
	rm -f   lcd.dis lcd.bin lcd_elf *.o
