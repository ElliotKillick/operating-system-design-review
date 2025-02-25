build:
	$(CC) -g -shared -o lib1.so lib1.c -fPIC
	# Intentionally give lib2 a circular dependency because lib1 also dynamically loads lib2
	$(CC) -g -shared -o lib2.so lib2.c -fPIC -L. -l1
	$(CC) -g -o main main.c -fPIC -L. -l1

clean:
	rm -f main lib1.so lib2.so

.PHONY: build clean
