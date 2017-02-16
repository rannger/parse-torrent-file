echo "
CC = clang
LD = Ld 
CFLAGS = -g -c -Wall  
LDFLAG = -g  
TARGET = torrent
OBJS = `echo $(ls *.c|sed 's/.c/.o/g')`
all: \$(TARGET)

\$(OBJS): %.o: %.c
	\$(CC) -c \$(CFLAGS) \$< -o \$@

\$(TARGET):\$(OBJS)
	\$(CC) \$(LDFLAG) -o \$(TARGET) \$(OBJS)

.PHONY: clean

clean:
	rm -f \$(OBJS) \$(TARGET)
" > makefile

make
