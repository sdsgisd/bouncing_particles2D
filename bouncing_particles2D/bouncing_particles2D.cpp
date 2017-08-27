//
//  bouncing_particles2D.cpp
//  BOUNCING_PARTICLES2D
//
//  Created by Sadashige Ishida on 8/27/17.
//
//

#include <png.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <cmath>
#include <vector>
#include <string>
#include <random>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cassert>

using std::vector;
using std::fabs;

bool record_images=false;
std::string output_directory="output_images";

//very small number.
const float epsilon=0.001f;

//Frame per second
const double FPS = 60;

const bool synchronize_with_real_time=1;
const float dt = synchronize_with_real_time?1/FPS:0.1;

const int num_particle=20;
const bool uniform_density=1;
const bool uniform_radius=0;
const float particle_radius=0.03f;
const float radius_max=uniform_radius?particle_radius: 0.07f;
const float radius_min=uniform_radius?particle_radius: 0.01f;

//max and min velocity
const float v_min=-1.f,v_max=1.f;

const float gravity_coefficient=0.98f;
const GLfloat external_force_coef=uniform_density?0.01f:2.f;

//damp coef for colliding the wall.
const float damp=1.f;//0.98f;

const float collisionDamp=0.95f;
//coef for friction at the floor.
const float friction=1.f;//0.98f;

float window_ratio;
int window_width, window_height;

const float ceil_=-0.9f,floor_=0.9f;
const float left_=-0.9f,right_=0.9f;

class Particle;
vector<Particle> particles;

namespace png_writer {
    
