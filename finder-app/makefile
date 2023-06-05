all: writer

writer: writer.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS) writer.c -o writer

clean:
	rm -f writer
	rm -f *.o
