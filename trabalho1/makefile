nomePrograma=client

objs=client.o lib.o

CFLAGS := -Wall -lncurses

all: $(nomePrograma)

$(nomePrograma): $(objs)
	gcc -o $(nomePrograma) $(objs) $(CFLAGS)

client.o: client.c
	gcc -c client.c $(CFLAGS)

lib.o: lib.h lib.c
	gcc -c lib.c $(CFLAGS)

clean:
	rm -f $(objs) *~

purge: clean
	-rm -f $(nomePrograma)


