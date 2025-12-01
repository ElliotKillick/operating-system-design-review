build:
	$(CC) -g -shared -o lib1.so lib1.c -fPIC
	$(CC) -g -o main main.c -fPIC -ldl

clean:
	rm -f main lib1.so

.PHONY: build clean
