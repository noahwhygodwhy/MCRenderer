# MCRenderer
A minecraft renderer

So it's come to my attention that people are actually seeing this, so uh...

This is a minecraft renderer written in C++ 17 using openGL 4.6. It is currently set up to parse 1.15.2 worlds. 
It is far from complete. I'm doing multiple things wrong and ending up with many missing blocks.
Additionally, the code is set up to access file paths on my laptop, so that will have to be changed for it to run.

I'm compiling in visual studio with nuget packages glfw v3.3.2 and glm 0.9.9.700. Has to be compiled with command line argument

/D _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS" 

as stb_image libary uses some old stuff.

It also uses a couple libaries.
These libraries include stb_image loader for loading textures at http://nothings.org/stb. This is public domain so that should be ok.
I'm also using glad which was generated with https://glad.dav1d.de/#profile=core&language=c&specification=gl&loader=on&api=gl%3D4.6.
Biomes.h contains a table of data for biome coloring, which isn't used yet, but was made by Eric Haines. (see full copyright in the file)
