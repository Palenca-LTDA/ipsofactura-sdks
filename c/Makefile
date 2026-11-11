CC     = cc
CFLAGS = -Wall -Wextra -O2

# OpenSSL — detect Homebrew path on macOS
OPENSSL_PREFIX := $(shell brew --prefix openssl 2>/dev/null || echo /usr)
CFLAGS  += -I$(OPENSSL_PREFIX)/include
LDFLAGS  = -L$(OPENSSL_PREFIX)/lib -lcurl -lssl -lcrypto

example: example.c ipsofactura.o cJSON.o
	$(CC) $(CFLAGS) -o example example.c ipsofactura.o cJSON.o $(LDFLAGS)

ipsofactura.o: ipsofactura.c ipsofactura.h cJSON.h
	$(CC) $(CFLAGS) -c ipsofactura.c

cJSON.o: cJSON.c cJSON.h
	$(CC) $(CFLAGS) -c cJSON.c

clean:
	rm -f example *.o

.PHONY: clean
