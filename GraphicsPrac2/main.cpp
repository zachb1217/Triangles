#define _USE_MATH_DEFINES
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>		// must be downloaded
#include <GL/freeglut.h>	// must be downloaded unless you have an Apple
#endif

unsigned int windowWidth = 512, windowHeight = 512;

// OpenGL major and minor versions
int majorVersion = 3, minorVersion = 0;
bool showTriangle = true;


void getErrorInfo(unsigned int handle)
{
    int logLen;
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
    if (logLen > 0)
    {
        char * log = new char[logLen];
        int written;
        glGetShaderInfoLog(handle, logLen, &written, log);
        printf("Shader log:\n%s", log);
        delete log;
    }
}

// check if shader could be compiled
void checkShader(unsigned int shader, char * message)
{
    int OK;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &OK);
    if (!OK)
    {
        printf("%s!\n", message);
        getErrorInfo(shader);
    }
}

// check if shader could be linked
void checkLinking(unsigned int program)
{
    int OK;
    glGetProgramiv(program, GL_LINK_STATUS, &OK);
    if (!OK)
    {
        printf("Failed to link shader program!\n");
        getErrorInfo(program);
    }
}

// vertex shader in GLSL
const char *vertexSource = R"(
#version 410
precision highp float;
in vec2 vertexPosition;		// variable input from Attrib Array selected by glBindAttribLocation
in vec3 vertexColor;        // variable input from Attrib Array 1 selected by glBindAttribLocation
uniform mat4 MVP;               // Model-View-Projection matrix in row-major format
out vec3 color; // output attribute

void main()
{
    color = vertexColor;				 		// set vertex color
    gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP;  // transform input position by MVP

}
)";

// fragment shader in GLSL
const char *fragmentSource = R"(
#version 410
precision highp float;

in vec3 color;			// variable input: interpolated from the vertex colors
out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

void main()
{
    fragmentColor = vec4(color, 1); // extend RGB to RGBA
}
)";

// row-major matrix 4x4
struct mat4
{
    float m[4][4];
public:
    mat4() {}
    mat4(float m00, float m01, float m02, float m03,
         float m10, float m11, float m12, float m13,
         float m20, float m21, float m22, float m23,
         float m30, float m31, float m32, float m33)
    {
        m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
        m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
        m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
        m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
    }
    
    mat4 operator*(const mat4& right)
    {
        mat4 result;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; k++) result.m[i][j] += m[i][k] * right.m[k][j];
            }
        }
        return result;
    }
    operator float*() { return &m[0][0]; }
};


// 3D point in homogeneous coordinates
struct vec4
{
    float v[4];
    
    vec4(float x = 0, float y = 0, float z = 0, float w = 1)
    {
        v[0] = x; v[1] = y; v[2] = z; v[3] = w;
    }
    
    vec4 operator*(const mat4& mat)
    {
        vec4 result;
        for (int j = 0; j < 4; j++)
        {
            result.v[j] = 0;
            for (int i = 0; i < 4; i++) result.v[j] += v[i] * mat.m[i][j];
        }
        return result;
    }
    
    vec4 operator+(const vec4& vec)
    {
        vec4 result(v[0] + vec.v[0], v[1] + vec.v[1], v[2] + vec.v[2], v[3] + vec.v[3]);
        return result;
    }
};

// 2D point in Cartesian coordinates
struct vec2
{
    float x, y;
    
    vec2(float x = 0.0, float y = 0.0) : x(x), y(y) {}
    
    vec2 operator+(const vec2& v)
    {
        return vec2(x + v.x, y + v.y);
    }
    
   
    
};



// handle of the shader program
unsigned int shaderProgram;

