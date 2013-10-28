CPPFLAGS = -std=c++11 -Iinclude
LINKFLAGS = -lGL -lGLEW

all: bin/basic_app bin/gfx_app

bin/basic_app: include/cu/* src/copper/* src/basic_app/*
	g++ $(CPPFLAGS) src/copper/*.cpp src/basic_app/*.cpp -o bin/basic_app $(LINKFLAGS)

bin/gfx_app: include/cu/* src/copper/* src/gfx_app/*
	g++ $(CPPFLAGS) src/copper/*.cpp src/gfx_app/*.cpp -o bin/gfx_app $(LINKFLAGS) -lSDL2

clean:
	rm bin/*