    struct Pixel
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };
    
    Pixel* m_data; // raw image data
    
    //Reference for writing png:
    //https://github.com/olkido/libigl_smgpclass/blob/master/external/yimg/YImage.cpp
    
    Pixel&
    at( int i, int j )
    
    {
        assert( i >= 0 && i < window_width ) ;
        assert( j >= 0 && j < window_height ) ;
        
        return m_data[ i + j * window_width ] ;
    }
    
    void flip_ceil_to_floor()
    {
        
        for( int j = 0 ; j < window_height / 2 ; ++j )
            for( int i = 0 ; i < window_width ; ++i )
            {
                Pixel* lhs = &at( i,j ) ;
                Pixel* rhs = &at( i, window_height - j - 1 ) ;
                
                Pixel temp = *lhs ;
                
                *lhs = *rhs ;
                *rhs = temp ;
            }
    }
    
    // We use out own reading/writing functions because libpng may have
    // been compiled using a different compiler & libc.  That could make
    // the FILE*'s incompatible.
    inline static void write_func(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        png_voidp write_io_ptr = png_get_io_ptr(png_ptr);
        fwrite((unsigned char*) data, length, 1, (FILE*) write_io_ptr);
    }
    
    inline static void flush_func(png_structp png_ptr)
    {
        png_voidp write_io_ptr = png_get_io_ptr(png_ptr);
        fflush((FILE*) write_io_ptr);
    }
    
    inline bool save_png(const char* fname)
    {
        
        m_data = (Pixel*) malloc(window_width * window_height * sizeof(Pixel));
        
        glReadPixels(0, 0, window_width, window_height, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char *)(m_data));
        
        flip_ceil_to_floor();
        FILE* fp = NULL;
        bool rval = true;
        png_structp png_ptr = NULL;
        png_infop info_ptr = NULL;
        
        // Some error strings
        char open[] = "Error opening %s\n";
        char problem[] = "libpng encountered a problem writing %s)\n";
        
        // Open the file for reading in binary mode.
        fp = fopen(fname, "wb");
        
        if (!fp) {
            fprintf(stderr, open, fname);
            rval = false;
            goto YImage_save_cleanup;
        }
        
        // Allocate the png structs.
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        
        if (!png_ptr) {
            fprintf(stderr, problem, fname);
            rval = false;
            goto YImage_save_cleanup;
        }
        
        info_ptr = png_create_info_struct(png_ptr);
        
        if (!info_ptr) {
            fprintf(stderr, problem, fname);
            rval = false;
            goto YImage_save_cleanup;
        }
        
        // Set up the png error routine.
        if (setjmp(png_jmpbuf(png_ptr))) {
            fprintf(stderr, problem, fname);
            rval = false;
            goto YImage_save_cleanup;
        }
        
        // Give libpng the FILE*.
        // png_init_io(png_ptr, fp);
        // or
        // use our own write callback
        png_set_write_fn(png_ptr, fp, (png_rw_ptr) write_func, flush_func);
        //png_init_io(png_ptr, fp);
        
        // We'll use the low-level interface since the high-level interface won't handle
        // png_set_filler() which we need to tell libpng to strip out the A from our
        // 4-byte pixels.
        
        // First we set and write the info struct.
        png_set_IHDR(
                     png_ptr, info_ptr,
                     window_width, window_height,
                     8, PNG_COLOR_TYPE_RGB_ALPHA,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
                     );
        
        png_write_info(png_ptr, info_ptr);
        
        // Then we set up transforms.
        /*
         // 1. tell libpng to strip out the filler byte in our 4-byte pixels
         // if YPixel::a comes after any other member (b,g,r), we stip AFTER
         if (offsetof(YPixel, a) > offsetof(YPixel, b)) {
         png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
         // printf("alpha after\n");
         } else {
         png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);
         // printf("alpha before\n");
         }
         */
        if (offsetof(Pixel, a) < offsetof(Pixel, b))
            png_set_swap_alpha(png_ptr);
        
        // 2. tell libpng how our color triples are stored (b < r or vice versa)
        if (offsetof(Pixel, b) < offsetof(Pixel, r)) {
            png_set_bgr(png_ptr);
            // printf("bgr\n");
        }
        
        // else { printf("rgb\n"); }
        
        // Finally we create a row_pointers[] pointing into our data* and write the png out to the FILE*.
        {
            // 1. allocate row pointers array
            png_bytep* row_pointers = (png_bytep*) png_malloc(png_ptr, window_height * sizeof(png_bytep));
            // 2. point row pointers into m_data
            
            for (int i = 0; i < window_height; ++i) {
                row_pointers[i] = (png_bytep)(m_data + i * window_width);
            }
            
            // 3. write the image data
            png_write_image(png_ptr, row_pointers);
            
            // 4. free row pointers array
            png_free(png_ptr, row_pointers);
        }
        
        // Write out end info.  We're done.  Fall through to cleanup.
        png_write_end(png_ptr, NULL);
        
    YImage_save_cleanup:
        png_destroy_write_struct(png_ptr ? &png_ptr : NULL, info_ptr ? &info_ptr : NULL);
        
        if (fp) fclose(fp);
        
        return rval;
    }
    
}

struct Vector2{
    
    GLfloat pvec[2];
    
    inline Vector2(const GLfloat & x, const GLfloat & y){
        pvec[0]=x;
        pvec[1]=y;
    }
    
    inline Vector2(const GLfloat & scalar){
        pvec[0]=scalar;
        pvec[1]=scalar;
    }
    
    //PVector(){}
    
    inline GLfloat& operator[](const int & index){
        return pvec[index];
    }
    inline Vector2 operator*(const GLfloat & scalar)const {
        return Vector2(scalar*pvec[0],scalar*pvec[1]);
    }
    
    inline Vector2 operator/(const GLfloat & scalar)const {
        return Vector2(1.f/scalar*pvec[0],1.f/scalar*pvec[1]);
    }
    
    inline Vector2 operator+(const Vector2 & anotherVector) const{
        return Vector2(this->pvec[0]+anotherVector.pvec[0],this->pvec[1]+anotherVector.pvec[1]);
    }
    
    inline Vector2 operator-(void) const{
        return Vector2(-pvec[0],-pvec[1]);
    }
    
    inline Vector2 operator-(const Vector2&  anotherVector)const{
        return *this+(-anotherVector);
    }
    
    inline void operator+=(const Vector2 & anotherVector){
        pvec[0]+=anotherVector.pvec[0];
        pvec[1]+=anotherVector.pvec[1];
        return;
    }
    