class Object {
protected:
    vec2 scaling; float orientation; vec2 position;
public:
    Object() : scaling(1.0, 1.0), orientation(0.0), position(0.0, 0.0) {}
    
    
    void Draw() {
       
        
        // define the MVPTransform here as mat4 according to scaling, orientation, and position
        mat4 scale = mat4(scaling.x,0.0,0.0,0.0,
                          0.0,scaling.y,0.0,0.0,
                          0.0,0.0,1.0,0.0,
                          0.0,0.0,0.0,1.0);
        
        mat4 rotate = mat4(cos(orientation),sin(orientation),0.0,0.0,
                           -sin(orientation),cos(orientation),0.0,0.0,
                           0.0,0.0,1.0,0.0,
                           0.0,0.0,0.0,1.0);
        
        mat4 translate = mat4(1.0,0.0,0.0,0.0,
                        0.0,1.0,0.0,0.0,
                        0.0,0.0,1.0,0.0,
                        position.x,position.y,0.0,1.0);
        
        mat4 V = mat4(
               1.0, 0, 0, 0,
               0, (float)windowWidth / windowHeight, 0, 0,
               0, 0, 1, 0,
               0, 0, 0, 1);
       

        
        
        mat4 MVPTransform = scale * rotate * translate * V;
        
        
        // define scaling, rotation, and translation matrices separately and multiply them
        
        // set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
        
        
        int location = glGetUniformLocation(shaderProgram, "MVP");
        // set uniform variable MVP to the MVPTransform
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVPTransform);
        else printf("uniform MVPTransform cannot be set\n");
        DrawModel();
    }
    
    

    Object* Scale(const vec2& s) {
        scaling.x *= s.x;
        scaling.y *= s.y;
        return this;
    }
    
    void Animate(){
        double t = glutGet(GLUT_ELAPSED_TIME) * 0.001;
        
        //change position
        orientation = t;
        
        scaling = vec2(sin(t),sin(t));
        
        glutPostRedisplay();
        

    }
    
   
    
    Object* Rotate(float angle) { orientation += angle; return this; }
    
    Object* Translate(const vec2& t) {
        position.x += t.x;
        position.y += t.y;
        return this;
    }
    
    
    virtual void DrawModel() = 0;
    

};

class Triangle: public Object
{
    unsigned int vao;	// vertex array object id
    
public:
    Triangle() {
        glGenVertexArrays(1, &vao);	// create a vertex array object
        glBindVertexArray(vao);		// make it active
        
        unsigned int vbo[2];		// vertex buffer object
        glGenBuffers(2, &vbo[0]);		// generate a vertex buffer object
        
        // vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexColor of the vertex shader
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array
        static float vertexCoords[] = {0, 0, 1, 0, 0 ,1}; // vertex data on the CPU
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
        // map Attribute Array 0 to the current bound vertex buffer (vbo[0])
        glEnableVertexAttribArray(0);
        //data organization of Attribute Array
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        
        // vertex colors: vbo[1] -> Attrib Array 1 -> vertexColor of the vertex shader
        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
        static float vertexColors[] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };// vertex data on the CPU
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);// copy to the GPU
        // map Attribute Array 1 to the current bound vertex buffer (vbo[1])
        glEnableVertexAttribArray(1);  // Vertex position
        // data organization of Attribute Array 1
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        
    }
    
    void DrawModel()
    {
        // make the vao and its vbos active playing the role of the data source
        glBindVertexArray(vao);
        // draw a single triangle with vertices defined in vao
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
};

class Quad : public Object
{
    unsigned int vao;// vertex array object id
    
public:
    Quad(){
        glGenVertexArrays(1, &vao);	// create a vertex array object
        glBindVertexArray(vao);		// make it active
        
        unsigned int vbo[2];		// vertex buffer object
        glGenBuffers(2, &vbo[0]);		// generate a vertex buffer object
        
        // vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexColor of the vertex shader
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array
        static float vertexCoords[] = {0, 0, 1, 0, 0 ,1, 1, 1}; // vertex data on the CPU
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
        // map Attribute Array 0 to the current bound vertex buffer (vbo[0])
        glEnableVertexAttribArray(0);
        //data organization of Attribute Array
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        
        // vertex colors: vbo[1] -> Attrib Array 1 -> vertexColor of the vertex shader
        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
        static float vertexColors[] = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1 };// vertex data on the CPU
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);// copy to the GPU
        // map Attribute Array 1 to the current bound vertex buffer (vbo[1])
        glEnableVertexAttribArray(1);  // Vertex position
        // data organization of Attribute Array 1
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    
    
    void DrawModel()
    {
        // make the vao and its vbos active playing the role of the data source
        glBindVertexArray(vao);
        // draw a single triangle with vertices defined in vao
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};


