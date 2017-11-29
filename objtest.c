#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>

#include "objtest.h"

/* 
 * PROGRAM: objtest
 * PURPOSE: Load an arbitrary OBJ model file and render it in a scene
 * AUTHORS: Bryan Haley
 */

// main is response for creating the window and OpenGL context, calling init_scene, then
// calling render_scene in a loop
int main(int argc, char** argv)
{
    /* Variables */

    // OpenGL/GLFW variables
    GLFWwindow* window = NULL;

    // 3D Scene
    Scene* scene = NULL;

    // Set some defaults
    char* obj_file = "monkey.obj"; // Suzanne is a better default as we use vertex lighting
    char* tex_file = "tex.tga";
    float scale = 2.5f;

    // Set the model files and scale to the command line input if we received any
    if (argc > 1)
    { obj_file = argv[1]; }

    if (argc > 2)
    { tex_file = argv[2]; }

    if (argc > 3)
    { scale = atof(argv[3]); }

    /* Window and OpenGL context creation */

    // Initialize the GLFW + error checking
    if (!glfwInit()) { fprintf(stderr, "Could not init window system.\n"); return ERR; }
    
    // Set OpenGL version to at least 2.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    // Use the compatibility profile for the simplicity of the fixed pipeline
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    
    // Initialize window
    window = glfwCreateWindow (DEF_WIN_WIDTH, DEF_WIN_HEIGHT, "OBJ Import", NULL, NULL);
    
    // error check window
    if (!window) 
    { fprintf(stderr, "Could not init window.\n"); glfwTerminate(); return ERR; }

    // Create OpenGL context
    glfwMakeContextCurrent(window);

    // Set window callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
    window_size_callback(window, DEF_WIN_WIDTH, DEF_WIN_HEIGHT);

    /* Scene creation */

    // Initialize the 3D scene
    scene = init_scene(scene, obj_file, tex_file, scale);

    // Error check
    if (!scene) 
    { fprintf(stderr, "Could not init 3D scene.\n"); glfwTerminate(); return ERR; }
    
    /* Render scene loop */

    while (!glfwWindowShouldClose(window))
    {
        // Get input
        glfwPollEvents();
        
        // Render the scene
        render_scene(scene);
        
        // Swap the buffers
        glfwSwapBuffers(window);

        // TODO: delay with a proper timestep
    }

    return NOERR;
}

Scene* init_scene(Scene* scene, char* model_filename, char* texture_filename, float scale)
{
    /* Variables */

    Model* model = NULL;
    Texture* texture = NULL;
    Model** models = calloc(1, sizeof(Model*));

    /* Load model and texture */

    model = load_obj (model_filename);
    texture = load_tex (texture_filename);

    // Error checking
    if (!model) 
    { fprintf(stderr, "Could not load model %s\n", model_filename); return NULL; }
    if (!texture) 
    { fprintf(stderr, "Could not load texture.%s\n", texture_filename); return NULL; }

    // Assign the texture to the model + error checking
    if (assign_tex(model, texture) < NOERR)
    { fprintf(stderr, "WARNING: Model %s does not use a texture.\n", model_filename); }

    models[0] = model;

    /* Scene initialization */

    scene = (Scene*) calloc(1, sizeof(Scene));

    scene->view_area_scale = scale;
    window_size_changed = TRUE;

    scene->model_count = 1; // Could expand the program to render an arbitrary number of
                            // models fairly easily
    scene->models = models;

    camera_xRot = 0;
    camera_yRot = 0;

    /* 
     * Setup OpenGL scene 
     * Could/should separate this into a separate function
     */
    
    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Set triangle color to white
    glColor3f(1.0f, 1.0f, 1.0f);
    
    // Enable some properties
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);

    /* OpenGL lighting */

    // Super simple single light setup
    glEnable(GL_LIGHT0);

    // These arrays should be stored in a Light struct instead
    float ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    float diffuse[] = { 3.0f, 3.0f, 3.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    float specular[] = { 0.2f, 0.2f, 0.2f, 0.2f };
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    float position[] = { 1.0f, 1.5f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    return scene;
}

void render_scene(Scene* scene)
{
    // Check for changes to the window size and dynamically scale
    if (window_size_changed)
    {
        GLfloat nRange = scene->view_area_scale;
        int w = window_width;
        int h = window_height;

        if (h==0) { h = 1; }

        glViewport(0,0,w*2,h*2);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        if (w <= h)
        { glOrtho (-nRange, nRange, -nRange*h/w, nRange*h/w, -nRange, nRange); }
        else
        { glOrtho (-nRange*w/h, nRange*w/h, -nRange, nRange, -nRange, nRange); }

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        window_size_changed = FALSE;
    }

    // Clear OpenGL buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Push eye matrix (includes light, we don't want to rotate it with the model)
    glPushMatrix();

    // Rotate modelview
    glRotatef(camera_xRot, 0.0f, 1.0f, 0.0f);
    glRotatef(camera_yRot, 1.0f, 0.0f, 0.0f);

    // Render models
    for (int i = 0; i < scene->model_count; i++)
    {
        Triangle** modelTris = scene->models[i]->triangles;

        if (scene->models[i]->textured)
        { glBindTexture(GL_TEXTURE_2D, *(scene->models[i]->texture)); }

        glBegin(GL_TRIANGLES); for (int j = 0; j < scene->models[i]->tri_count; j++)
        {
            if (scene->models[i]->textured)
            { glTexCoord2f(modelTris[j]->u1->x, modelTris[j]->u1->y); }
            glNormal3f(modelTris[j]->n1->x, modelTris[j]->n1->y, modelTris[j]->n1->z);
            glVertex3f(modelTris[j]->v1->x, modelTris[j]->v1->y, modelTris[j]->v1->z);
            
            if (scene->models[i]->textured)
            { glTexCoord2f(modelTris[j]->u2->x, modelTris[j]->u2->y); }
            glNormal3f(modelTris[j]->n2->x, modelTris[j]->n2->y, modelTris[j]->n2->z);
            glVertex3f(modelTris[j]->v2->x, modelTris[j]->v2->y, modelTris[j]->v2->z);
            
            if (scene->models[i]->textured)
            { glTexCoord2f(modelTris[j]->u3->x, modelTris[j]->u3->y); }
            glNormal3f(modelTris[j]->n3->x, modelTris[j]->n3->y, modelTris[j]->n3->z);
            glVertex3f(modelTris[j]->v3->x, modelTris[j]->v3->y, modelTris[j]->v3->z);
        } glEnd();
    }

    glPopMatrix();
}

void key_callback (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Rotate camera based on input
    if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
    { camera_yRot -= 2; }
    else if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
    { camera_yRot += 2; }

    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
    { camera_xRot -= 2; }
    else if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
    { camera_xRot += 2; }
}

void window_size_callback(GLFWwindow* window, int w, int h) 
{ 
    // Nasty global variables since we can't pass anything to this function
    window_size_changed = TRUE;
    window_width = w;
    window_height = h;
}