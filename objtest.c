#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define GL_PI 3.1415f
#define ERR -1
#define NOERR 0
#define DEF_WIN_WIDTH 640
#define DEF_WIN_HEIGHT 480

#define NOERR  0
#define ERR   -1
#define STR_EQUAL 0

typedef struct vector2f
{
    float x;
    float y;
} Vector2f;

typedef struct vector3f
{
    float x;
    float y;
    float z;
} Vector3f;

typedef struct triangle
{
    Vector3f* v1;
    Vector3f* v2;
    Vector3f* v3;

    Vector2f* u1;
    Vector2f* u2;
    Vector2f* u3;

    Vector3f* n1;
    Vector3f* n2;
    Vector3f* n3;
} Triangle;

typedef struct tga_header
{
    char id_length;
    char color_map;
    char data_type;
    short int color_map_origin;
    short int color_map_length;
    char color_map_depth;
    short int x_origin;
    short int y_origin;
    short width;
    short height;
    char bitsperpixel;
    char descriptor;
} TGA_Header;

float model_xRot, model_yRot;
int triangle_count;
Triangle** triangles;
GLuint* textureID;

void render_scene(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_TEXTURE_2D);

    glPushMatrix();
    glRotatef(model_xRot, 1.0f, 0.0f, 0.0f);
    glRotatef(model_yRot, 0.0f, 1.0f, 0.0f);

    glBindTexture(GL_TEXTURE_2D, *textureID);

    glBegin(GL_TRIANGLES);

        for (int i = 0; i < triangle_count; i++)
        {
            glTexCoord2f(triangles[i]->u1->x, triangles[i]->u1->y);
            glNormal3f(triangles[i]->n1->x, triangles[i]->n1->y, triangles[i]->n1->z);
            glVertex3f(triangles[i]->v1->x, triangles[i]->v1->y, triangles[i]->v1->z);
            
            glTexCoord2f(triangles[i]->u2->x, triangles[i]->u2->y);
            glNormal3f(triangles[i]->n2->x, triangles[i]->n2->y, triangles[i]->n2->z);
            glVertex3f(triangles[i]->v2->x, triangles[i]->v2->y, triangles[i]->v2->z);
            
            glTexCoord2f(triangles[i]->u3->x, triangles[i]->u3->y);
            glNormal3f(triangles[i]->n3->x, triangles[i]->n3->y, triangles[i]->n3->z);
            glVertex3f(triangles[i]->v3->x, triangles[i]->v3->y, triangles[i]->v3->z);
        }

    glEnd();

    glPopMatrix();
}

void key_callback (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
    { model_xRot -= 2; }
    else if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
    { model_xRot += 2; }

    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
    { model_yRot -= 2; }
    else if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
    { model_yRot += 2; }
}

void window_size_callback(GLFWwindow* window, int w, int h)
{
    GLfloat nRange = 5.0f;

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
}

void init_scene(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glColor3f(1.0f, 1.0f, 1.0f);
}


