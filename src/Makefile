CC = g++
CFLAGS = -Wall -std=c++17 -I../buddy_system

TARGET = Parcial2_Danna
SRCS = main.cpp conv_img_processor.cpp buddy_img_processor.cpp
OBJS = $(SRCS:.cpp=.o)

all: build-buddy $(TARGET)

build-buddy:
	$(MAKE) -C ../buddy_system

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) ../buddy_system/imagen.o ../buddy_system/stb_wrapper.o ../buddy_system/buddy_allocator.o

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)