class Scene
{
    std::vector<Object*> objects;
public:
    void AddObject(Object* o) { objects.push_back(o); }
    
    ~Scene()
    {
        for(int i = 0; i < objects.size(); i++) delete objects[i];
    }
    
    void animateScene(){
        for(int i = 0; i < objects.size(); i++) objects[i]->Animate();
    }
    
    void Draw()
    {
        for(int i = 0; i < objects.size(); i++) objects[i]->Draw();
    }
};






// the virtual world: a single triangle

Scene scene;


// initialization, create an OpenGL context
void onInitialization()
{
    
    glViewport(0, 0, windowWidth, windowHeight);
    
    
    
    scene.AddObject((new Triangle())->Translate(vec2(-0.25,0.25)));
    scene.AddObject((new Triangle())->Translate(vec2(-0.25,-0.25)));
    scene.AddObject((new Triangle())->Translate(vec2(0.25,-0.25)));
    scene.AddObject((new Triangle())->Translate(vec2(0.25,0.25)));
    
    


    


    
    
    
    
    // create objects by setting up their vertex data on the GPU
    

    
    // create vertex shader from string
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }
    
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    checkShader(vertexShader, "Vertex shader error");
    
    // create fragment shader from string
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }
    
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    checkShader(fragmentShader, "Fragment shader error");
    
    // attach shaders to a single program
    shaderProgram = glCreateProgram();
    if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }
    
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    
    // connect Attrib Array to input variables of the vertex shader
    glBindAttribLocation(shaderProgram, 0, "vertexPosition"); // vertexPosition gets values from Attrib Array 0
    glBindAttribLocation(shaderProgram, 1, "vertexColor"); // vertexColor gets values from Attrib Array 1
    
    
    
    // connect the fragmentColor to the frame buffer memory
    glBindFragDataLocation(shaderProgram, 0, "fragmentColor"); // fragmentColor goes to the frame buffer memory
    
    
    // program packaging
    glLinkProgram(shaderProgram);
    checkLinking(shaderProgram);
    // make this program run
    glUseProgram(shaderProgram);
}

void onExit()
{
    
 
    glDeleteProgram(shaderProgram);
    printf("exit");
}

// window has become invalid: redraw
void onDisplay()
{
    glClearColor(0, 0, 0, 0); // background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
    
    scene.Draw();
    

   
  
   
    glutSwapBuffers(); // exchange the two buffers
    
    
}

void onReshape(int winWidth0, int winHeight0) {
    windowWidth = winWidth0;
    windowHeight = winHeight0;

    glViewport(0, 0, winWidth0, winHeight0);
    }

void onKeyboard(unsigned char x, int y, int z) {
    showTriangle = !showTriangle;
    glutPostRedisplay();
}

void onIdle(){
  
    scene.animateScene();
    
}

int main(int argc, char * argv[])
{
    glutInit(&argc, argv);
#if !defined(__APPLE__)
    glutInitContextVersion(majorVersion, minorVersion);
#endif
    glutInitWindowSize(windowWidth, windowHeight); 	// application window is initially of resolution 512x512
    glutInitWindowPosition(50, 50);			// relative location of the application window
#if defined(__APPLE__)
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);  // 8 bit R,G,B,A + double buffer + depth buffer
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    glutCreateWindow("Triangle Rendering");
    
#if !defined(__APPLE__)
    glewExperimental = true;
    glewInit();
#endif
    printf("GL Vendor    : %s\n", glGetString(GL_VENDOR));
    printf("GL Renderer  : %s\n", glGetString(GL_RENDERER));
    printf("GL Version (string)  : %s\n", glGetString(GL_VERSION));
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    printf("GL Version (integer) : %d.%d\n", majorVersion, minorVersion);
    printf("GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    onInitialization();
    
    glutDisplayFunc(onDisplay); // register event handlers
    glutReshapeFunc(onReshape);
    glutKeyboardFunc(onKeyboard);
    glutIdleFunc(onIdle);
    
    
    glutMainLoop();
    onExit();
    return 1;
}