int load_tex(char* filename)
{
    unsigned char* buffer = (unsigned char*) calloc(18, sizeof(unsigned char));

    // Parse image file
    TGA_Header* header = (TGA_Header*) calloc(1, sizeof(TGA_Header));
    FILE* tex_file = fopen(filename, "rb");
    if (!tex_file) { fprintf(stderr, "Could not open texture file.\n"); return ERR; }

    if (fread(buffer, sizeof(char), 18, tex_file) != 18)
    { fprintf(stderr, "Texture file corrupted.\n"); return ERR; }

    header->id_length = buffer[0];
    header->color_map = buffer[1];
    header->data_type = buffer[2];
    header->color_map_origin = buffer[3] + ((short int) (buffer[4])<<8); //little endian
    header->color_map_length = buffer[5] + ((short int) (buffer[6])<<8);
    header->color_map_depth = buffer[7];
    header->x_origin = buffer[8] + ((short int) (buffer[9])<<8);
    header->y_origin = buffer[10] + ((short int) (buffer[11])<<8);
    header->width = buffer[12] + ((short int) (buffer[13])<<8);
    header->height = buffer[14] + ((short int) (buffer[15])<<8);
    header->bitsperpixel = buffer[16];
    header->descriptor = buffer[17];

    if (header->data_type != 2)
    { 
        fprintf(stderr, "Expected TGA type 2 (Uncompressed RGB). Got: %d\n", header->data_type);
        return ERR;
    }

    if (header->bitsperpixel != 24)
    {
        fprintf(stderr, "Expected 24 bits per pixel. Got: %d\n", header->bitsperpixel);
        return ERR;
    }

    if (header->color_map_length != 0)
    {
        //skip past unneeded data
        fseek(tex_file, header->color_map_origin+header->color_map_length, SEEK_SET);
    }

    else if (header->id_length != 0)
    {
        //skip past unneeded data
        fseek(tex_file, header->id_length+18, SEEK_SET);
    }

    int byte_count = header->width*header->height*(header->bitsperpixel/8);

    unsigned char* data = calloc(byte_count, sizeof(unsigned char));

    if (fread(data, sizeof(unsigned char), byte_count, tex_file) < byte_count)
    { fprintf(stderr, "Unexpected end of texture file.\n"); return ERR; }

    // Load into OpenGL
    textureID = (GLuint*) calloc(1, sizeof(GLuint));

    glGenTextures(1, textureID);

    glBindTexture(GL_TEXTURE_2D, *textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, header->width, header->height, 0, 
                 GL_BGR, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    free(data); data = NULL;

    return NOERR;
}

int load_obj(char* filename)
{
    //Load OBJ model
    int v_count = 0;
    int vt_count = 0;
    int vn_count = 0;
    int f_count = 0;
    int scanret = 0;
    int buff_size = 1024;
    char* buff = calloc(buff_size, sizeof(char));
    Vector3f** vertices = (Vector3f**) calloc(buff_size, sizeof(Vector3f*));
    Vector2f** uvs = (Vector2f**) calloc(buff_size, sizeof(Vector2f*));
    Vector3f** normals = (Vector3f**) calloc(buff_size, sizeof(Vector3f*));
    Vector3f* temp;
    Vector2f* temp_vt;
    Triangle** faces = (Triangle**) calloc(buff_size, sizeof(Triangle*));
    Triangle* temp_f;

    //int v1,u1,n1,v2,u2,n2,v3,u3,n3;
    int* v = (int*) calloc(3, sizeof(int));
    int* u = (int*) calloc(3, sizeof(int));
    int* n = (int*) calloc(3, sizeof(int));

    FILE* cube_obj = fopen(filename, "r");
    if (!cube_obj) { fprintf(stderr, "Could not open cube.obj\n"); return ERR; }

    while (scanret != EOF)
    {
        memset(buff, '\0', buff_size);
        scanret = fscanf(cube_obj, "%s", buff);
        if (scanret == EOF) { break; }

        // Get UVs
        if (strncmp (buff, "vt", 2) == STR_EQUAL)
        {
            temp_vt = (Vector2f*) calloc(1, sizeof(Vector2f));
            scanret = fscanf(cube_obj, "%f %f\n", &temp_vt->x, &temp_vt->y);
            uvs[vt_count] = temp_vt;
            vt_count++;

            if (scanret == EOF) { fprintf(stderr, "Unexpected end of file.\n"); return ERR; }
        }

        // Get Normals
        else if (strncmp (buff, "vn", 2) == STR_EQUAL)
        {
            temp = (Vector3f*) calloc(1, sizeof(Vector3f));
            scanret = fscanf(cube_obj, "%f %f %f\n", &temp->x, &temp->y, &temp->z);
            normals[vn_count] = temp;
            vn_count++;

            if (scanret == EOF) { fprintf(stderr, "Unexpected end of file.\n"); return ERR; }
        }
        
        // Get vertices
        else if (strncmp (buff, "v", 1) == STR_EQUAL)
        {
            temp = (Vector3f*) calloc(1, sizeof(Vector3f));
            scanret = fscanf(cube_obj, "%f %f %f\n", &temp->x, &temp->y, &temp->z);
            vertices[v_count] = temp;
            v_count++;

            if (scanret == EOF) { fprintf(stderr, "Unexpected end of file.\n"); return ERR; }
        }

        // Get faces
        if (strncmp (buff, "f", 1) == STR_EQUAL)
        {
            temp_f = (Triangle*) calloc(1, sizeof(Triangle));
            scanret = fscanf(cube_obj, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
                             &v[0], &u[0], &n[0],
                             &v[1], &u[1], &n[1],
                             &v[2], &u[2], &n[2]);

            temp_f->v1 = vertices[v[0]];
            temp_f->v2 = vertices[v[1]];
            temp_f->v3 = vertices[v[2]];

            temp_f->u1 = uvs[u[0]-1];
            temp_f->u2 = uvs[u[1]-1];
            temp_f->u3 = uvs[u[2]-1];

            temp_f->n1 = normals[n[0]-1];
            temp_f->n2 = normals[n[1]-1];
            temp_f->n3 = normals[n[2]-1];
            
            faces[f_count] = temp_f;
            f_count++;

            if (scanret == EOF) { fprintf(stderr, "Unexpected end of file.\n"); return ERR; }
        }
    }

    free (v); v = NULL;
    free (u); u = NULL;
    free (n); n = NULL;

    triangles = faces;
    triangle_count = f_count;

    return NOERR;
}

int main(int argc, char** argv)
{
    char* obj_file = "cube.obj";
    char* tex_file = "tex.tga";
    
    if (argc > 1)
    { obj_file = argv[1]; }

    if (argc > 2)
    { tex_file = argv[2]; }

    model_xRot = 0;
    model_yRot = 0;

    GLFWwindow* window;

    if (!glfwInit()) { fprintf(stderr, "Could not init window system.\n"); return ERR; }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    
    window = glfwCreateWindow (DEF_WIN_WIDTH, DEF_WIN_HEIGHT, "OBJ Import", NULL, NULL);
    
    if (!window) { fprintf(stderr, "Could not init window.\n"); glfwTerminate(); return ERR; }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

    init_scene();
    
    window_size_callback(window, DEF_WIN_WIDTH, DEF_WIN_HEIGHT);

    if (load_obj(obj_file) == ERR) { fprintf(stderr, "Could not load OBJ file.\n"); return ERR; }
    if (load_tex(tex_file) == ERR) { fprintf(stderr, "Could not load texture.\n"); return ERR; }
    
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        render_scene();
        
        glfwSwapBuffers(window);
    }

    return NOERR;
}
