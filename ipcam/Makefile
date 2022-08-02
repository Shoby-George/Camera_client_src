CC = gcc
OUTPUT = camera_client
SRCDIR = src
SOURCES = $(shell find $(SRCDIR) -name '*.c')
OBJECTS = $(SOURCES:%.c=%.o)
CFLAGS = -std=c11 -DDEBOG=0 -O3 # -mcpu=cortex-a53 -mfpu=neon-fp-armv8
LDFLAGS = -lpthread -lyajl -ljpeg

.PHONY: all clean

all: $(OBJECTS) $(OUTPUT)

$(OBJECTS): %.o:%.c
	@echo compiling $@ from $<
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUTPUT): $(OBJECTS)
	@echo Linking for $(OUTPUT)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo Done

clean:
	@echo cleaning binaries and objects
	@rm $(OBJECTS)
	@rm $(OUTPUT)

