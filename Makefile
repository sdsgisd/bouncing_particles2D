# Makefile

# build: #most simple case.
# 	g++ -std=c++14 -framework OpenGL -I/usr/local/include -L/usr/local/lib -lGLEW  -lglfw.3.1 bouncing_particles.cpp -o bouncing_particles

COMPILER  = g++
CFLAGS    = -std=c++14
LDFLAGS = -framework OpenGL -L/usr/local/lib -lGLEW  -lglfw.3.1 -lpng

INCLUDE   = -I/usr/local/include/ -I/usr/local/include/libpng16/
BUILDDIR	= ./build
TARGET    = bouncing_particles
SRCDIR    = ./bouncing_particles2D
ifeq "$(strip $(SRCDIR))" ""
  SRCDIR  = .
endif
SOURCES   = $(wildcard $(SRCDIR)/*.cpp)
OBJDIR    = ./
ifeq "$(strip $(OBJDIR))" ""
  OBJDIR  = .
endif
OBJECTS   = $(addprefix $(OBJDIR)/, $(notdir $(SOURCES:.cpp=.o)))
DEPENDS   = $(OBJECTS:.o=.d)

$(TARGET): $(OBJECTS)
	-mkdir -p $(BUILDDIR)
	$(COMPILER) -o $(BUILDDIR)/$@ $^ $(LDFLAGS)
	rm -rf $(OBJECTS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	-mkdir -p $(OBJDIR)
	$(COMPILER) $(CFLAGS) $(INCLUDE) -o $@ -c $<

all: clean $(TARGET)

clean:
	-rm -f $(OBJECTS) $(DEPENDS) $(TARGET)

-include $(DEPENDS)
