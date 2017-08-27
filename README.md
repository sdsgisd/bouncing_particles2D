# bouncing_particles2D
This is a 2D implementation of bouncing particles in a box (in C++).

![Screen Shot](https://github.com/sdsgisd/bouncing_particles2D/blob/master/screenshot.png =60x60)
<!--60x60 is the size of image-->

[KEY SETTING]  
r/0: Reset.  
a: Add 20 particles.  
Arrow keys: Act force.

[MOUSE SETTING]  
Left click: Pull particles.  
Right click: Repel particles.

For making movie with libav
"avconv -start_number 2 -r 100 -i ./image%6d.png -r 60 -vf "scale=640:480,setsar=1" -b:v 120000k ../movie.mp4"


Dependencies: OpenGL, GLFW3, GLEW, libpng
Author: Sadashige Ishida  
License: MIT  

Currently attached Makefile and CMakeLists.txt are supported only for Mac.  
<!-- The library was tested on Mac OS X 10.11.6 -->
For Xcode users,   [Getting Started in OpenGL with GLFW/GLEW in Xcode 6(Youtube)][1]

[1]:https://www.youtube.com/watch?v=lTmM3Y8SMOM
