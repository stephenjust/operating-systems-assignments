all: f_server p_server
f_server:
	gcc f_server.c server_common.c -g -Wall -O0 -o ./server_f
p_server:
	gcc p_server.c server_common.c -g -pthread -Wall -O0 -o ./server_p
clean:
	rm -f ./server_f ./server_p