    inline void operator-=(const Vector2 & anotherVector){
        pvec[0]-=anotherVector.pvec[0];
        pvec[1]-=anotherVector.pvec[1];
        return;
    }
    
    inline void operator*=(const GLfloat & scalar){
        pvec[0]*=scalar;
        pvec[1]*=scalar;
        return;
    }
    
    inline float dot(const Vector2 & anotherVector)const{
        return pvec[0]*anotherVector.pvec[0]+pvec[1]*anotherVector.pvec[1];
        
    }
    
    inline GLfloat norm()const {
        return sqrtf(pvec[0]*pvec[0]+pvec[1]*pvec[1]);
    }
    
    ~Vector2(){
    }
};

inline Vector2 operator*(const GLfloat & scalar, const Vector2 & aVector){
    return aVector*scalar;
}

const Vector2 gravity=Vector2(0, gravity_coefficient);

class Particle {
public:
    float radius;
    float mass;
    float duration;//duration of existance. Not used now.
    
    Vector2 position;
    Vector2 velocity;
    Vector2 force;
    Vector2 original_force;
    
    Particle(const Vector2 & position, const Vector2 & velocity, const float mass,const float radius=particle_radius):radius(radius),mass(mass),position(position),velocity(velocity),force(gravity*mass),original_force(force) {
        
    }
    
    inline void resetForce(){
        force=original_force;
    }
    
    inline void draw(){
        glVertex2f(position[0] , position[1]);
    }
    
    inline void addForce(const Vector2 & external_force){
        force=force+external_force;
    }
    
    //Euler integration
    inline void update(){
        
        auto prevPosition=position;
        
        velocity=velocity+dt/mass*force;
        position=position+velocity*dt;
        
        if (position[0]>right_ ) {
            velocity[0]=-damp*fabs(velocity[0]);
            
        } else if (position[0]<left_ ) {
            velocity[0]=damp*fabs(velocity[0]);
        }
        
        if (position[1]>floor_ -epsilon) {
            position[1]=floor_;
            velocity[1]=-damp*fabs(velocity[1]);
        } else if (position[1]<ceil_ ) {
            velocity[1]=+damp*fabs(velocity[1]);
        }
        
        //friction
        if(prevPosition[1]>floor_-epsilon&& position[1]>floor_-epsilon){
            velocity[0]=friction* velocity[0];
        }
        
    }
    
    inline bool detectCollide(const Particle &particle){
        float dist=(position-particle.position).norm();
        if(dist<radius+particle.radius){
            return true;
        }
        
        return false;
    }
    
    inline void respondCollide(Particle &particle){
        Vector2 diff=position-particle.position;
        Vector2 k(diff/diff.norm());
        float a=2*k.dot(velocity-particle.velocity )/(1/mass+1/particle.mass);
        velocity=collisionDamp*(velocity-(a/mass)*k);
        particle.velocity=collisionDamp*(particle.velocity+(a/particle.mass)*k);
    }
    
    //This function handles both detect and response to collision.
    inline bool collide(Particle &particle){
        
        bool isColliding=detectCollide(particle);
        if(isColliding){
            respondCollide(particle);
        }
        
        return isColliding;
    }
    
};

inline void save_image(){
    
    static int image_count=0;
    
    std::stringstream filename_ss;
    
    mkdir(output_directory.c_str(), 0755);
    filename_ss <<output_directory<< "/image" << std::setfill('0') << std::setw(6) << image_count++ << ".png";
    
    png_writer::save_png(filename_ss.str().c_str());
    
}

inline void handleCollision(){
    for(int i=0;i!=particles.size();++i){
        for(int j=i+1;j!=particles.size();++j){
            
            particles[i].collide(particles[j]);
            
        }
    }
    
}

inline void drawFrame(){
    static const GLfloat vtx3[] = {
        left_, ceil_,
        left_, floor_,
        right_, floor_,
        right_, ceil_,
    };
    
    glVertexPointer(2, GL_FLOAT, 0, vtx3);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    glEnable(GL_MULTISAMPLE);
    glMatrixMode(GL_MODELVIEW);
    
    //glDrawArrays has to be sandwiched by glEnableCliantState and glDisableCliantState.
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glPushMatrix();
    glDrawArrays(GL_QUADS, 0, 4);
    glPopMatrix();
    
    glDisableClientState(GL_VERTEX_ARRAY);
}

