build:
	$(CC) -shared -o lib1.so lib1.c -fPIC -lpthread
	$(CC) -o main main.c -fPIC -ldl -lpthread

clean:
	rm -f main lib1.so

.PHONY: build clean
