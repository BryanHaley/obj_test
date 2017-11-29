#ifndef OBJTEST_H
#define OBJTEST_H

#include "OpenGL/gl.h"

/* Magic Numbers */

// Default window dimensions
#define DEF_WIN_WIDTH 640
#define DEF_WIN_HEIGHT 480

// Return codes
#define NOERR  0
#define ERR   -1
// fancier return codes go here

// For boolean values
#define bool int
#define TRUE 1
#define FALSE 0

// String comparison
#define STR_EQUAL 0

/* Structure Declarations */

struct vector2f;
struct vector3f;
struct model;
struct scene;
struct triangle;

// To use _t or to not use _t?
typedef struct vector2f Vector2f;
typedef struct vector3f Vector3f;
typedef struct model Model;
typedef struct scene Scene;
typedef struct triangle Triangle;
//typedef struct texture Texture;
typedef GLuint Texture; // cheat for demonstration purposes

/* 
 * Global variables 
 * Nasty and should be avoided.
 */

bool window_size_changed;
int window_width, window_height;
float camera_xRot, camera_yRot;

/* Functions */

// Defined in: objtest.c
extern Scene* init_scene(Scene* scene, 
						 char* model_filename, char* texture_filename, float scale);
extern void render_scene(Scene* scene);
extern void key_callback (GLFWwindow* window, int key, int scancode, int action, int mods);
extern void window_size_callback(GLFWwindow* window, int w, int h);

// Defined in: file_loaders.c
// load_tex reads a TGA file and returns a Texture (OpenGL texture ID)
extern Texture* load_tex(char* filename);
// load_obj reads an OBJ file and returns a Model object
extern Model* load_obj(char* filename);
// assign_tex pairs a Model with a Texture
extern int assign_tex(Model* model, Texture* tex);

/* 
 * Structure Definitions
 * Could separate these out into a .c but it doesn't seem necessary at this point.
 */

// Two dimensional point/vector
struct vector2f
{
    float x;
    float y;
};

// Three dimensional point/vector
struct vector3f
{
    float x;
    float y;
    float z;
};

// Triangle consisting of vertices, UV coordinates, and normal vectors
struct triangle
{
	// 3 points (vertices) = 1 triangle
    Vector3f* v1;
    Vector3f* v2;
    Vector3f* v3;

    // Each vertice has a UV coordinate for texture mapping
    Vector2f* u1;
    Vector2f* u2;
    Vector2f* u3;

    // Each vertice has a normal for shading
    Vector3f* n1;
    Vector3f* n2;
    Vector3f* n3;
};

// A texture holds its width and height, bit depth, and an array of pixels
/*
	struct texture
	{
		int width, height, depth;
		unsigned char* pixels;
	}
*/
// In a real program we'd use this as an interface but for here we'll just use an OpenGL
// texture ID.

// Models consist of the number of triangles, an array of triangle pointers, and a texture.
struct model
{
	int tri_count;
	bool textured;

	Triangle** triangles;
	Texture* texture;
};

// Scenes consist of a camera and some models for this demo
struct scene
{
	float view_area_scale; // Should be a struct
    int model_count;
    Model** models;
};

#endif