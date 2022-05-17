.PHONY: all

all: enc_server enc_client dec_server dec_client keygen

enc_server: enc_server.c
	gcc -std=gnu99 -o enc_server enc_server.c

enc_client: enc_client.c
	gcc -std=gnu99 -o enc_client enc_client.c

dec_server: dec_server.c
	gcc -std=gnu99 -o dec_server dec_server.c

dec_client: dec_client.c
	gcc -std=gnu99 -o dec_client dec_client.c

keygen: keygen.c
	gcc -std=gnu99 -o keygen keygen.c
	
clean:
	rm -rf enc_server
	rm -rf enc_client
	rm -rf dec_server
	rm -rf dec_client
	rm -rf keygen