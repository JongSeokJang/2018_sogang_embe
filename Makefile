20121635_1 : 20121635_1.o
	arm-none-linux-gnueabi-gcc -static -o 20121635_1 20121635_1.o
20121635_1.o : 20121635_1.c
	arm-none-linux-gnueabi-gcc -static -c -o 20121635_1.o 20121635_1.c
clean :
	rm 20121635_1.o 20121635_1
