CC=gcc
CFLAGS=-c -g
TFLAG=-lrt

all: biplapsa_proj1
	@- echo Make successful.

biplapsa_proj1: rfss.o remote_command_processor.o file_utils.o biplapsa_proj1.o
	$(CC) ${TFLAG} rfss.o remote_command_processor.o file_utils.o biplapsa_proj1.o -o biplapsa_proj1

rfss.o: rfss.c
	$(CC) $(CFLAGS) rfss.c

remote_command_processor.o: remote_command_processor.c
	$(CC) $(CFLAGS) remote_command_processor.c

file_utils.o: file_utils.c
	$(CC) $(CFLAGS) file_utils.c

biplapsa_proj1.o: biplapsa_proj1.c
	$(CC) $(CFLAGS) biplapsa_proj1.c

clean:
	rm -rf *o biplapsa_proj1
	@- echo Data Cleansing Done.Ready to Compile
