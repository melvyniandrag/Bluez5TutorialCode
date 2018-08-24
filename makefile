INC_DIRS := -I/usr/include/glib-2.0 -I/usr/include/gio-unix-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
COMP_STD := -std=c++11
LIBS := -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lpthread

main: Listener.o Profile.o socket.o main.o
	g++ $? -o $@ $(INC_DIRS) $(LIBS) $(COMP_STD)

socket.o: socket.cpp
	g++ -c $< -o $@ $(INC_DIRS) $(LIBS) $(COMP_STD)

Listener.o: Listener.cpp
	g++ -c $< -o $@ $(INC_DIRS) $(LIBS) $(COMP_STD)

Profile.o: Profile.c
	gcc -c $< -o $@ $(INC_DIRS) $(LIBS)

main.o: main.cpp
	g++ -c $< -o $@ $(INC_DIRS) $(LIBS) $(COMP_STD)

clean:
	rm *.o main
