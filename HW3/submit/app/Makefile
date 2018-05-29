App : app.o
	arm-none-linux-gnueabi-gcc -static -o App app.o
app.o : app.c
	arm-none-linux-gnueabi-gcc -static -c -o app.o app.c
clean :
	rm app.o App
