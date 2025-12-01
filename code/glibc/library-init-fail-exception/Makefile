build:
	$(CXX) -g -shared -o lib1.so lib1.cpp -fPIC
	$(CXX) -g -o main main.c -fPIC -L. -l1

clean:
	rm -f main lib1.so

.PHONY: build clean