inline void step(){
    
    for( auto it=particles.begin();it!=particles.end();it++){
        it->update();;
    }
    handleCollision();
    
}

inline void drawParticles(){
    glColor3f(0.f, 0.f, 1.f);
    float sizePx;
    
    glEnable(GL_POINT_SMOOTH);
    if(uniform_radius){
        sizePx=(window_height*particle_radius/(1.f+1.f)*2.f);
        glPointSize( sizePx );
        
        glBegin(GL_POINTS);
        for( auto &particle:particles){
            
            particle.draw();
            
        }
        glEnd();
        
    }else{
        
        for( auto &particle:particles){
            
            sizePx=(window_height*particle.radius/(1.f+1.f)*2.f);
            
            glPointSize( sizePx );
            
            glBegin(GL_POINTS);
            
            particle.draw();
            
            glEnd();
        }
        
    }
    
}

void generate_particles(){
    
    //Generate particles.
    //Initial positions and velocities are given by Mersenne twister.
    
    static std::random_device rd;
    //Random number generater using Mersenne twister.
    static std::mt19937 engine(rd()) ;
    
    //Generate positions and velocities under uniform distribution.
    std::uniform_real_distribution<GLfloat> distribution_x( left_,  right_) ;
    std::uniform_real_distribution<GLfloat> distribution_y( ceil_,  floor_) ;
    std::uniform_real_distribution<GLfloat> distribution_vx( v_min,  v_max) ;
    std::uniform_real_distribution<GLfloat> distribution_vy( v_min,  v_max) ;
    
    std::uniform_real_distribution<GLfloat> distribution_r( radius_min,  radius_max) ;
    
    for(int i=0;i<num_particle;i++){
        float x=distribution_x(engine);
        float y=distribution_y(engine);
        float vx=distribution_vx(engine);
        float vy=distribution_vy(engine);
        
        float r=distribution_r(engine);
        
        float mass=uniform_density?1.f*r*r:1.f;
        
        particles.push_back((Particle(Vector2(x, y), Vector2(vx, vy), mass,r)));
    }
}

inline void addForce(Vector2 external_force){
    for(auto it=particles.begin();it!=particles.end();it++){
        it->addForce(external_force);
    }
}

const float goToAndFrom_speed=0.1f;//0.1f;
inline void goToPoint(Vector2 point){
    for(auto it=particles.begin();it!=particles.end();it++){
        Vector2 direction=point-it->position;
        
        //it->force=it->force+goToAndFrom_speed*direction;
        it->velocity+=goToAndFrom_speed*direction;
    }
}

inline void goFromPoint(Vector2 point){
    for(auto it=particles.begin();it!=particles.end();it++){
        Vector2 direction=point-it->position;
        float distance=direction.norm();
        
        //it->force=it->force-goToAndFrom_speed*direction;
        it->velocity-=goToAndFrom_speed*direction/(distance*distance);
        
    }
}

static void clear(){
    particles.clear();
}

