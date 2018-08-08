.PHONY:clean all

object=tetris_client tetris_server 

all: $(object) 

tetris_client: tetris_client.c
	gcc -g -o $@ $^ -L ./keyboard/ -lkeyboard -lpthread
tetris_server: tetris_server.c
	gcc -g -o $@ $^ -L ./keyboard/ -lkeyboard -lpthread

clean:
	rm -f $(object)
