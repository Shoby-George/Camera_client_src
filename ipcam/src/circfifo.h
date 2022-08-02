#ifndef CIRCFIFO_H
#define CIRCFIFO_H

struct cirfifo
{
    unsigned long fifo_depth; // fifo depth (constant)
    unsigned long reserved0; // to pad
    unsigned char *fifo;    // address of the fifo (constant)
    unsigned long reserved1; // to pad
    unsigned long readptr;
    unsigned long writeptr;
    unsigned long filled_length;
};

void fifo_init(struct cirfifo *fifo, unsigned long depth, unsigned char *data);
void fifo_flush(struct cirfifo *fifo);
unsigned long fifo_rewind(struct cirfifo *fifo, unsigned long length);
unsigned long fifo_write(struct cirfifo *fifo, unsigned char *data, unsigned long len);
unsigned long fifo_read(struct cirfifo *fifo, unsigned char * data, unsigned long len);
unsigned long fifo_peek(struct cirfifo *fifo, unsigned char *data, unsigned long len);
#endif