inline void render (GLFWwindow * window){
    
    glfwGetFramebufferSize(window, &window_width, &window_height);
    window_ratio = window_width / (float)window_height;
    glViewport(0, 0, window_width, window_height);
    glClear(GL_COLOR_BUFFER_BIT);
    
    //Make the aspect ratio to be invariant for window resize.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    //Specify the domain to be rendered.
    //If setting up, glOrtho(0.0f, width, 0.0f, height, -1.0f, 1.0f);
    //then the window size coincides with the inner scale.
    glOrtho(-window_ratio, window_ratio, -1.f, 1.f, 1.f, -1.f);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glPushMatrix();
    drawFrame();
    glPopMatrix();
    
    glPushMatrix();
    glRotatef(180, 1, 0, 0);
    drawParticles();
    glPopMatrix();
    
    glfwSwapBuffers(window);
    glfwPollEvents();
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action,
                         int mods)
{
    
    if ( key==GLFW_KEY_A  &&action==GLFW_PRESS){
        generate_particles();
    }
    
    if ( (key==GLFW_KEY_R or key==GLFW_KEY_0)  &&action==GLFW_PRESS){
        clear();
        generate_particles();
    }
    
    if ( key==GLFW_KEY_S &&action==GLFW_PRESS){
        
        save_image();
        
    }
    
    if ( key==GLFW_KEY_W &&action==GLFW_PRESS){
        
        record_images=!record_images;
        
    }
    
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    Vector2 ex_force(0,0);
    if (key==GLFW_KEY_RIGHT ){
        ex_force+=Vector2(external_force_coef,0);
    }
    if (key==GLFW_KEY_LEFT ){
        ex_force+=Vector2(-external_force_coef,0);
    }
    if (key==GLFW_KEY_DOWN ){
        ex_force+=Vector2(0,external_force_coef);
    }
    if (key==GLFW_KEY_UP ){
        ex_force+=Vector2(0,-external_force_coef);
    }
    
    if(action==GLFW_PRESS){
        addForce(ex_force);
    }else if (action==GLFW_RELEASE){
        addForce(-ex_force);
    }
}

static void mouse_input(GLFWwindow*window){
    
    if( glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ) {
        double mousex, mousey;
        glfwGetCursorPos(window,&mousex, &mousey);
        
        //Translate the mouse coordinate (mousex,mousey) in [0,width]*[0,height]
        //to target position in [-1,1]*[-,1,1].
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        GLfloat x,y;
        GLfloat W=width/2,H=height/2;
        x=(mousex-W)/H;
        y=(mousey/height-0.5)*2;
        Vector2 target_position(x,y);
        goToPoint(target_position);
        
    }
    
    if( glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE ) {
        
    }
    
    if( glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS ) {
        double mousex, mousey;
        glfwGetCursorPos(window,&mousex, &mousey);
        
        //Translate the mouse coordinate (mousex,mousey) in [0,width]*[0,height]
        //to target position in [-1,1]*[-,1,1].
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        GLfloat x,y;
        GLfloat W=width/2,H=height/2;
        x=(mousex-W)/H;
        y=(mousey/height-0.5)*2;
        Vector2 target_position(x,y);
        goFromPoint(target_position);
        
    }
    if( glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE ) {
        
    }
    
}

void show_key_mouse_setting(){
    using std::cout;
    using std::endl;
    cout<<"[KEY SETTING]"<<endl;
    cout<<"r/0: Reset."<<endl;
    cout<<"a: Add "<<num_particle<<" particles."<<endl;
    cout<<"Arrow keys: Act force."<<endl;
    cout<<"s: Save a screen shot."<<endl;
    cout<<"w: Turn on/off recording mode. Images are saved in \""<<output_directory<<"\"directory."<<endl;

    cout<<endl;
    cout<<"[MOUSE SETTING]"<<endl;
    cout<<"Left click: Pull particles."<<endl;
    cout<<"Right click: Repel particles."<<endl;
    
}

int main(void)
{
    show_key_mouse_setting();
    
    //Set up OpenGL.
    GLFWwindow* window;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);
    window = glfwCreateWindow(60*16, 60*9, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    glfwSetKeyCallback(window, key_callback);
    
    //Timer setting
    double currentTime, lastTime, elapsedTime;
    currentTime = lastTime = elapsedTime = 0.0;
    glfwSetTime(0.0); //Initialize timer.
    
    generate_particles();
    
    while (!glfwWindowShouldClose(window))
    {
        
        glClearColor(0.3f, .3f, 0.3f, 0.f);
        currentTime = glfwGetTime();
        elapsedTime = currentTime - lastTime;
        
        if (elapsedTime > 1.0/FPS) {
            
            mouse_input(window);
            
            step();
            
            render(window);
            
            if(record_images){
                save_image();
            }
            
            lastTime = glfwGetTime();
        }
        
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}