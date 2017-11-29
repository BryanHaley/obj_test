#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>

#include "objtest.h"

/* Magic Numbers */
#define H_SIZE 18 // size of a TGA file header
#define U_RGB 2 // TGA mode Uncompressed RGB
#define RGB_24 24 // 24 bit color depth RGB
#define RGBA_32 32 // 32 bit color depth RGBA

// Container for making sense of a TGA file header
typedef struct tga_header
{
    //Reference: http://www.paulbourke.net/dataformats/tga
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

Texture* load_tex(char* filename)
{
    /* Variables */
    
    // Buffer for reading the header
    unsigned char* buffer = (unsigned char*) calloc(H_SIZE, sizeof(unsigned char));
    // Header object for storing the TGA file's properties
    TGA_Header* header = (TGA_Header*) calloc(1, sizeof(TGA_Header));
    // The TGA file itself
    FILE* tex_file = fopen(filename, "rb");
    // Texture to return
    Texture* textureID = NULL;
    
    /* Parsing the image file */
    
    // error check the file
    if (!tex_file) { fprintf(stderr, "Could not open texture file.\n"); return NULL; }

    // read the header + error checking
    if (fread(buffer, sizeof(char), H_SIZE, tex_file) != H_SIZE)
    { fprintf(stderr, "Texture file corrupted.\n"); return NULL; }

    // Parse the file header and place results in the header object.
    // We can't just read it directly into the object because TGA files are little endian.
    header->id_length = buffer[0];
    header->color_map = buffer[1];
    header->data_type = buffer[2];
    header->color_map_origin = buffer[3] | ((short int) (buffer[4])<<8); //little endian
    header->color_map_length = buffer[5] | ((short int) (buffer[6])<<8);
    header->color_map_depth = buffer[7];
    header->x_origin = buffer[8] | ((short int) (buffer[9])<<8);
    header->y_origin = buffer[10] | ((short int) (buffer[11])<<8);
    header->width = buffer[12] | ((short int) (buffer[13])<<8);
    header->height = buffer[14] | ((short int) (buffer[15])<<8);
    header->bitsperpixel = buffer[16];
    header->descriptor = buffer[17];

    // This function expects uncompressed RGB TGA files, so check
    if (header->data_type != U_RGB)
    { 
        fprintf(stderr, "Expected TGA type 2 (Uncompressed RGB). Got: %d\n", 
                header->data_type);
        return NULL;
    }

    // This function expects 24 bit RGB pixel format, so check
    if (header->bitsperpixel != RGB_24)
    {
        fprintf(stderr, "Expected 24 bits per pixel. Got: %d\n", header->bitsperpixel);
        return NULL;
    }

    // Check if the header is larger and skip past the unneeded data
    if (header->color_map_length != 0)
    {
        fseek(tex_file, header->color_map_origin+header->color_map_length, SEEK_SET);
    }

    else if (header->id_length != 0)
    {
        // The color map always comes after the free form ID so this can be else if
        fseek(tex_file, header->id_length+H_SIZE, SEEK_SET);
    }

    // Calculate the number of bytes needed to hold the pixel data
    int byte_count = header->width*header->height*(header->bitsperpixel/8);

    // Allocate memory for pixels
    unsigned char* data = calloc(byte_count, sizeof(unsigned char));

    // Read the pixels from the TGA file directly into the data object
    if (fread(data, sizeof(unsigned char), byte_count, tex_file) < byte_count)
    { fprintf(stderr, "Unexpected end of texture file.\n"); return NULL; }

    /* 
     * Passing the image to OpenGL
     * In a real program we'd want to separate this part of the function out and have it
     * access the first part of the function through a generic interface so we could create
     * loaders for any file format without reusing the following code.
     */

    // create a GLuint object
    textureID = (GLuint*) calloc(1, sizeof(GLuint));

    // Ask OpenGL to generate a texture and put the ID in our GLuint object
    glGenTextures(1, textureID);

    // Bind the texture so future OpenGL texture operations apply to our texture
    glBindTexture(GL_TEXTURE_2D, *textureID);

    // Pass the pixels read from the TGA file to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, header->width, header->height, 0, 
                 GL_BGR, GL_UNSIGNED_BYTE, data);

    // Set filtering to nearest for demo purposes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    /* Garbage Collection */

    // OpenGL now stores what we need so we can free everything
    free(data); data = NULL;
    free (header); header = NULL;
    fclose(tex_file); tex_file = NULL;

    return textureID;
}

Model* load_obj(char* filename)
{
    /* Variables */

    // counters
    int v_count = 0, v_buff = 0; // vertex count
    int vt_count = 0, vt_buff = 0; // uv coordinates count
    int vn_count = 0, vn_buff = 0; // vertice normals count
    int f_count = 0, f_buff = 0; // face (triangle) count
    
    // related to reading the file
    int scanret = 0; // for storing the return code of scanf
    int buff_size = 32; // buffer size for reading the OBJ file
    char* buff = calloc(buff_size, sizeof(char)); // OBJ file read buffer
    //int v1,u1,n1,v2,u2,n2,v3,u3,n3;
    int* v = (int*) calloc(3, sizeof(int)); // vertex point indice buffer
    int* u = (int*) calloc(3, sizeof(int)); // uv coordinate point indice buffer
    int* n = (int*) calloc(3, sizeof(int)); // vertex normals indice buffer
    
    // objects
    Vector3f** vertices = NULL; // pointer array of vertices
    Vector2f** uvs = NULL; // pointer array of uv coordinates
    Vector3f** normals = NULL; // pointer array of vertice normals
    Triangle** faces = NULL; // pointer array of faces (triangles)

    Model* model = NULL; // the final model to return

    Vector3f* temp = NULL; // used for 3D points during object creation
    Vector2f* temp_vt = NULL; // used for 2D points during object creation
    Triangle* temp_f = NULL; // used for triangles during object creation

    FILE* obj_file = fopen(filename, "r"); // the obj file itself

    /* Parsing the file */

    // error check obj file
    if (!obj_file) { fprintf(stderr, "Could not open %s\n", filename); return NULL; }

    // run through the file once so we know how many objects to make
    while (scanret != EOF)
    {
        // clear the read buffer with terminating zeroes as fscanf does not terminate
        memset(buff, '\0', buff_size);
        scanret = fscanf(obj_file, "%s", buff);
        if (scanret == EOF) { break; }

        // Get count of UV coordinates
        if (strncmp (buff, "vt", 2) == STR_EQUAL)
        { vt_buff++; }

        // Get count of normal vectors
        else if (strncmp (buff, "vn", 2) == STR_EQUAL)
        { vn_buff++; }
        
        // Get count of vertices
        else if (strncmp (buff, "v", 1) == STR_EQUAL)
        { v_buff++; }

        // Get count of triangles
        if (strncmp (buff, "f", 1) == STR_EQUAL)
        { f_buff++; }
    }

    // initialize our objects
    vertices = (Vector3f**) calloc(v_buff, sizeof(Vector3f*));
    uvs = (Vector2f**) calloc(vt_buff, sizeof(Vector2f*));
    normals = (Vector3f**) calloc(vn_buff, sizeof(Vector3f*));
    faces = (Triangle**) calloc(f_buff, sizeof(Triangle*));

    // reset file IO 
    scanret = 0;
    fseek(obj_file, 0, SEEK_SET);

    // parse vertices, uvs, normals, and faces from the file
    while (scanret != EOF)
    {
        // clear the read buffer
        memset(buff, '\0', buff_size);
        scanret = fscanf(obj_file, "%s", buff);
        if (scanret == EOF) { break; }

        // TODO: The process of reading vertices, normals, and UV coordinates out of the
        //       file are almost identical, this should be made into a function for reuse.
        // Get UV coordinates
        if (strncmp (buff, "vt", 2) == STR_EQUAL)
        {
            // allocate memory for the UV coordinate
            temp_vt = (Vector2f*) calloc(1, sizeof(Vector2f));

            // read the coordinates from the file
            scanret = fscanf(obj_file, "%f %f\n", &temp_vt->x, &temp_vt->y);

            // add it to the pointer array
            uvs[vt_count] = temp_vt;
            vt_count++;

            if (scanret == EOF) { fprintf(stderr, "Unexpected end of file.\n"); return NULL; }
        }

        // Get normal vectors
        else if (strncmp (buff, "vn", 2) == STR_EQUAL)
        {
            temp = (Vector3f*) calloc(1, sizeof(Vector3f));
            scanret = fscanf(obj_file, "%f %f %f\n", &temp->x, &temp->y, &temp->z);
            normals[vn_count] = temp;
            vn_count++;

            if (scanret == EOF) { fprintf(stderr, "Unexpected end of file.\n"); return NULL; }
        }
        
        // Get vertices
        else if (strncmp (buff, "v", 1) == STR_EQUAL)
        {
            temp = (Vector3f*) calloc(1, sizeof(Vector3f));
            scanret = fscanf(obj_file, "%f %f %f\n", &temp->x, &temp->y, &temp->z);
            vertices[v_count] = temp;
            v_count++;

            if (scanret == EOF) { fprintf(stderr, "Unexpected end of file.\n"); return NULL; }
        }

        // Get faces
        if (strncmp (buff, "f", 1) == STR_EQUAL)
        {
            // Allocate memory for the triangle
            temp_f = (Triangle*) calloc(1, sizeof(Triangle));

            // Read indices out of the file into respective buffers.
            // Format: "v/u/n v/u/n v/u/n" where three vertices with uv coordinates and
            // normal vectors are defined. u can be empty.
            if (vt_count > 0) // if textured
            {
                scanret = fscanf(obj_file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
                                &v[0], &u[0], &n[0],
                                &v[1], &u[1], &n[1],
                                &v[2], &u[2], &n[2]);
            }

            else
            {
                scanret = fscanf(obj_file, "%d//%d %d//%d %d//%d\n", 
                                &v[0], &n[0],
                                &v[1], &n[1],
                                &v[2], &n[2]);
            }

            // Create triangle from buffers. On the usage of -1 here:
            /*
             * Basically, it's a magic. Dunno why it works like this, it just does.
             * Originally nothing used -1 and the UVs/normals were causing segfaults.
             * I looked at an OBJ file and saw no indices with the value of 0 so I figured
             * it was an off by one error. However, making the vertices use -1 also caused
             * segfaults. How it is now means, in theory, the 0th vertice is never used,
             * but the output is correct, so, magic.
            */
            temp_f->v1 = vertices[v[0]];
            temp_f->v2 = vertices[v[1]];
            temp_f->v3 = vertices[v[2]];

            temp_f->u1 = uvs[u[0]-1];
            temp_f->u2 = uvs[u[1]-1];
            temp_f->u3 = uvs[u[2]-1];

            temp_f->n1 = normals[n[0]-1];
            temp_f->n2 = normals[n[1]-1];
            temp_f->n3 = normals[n[2]-1];
            
            // Add to pointer array
            faces[f_count] = temp_f;
            f_count++;

            if (scanret == EOF) { fprintf(stderr, "Unexpected end of file.\n"); return NULL; }
        }
    }

    // Error checking
    // TODO: Garbage should be collected even when encountering an error
    if (!faces) 
    { fprintf(stderr, "Could not generate faces from %s\n", filename); return NULL; }
    if (v_count != v_buff)
    { fprintf(stderr, "Error reading vertices from %s\n", filename); return NULL; }
    if (vt_count != vt_buff)
    { fprintf(stderr, "Error reading UV coordinates from %s\n", filename); return NULL; }
    if (vn_count != vn_buff)
    { fprintf(stderr, "Error reading normal vectors from %s\n", filename); return NULL; }
    if (f_count != f_buff)
    { fprintf(stderr, "Error reading faces from %s\n", filename); return NULL; }

    // Create Model object for return
    model = calloc(1, sizeof(Model));
    model->textured = FALSE; // default value
    model->triangles = faces;
    model->tri_count = f_count;

    // If the model has UV coordinates, that implies it should be textured
    if (vt_count > 0) { model->textured = TRUE; }

    /* Garbage Collection */

    free (v); v = NULL;
    free (u); u = NULL;
    free (n); n = NULL;

    return model;
}

int assign_tex(Model* model, Texture* tex)
{
    // Error checking
    if (!tex) { return ERR; }
    if (!model->textured) { return ERR; }

    // Associate the model and texture  
    model->texture  = tex;

    return NOERR;
}