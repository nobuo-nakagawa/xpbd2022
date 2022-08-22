#include <cstdlib>

#if defined(WIN32)
#pragma warning(disable:4996)
#include <GL/glut.h>
#ifdef NDEBUG
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif // NDEBUG
#elif defined(__APPLE__) || defined(MACOSX)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <GLUT/glut.h>
#else // MACOSX
#include <GL/glut.h>
#endif // unix

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/norm.hpp"
#include "glm/gtx/string_cast.hpp"
#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl2.h"
#include <vector>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <cfloat>

#define USE_TEST_SCENE (0)
#define USE_DOUBLE     (0)
#define USE_CAPTURE    (0)

#if !(USE_DOUBLE)
typedef float     Float;
typedef glm::vec3 Vec3;
typedef glm::vec2 Vec2;
typedef glm::quat Quat;
typedef glm::mat3 Mat3;
#else
typedef double       Float;
typedef glm::f64vec3 Vec3;
typedef glm::f64vec2 Vec2;
typedef glm::dquat   Quat;
typedef glm::dmat3   Mat3;
#endif

namespace {
  Float      FIXED_DT  = (Float)(1.0 / 30.0);
  Float      GRAVITY   = (Float)9.8;
  enum eMat : int {
    eMat_Concrete,
    eMat_Wood,
    eMat_Leather,
    eMat_Tendon,
    eMat_Rubber,
    eMat_Muscle,
    eMat_Fat,
    eMat_Max,
  };
  static const float MAT_COMPLIANCE[eMat_Max] = { // Miles Macklin's blog (http://blog.mmacklin.com/2016/10/12/xpbd-slides-and-stiffness/)
    0.00000000004f, // 0.04 x 10^(-9) (M^2/N) Concrete
    0.00000000016f, // 0.16 x 10^(-9) (M^2/N) Wood
    0.000000001f,   // 1.0  x 10^(-8) (M^2/N) Leather
    0.000000002f,   // 0.2  x 10^(-7) (M^2/N) Tendon
    0.0000001f,     // 1.0  x 10^(-6) (M^2/N) Rubber
    0.00002f,       // 0.2  x 10^(-3) (M^2/N) Muscle
    0.0001f,        // 1.0  x 10^(-3) (M^2/N) Fat
  };
};

namespace Cloth {
  Vec3       POS((Float)0.0, (Float)2.5, (Float)0.0);
  Vec2       WIDTH((Float)2.0, (Float)2.0);
  glm::ivec2 DIVISION(16, 16);
};

struct Material {
  GLfloat ambient[4];
  GLfloat diffuse[4];
  GLfloat specular[4];
  GLfloat shininess;
};

// emerald
Material mat_emerald = {
  { 0.0215f,  0.1745f,   0.0215f,  1.0f },
  { 0.07568f, 0.61424f,  0.07568f, 1.0f },
  { 0.633f,   0.727811f, 0.633f,   1.0f },
  76.8f
};

// jade
Material mat_jade = {
  { 0.135f,     0.2225f,   0.1575f,   1.0f },
  { 0.54f,      0.89f,     0.63f,     1.0f },
  { 0.316228f,  0.316228f, 0.316228f, 1.0f },
  12.8f
};

// obsidian
Material mat_obsidian = {
  { 0.05375f, 0.05f,      0.06625f,  1.0f },
  { 0.18275f, 0.17f,      0.22525f,  1.0f },
  { 0.332741f, 0.328634f, 0.346435f, 1.0f },
  38.4f
};

// pearl
Material mat_pearl = {
  { 0.25f,     0.20725f,  0.20725f,  1.0f },
  { 1.0f,      0.829f,    0.829f,    1.0f },
  { 0.296648f, 0.296648f, 0.296648f, 1.0f },
  10.24f
};

// ruby
Material mat_ruby  = {
  { 0.1745f,   0.01175f,  0.01175f,  1.0f },
  { 0.61424f,  0.04136f,  0.04136f,  1.0f },
  { 0.727811f, 0.626959f, 0.626959f, 1.0f },
  76.8f
};

// turquoise
Material mat_turquoise = {
  { 0.1f,      0.18725f, 0.1745f,   1.0f },
  { 0.396f,    0.74151f, 0.69102f,  1.0f },
  { 0.297254f, 0.30829f, 0.306678f, 1.0f },
  12.8f,
};

// brass
Material mat_brass = {
  { 0.329412f,  0.223529f, 0.027451f, 1.0f },
  { 0.780392f,  0.568627f, 0.113725f, 1.0f },
  { 0.992157f,  0.941176f, 0.807843f, 1.0f },
  27.89743616f,
};

// bronze
Material mat_bronze = {
  { 0.2125f,   0.1275f,   0.054f,    1.0f },
  { 0.714f,    0.4284f,   0.18144f,  1.0f },
  { 0.393548f, 0.271906f, 0.166721f, 1.0f },
  25.6f,
};

// chrome
Material mat_chrome = {
  { 0.25f,     0.25f,     0.25f,     1.0f },
  { 0.4f,      0.4f,      0.4f,      1.0f },
  { 0.774597f, 0.774597f, 0.774597f, 1.0f },
  76.8f,
};

// copper
Material mat_copper = {
  { 0.19125f,  0.0735f,   0.0225f,   1.0f },
  { 0.7038f,   0.27048f,  0.0828f,   1.0f },
  { 0.256777f, 0.137622f, 0.086014f, 1.0f },
  12.8f,
};

// gold
Material mat_gold = {
  { 0.24725f,  0.1995f,   0.0745f,    1.0f },
  { 0.75164f,  0.60648f,  0.22648f,   1.0f },
  { 0.628281f, 0.555802f, 0.366065f,  1.0f },
  51.2f,
};

// silver
Material mat_silver = {
  { 0.19225f,  0.19225f,  0.19225f,  1.0f },
  { 0.50754f,  0.50754f,  0.50754f,  1.0f },
  { 0.508273f, 0.508273f, 0.508273f, 1.0f },
  51.2f,
};

// plastic(black)
Material mat_plastic_black = {
  { 0.0f,  0.0f,  0.0f,  1.0f },
  { 0.01f, 0.01f, 0.01f, 1.0f },
  { 0.50f, 0.50f, 0.50f, 1.0f },
  32.0f,
};

// plastic(cyan)
Material mat_plastic_cyan = {
  { 0.0f, 0.1f,        0.06f,    1.0f },
  { 0.0f, 0.50980392f, 0.50980392f, 1.0f },
  { 0.50196078f, 0.50196078f, 0.50196078f, 1.0f },
  32.0f,
};

void set_material(Material& mat, GLenum face = GL_FRONT) {
  glMaterialfv(face, GL_AMBIENT,   mat.ambient);
  glMaterialfv(face, GL_DIFFUSE,   mat.diffuse);
  glMaterialfv(face, GL_SPECULAR,  mat.specular);
  glMaterialfv(face, GL_SHININESS, &mat.shininess);
}

void set_material(Material& in_mat, float alpha, GLenum face = GL_FRONT) {
  Material mat = in_mat;
  mat.ambient[3]   = alpha;
  mat.diffuse[3]   = alpha;
  mat.specular[3]  = alpha;
  set_material(mat, face);
}

struct DebugInfo {
  bool    show_depth;
  GLfloat dof;
  GLfloat focus;
  DebugInfo() : show_depth(false), dof(0.1f), focus(0.0f){}
};

struct Plane {
  glm::vec4 v;
};

struct Light {
  glm::vec4 v;
};

struct Shadow {
  float v[4][4];
};

struct Context;

class Scene {
public:
  enum {
    eCloth,
    eNum,
  };
  virtual void Update(Context& context, Float dt) = 0;
  virtual void Render(float alpha = 1.0f) = 0;
  virtual ~Scene() {}
};

struct Context {
  std::uint32_t       frame;
  float               time;
  DebugInfo           debug_info;
  Plane               floor;
  Light               light;
  Shadow              floor_shadow;
  Scene*              scene;
  int                 num_iteration;
  int                 mat_compliance;
  float               compliance;
  Context() : frame(0), time(0.0f), debug_info(), floor(), light(), floor_shadow(), scene(nullptr), num_iteration(20), mat_compliance(eMat_Fat), compliance((Float)MAT_COMPLIANCE[mat_compliance]) {}
};

class Point{
public:
  Float inv_mass;
  Vec3  position;
  Vec3  prev_position;
  Vec3  velocity;
  Point(Float inv_m, Vec3& pos, Vec3& vel) : inv_mass(inv_m), position(pos), prev_position(pos), velocity(vel) {
  };
  Point();
  void Predict(Float dt) {
    if (inv_mass < FLT_EPSILON) {
      return;
    }
    velocity  = position - prev_position;
    velocity.y -= GRAVITY * dt;
    prev_position = position;
    position += velocity * dt;
  }
  void SolveVelocity(GLfloat dt) {
    velocity  = position - prev_position;
    velocity *= inv_mass;
  }
  void Render(GLdouble radius, Material& mat, float alpha) {
    glPushMatrix();
      glTranslatef((GLfloat)position.x, (GLfloat)position.y, (GLfloat)position.z);
      set_material(mat, alpha);
      glutSolidSphere(radius, 16, 16);
    glPopMatrix();
  }
};

Context g_Context;

class DistanceConstraint {
public:
  Point* point0;
  Point* point1;
  Float  rest_length;
  Float  compliance;
  Float  lambda;
  DistanceConstraint(Point* p0, Point* p1, Float in_compliance) : point0(p0), point1(p1), rest_length((Float)0.0), compliance(in_compliance), lambda((Float)0.0) {
    rest_length = glm::length(point1->position - point0->position);
  }
  void LambdaInit() {
    lambda = 0.0f; // reset every time frame
  }
  void SolvePosition(Float dt){
    Float w = point0->inv_mass + point1->inv_mass;
    if (w < FLT_EPSILON) {
      return;
    }
    Vec3  grad = point0->position - point1->position;
    Float    d = glm::length(grad);
    compliance = g_Context.compliance;
    compliance /= dt * dt;    // a~
    GLfloat constraint = d- rest_length; // Cj(x)
    GLfloat dlambda    = (-constraint - (GLfloat)compliance * lambda) / (w + compliance); // eq.18
    Vec3    corr    = dlambda * grad / (d + FLT_EPSILON);                     // eq.17
    lambda += dlambda;
    point0->position += (corr * point0->inv_mass);
    point1->position -= (corr * point1->inv_mass);
  }
};

class DihedralConstraint {
public:
  DihedralConstraint() {
    
  }
  void SolvePos() {
    
  }
};

class VolumeConstraint {
  VolumeConstraint() {
    
  }
  void SolvePos() {
    
  }
};

class SceneCloth : public Scene {
private:
  glm::ivec2                      size;
  std::vector<Point>              points;
  std::vector<Vec3>               normals;
  std::vector<DistanceConstraint> constraints;
  Point* GetPoint(int w, int h)  {return &points[ h * size.x + w ]; }
  Vec3*  GetNormal(int w, int h) {return &normals[ h * size.x + w ]; }
  void   MakeConstraint(Point* p1, Point* p2, Float in_compliance) { constraints.push_back(DistanceConstraint(p1, p2, in_compliance)); }
  void   CalcNormal() {
    normals.clear();
    normals.shrink_to_fit();
    normals.resize(size.x * size.y);
    for(int w = 0; w < size.x - 1; w++){
      for(int h = 0; h < size.y - 1; h++){
        glm::vec3 v0 = glm::vec3(GetPoint(w,   h  )->position);
        glm::vec3 v1 = glm::vec3(GetPoint(w  , h+1)->position);
        glm::vec3 v2 = glm::vec3(GetPoint(w+1, h  )->position);
        glm::vec3 v3 = glm::vec3(GetPoint(w+1, h+1)->position);
        glm::vec3 f0 = glm::normalize(glm::cross(v2-v0, v1-v0));
        glm::vec3 f1 = glm::normalize(glm::cross(v3-v2, v1-v2));
        glm::vec3* n0 = GetNormal(w,   h  );
        glm::vec3* n1 = GetNormal(w,   h+1);
        glm::vec3* n2 = GetNormal(w+1, h  );
        glm::vec3* n3 = GetNormal(w+1, h+1);
        *n0 += f0;
        *n1 += f0;
        *n2 += f0;
        *n1 += f1;
        *n2 += f1;
        *n3 += f1;
      }
    }
    for(auto& n : normals) {
      n = glm::normalize(n);
    }
  }
  void   DrawTriangle(Point* p1, Point* p2, Point* p3, glm::vec3* n0, glm::vec3* n1, glm::vec3* n2){
    glm::vec3 v1(p1->position);
    glm::vec3 v2(p2->position);
    glm::vec3 v3(p3->position);
    glNormal3fv((GLfloat*)n0);
    glVertex3fv((GLfloat*)&v1);
    glNormal3fv((GLfloat*)n1);
    glVertex3fv((GLfloat*)&v2);
    glNormal3fv((GLfloat*)n2);
    glVertex3fv((GLfloat*)&v3);
  }
public:
  SceneCloth(Vec2& width, glm::ivec2& in_div, Vec3& in_pos, Float in_compliance) : size(in_div.x, in_div.y), points(), normals(), constraints() {
    points.reserve(size.x * size.y);
    for(int w = 0; w < size.x; w++){
      for(int h = 0; h < size.y; h++){
        Vec3 pos( width.x  * ((Float)w/(Float)(size.x-1)) - width.x  * 0.5f,
                  0.0f,
                  width.y * ((Float)h/(Float)(size.y-1)) + width.y * 0.5f );
        //Vec2 uv((Float)(w) / (size.x - 1), (Float)(h) / (size.y - 1));
        GLfloat inv_mass = 1.0f;
        Vec3 vel((Float)0.0, (Float)0.0, (Float)0.0);
        if ((h == 0) && (w == 0)          ||
            (h == 0) && (w == size.x - 1)) {
          inv_mass = 0.0f; // fix only edge point
        }
        pos += in_pos;
        points.push_back(Point(inv_mass, pos, vel));
      }
    }
    for(int w = 0; w < size.x; w++){
      for(int h = 0; h < size.y; h++){               // structual constraint
        if  (w < size.x - 1){ MakeConstraint(GetPoint(w, h), GetPoint(w+1, h  ), in_compliance); }
        if  (h < size.y - 1){ MakeConstraint(GetPoint(w, h), GetPoint(w,   h+1), in_compliance); }
        if ((w < size.x - 1) && (h < size.y - 1) ) { // shear constraint
          MakeConstraint(GetPoint(w,   h), GetPoint(w+1, h+1), in_compliance);
          MakeConstraint(GetPoint(w+1, h), GetPoint(w,   h+1), in_compliance);
        }
      }
    }
    for(int w = 0; w < size.x; w++){
      for(int h = 0; h < size.y; h++){               // bend constraint
        if  (w < size.x  - 2){ MakeConstraint(GetPoint(w, h), GetPoint(w+2, h  ), in_compliance); }
        if  (h < size.y  - 2){ MakeConstraint(GetPoint(w, h), GetPoint(w,   h+2), in_compliance); }
        if ((w < size.x  - 2) && (h < size.y - 2)) {
          MakeConstraint(GetPoint(w,   h), GetPoint(w+2, h+2), in_compliance);
          MakeConstraint(GetPoint(w+2, h), GetPoint(w,   h+2), in_compliance);
        }
      }
    }
    CalcNormal();
  }
  ~SceneCloth() {
    points.clear();
    points.shrink_to_fit();
    normals.clear();
    normals.shrink_to_fit();
    constraints.clear();
    constraints.shrink_to_fit();
  }
  virtual void Update(Context& ctx, Float dt) {
    for (auto& p : points) {
      p.Predict(dt);
    }
    for(auto& c : constraints) {
      c.LambdaInit();
    }
    for(int i = 0; i < ctx.num_iteration; i++) {
      for(auto& c : constraints) {
        c.SolvePosition(dt);
      }
    }
    CalcNormal();
  }
  virtual void Render(float alpha = 1.0f) {
    glFrontFace(GL_CW);
    glBegin(GL_TRIANGLES);
    int col_idx = 0;
    for(int w = 0; w < size.x - 1; w++){
      for(int h = 0; h < size.y - 1; h++){
        set_material(mat_emerald, alpha, GL_FRONT);
        set_material(mat_bronze,  alpha, GL_BACK);
        DrawTriangle(GetPoint (w,   h), GetPoint (w, h+1), GetPoint (w+1, h),
                     GetNormal(w,   h), GetNormal(w, h+1), GetNormal(w+1, h));
        DrawTriangle(GetPoint (w+1, h), GetPoint (w, h+1), GetPoint (w+1, h+1),
                     GetNormal(w+1, h), GetNormal(w, h+1), GetNormal(w+1, h+1));
      }
    }
    glEnd();
    glFrontFace(GL_CCW);
  }
};

void write_ppm(GLubyte* buff, GLenum format) {
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);
  char suffix[256];
  int  pix_sz = (format == GL_RGBA) ? 4 : 1;
  sprintf(suffix, (format == GL_RGBA) ? "screen.ppm" : "depth.ppm");
  char filename[1024];
  sprintf(filename, "%08d_%s", g_Context.frame, suffix);
  FILE *fp = fopen(filename, "wb");
  if (fp) {
    fprintf(fp, "P%d\n", (format == GL_RGBA) ? 6 : 5); // 5:Portable graymap(Binary), 6:Portable pixmap(Binary)
    fprintf(fp, "%u %u\n", w, h);
    fprintf(fp, "255\n");
    for(int y = 0; y < h; y++) {
      for(int x = 0; x < w; x++) {
        int index = (h - y - 1) * w * pix_sz + (x * pix_sz);
        if (format == GL_RGBA) {
          int r = buff[index];
          int g = buff[index + 1];
          int b = buff[index + 2];
          int a = buff[index + 3]; // not use here
          putc(r, fp); // binary
          putc(g, fp);
          putc(b, fp);
        } else {
          putc(buff[index], fp);
        }
      }
    }
    fclose(fp);
  }
}

void write_image(GLenum format) {
  GLsizei w = glutGet(GLUT_WINDOW_WIDTH);
  GLsizei h = glutGet(GLUT_WINDOW_HEIGHT);
  GLubyte* buff = (GLubyte*)malloc((size_t)w * (size_t)h * 4); // w x h * RGBA
  glReadBuffer(GL_BACK);
  glReadPixels(0, 0, w, h, format, GL_UNSIGNED_BYTE, buff);
  write_ppm(buff, format);
  free(buff);
}

void render_pipe(GLfloat width, GLfloat length, int slice, GLfloat color[]) {
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
  GLUquadricObj* q;
  q = gluNewQuadric();
  gluQuadricDrawStyle(q, GLU_FILL);
  gluQuadricNormals(q, GLU_SMOOTH);
  gluCylinder(q, width, width, length, slice, 1); // quad, base, top, height, slice, stacks
  glPushMatrix();
    glTranslatef(0.0f, 0.0f, length);
    gluDisk(q, 0.0f, width, slice, 1); // quad, inner, outer, slices, loops(top)
  glPopMatrix();
  gluQuadricOrientation(q, GLU_INSIDE);
  gluDisk(q, 0.0f, width, slice, 1); // quad, inner, outer, slices, loops(bottom)
  gluDeleteQuadric(q); 
}

void render_pipe(glm::vec3& start, glm::vec3& end, GLfloat width, int slice, GLfloat color[]) {
  glm::vec3 vec    = end - start;
  GLfloat   length = glm::length(vec);
  GLfloat   ax;
  if (fabs(vec.z) < FLT_EPSILON) {
    ax = 57.2957795f * acos( vec.x / length ); // rotation angle in x-y plane
    if (vec.y <= 0.0f)
      ax = -ax;
  } else {
    ax = 57.2957795f * acos( vec.z / length ); // rotation angle
    if (vec.z <= 0.0f)
      ax = -ax;
  }
  GLfloat rx = -vec.y * vec.z;
  GLfloat ry =  vec.x * vec.z;
  glPushMatrix();
    glTranslatef(start.x, start.y, start.z);
    if (fabs(vec.z) < FLT_EPSILON) {
      glRotatef(90.0f,  0.0f, 1.0f, 0.0f); // Rotate & align with x axis
      glRotatef(   ax, -1.0f, 0.0f, 0.0f); // Rotate to point 2 in x-y plane
    } else {
      glRotatef(   ax,   rx,    ry, 0.0f); // Rotate about rotation vector
    }
    render_pipe(width, length, slice, color);
  glPopMatrix();
}

void render_arrow(glm::vec3& start, glm::vec3& end, GLfloat width, int slice, GLfloat height, GLfloat color[]) {
  render_pipe(start, end, width, slice, color);
  glm::vec3 vec        = end - start;
  float     vec_length = glm::length(vec);
  if (vec_length > FLT_MIN) {
    static const glm::vec3 init(0.0f, 0.0f, 1.0f); // glutSolidCone() +z
    glm::vec3 normalized_vec = glm::normalize(vec);
    glm::vec3 diff = normalized_vec - init;
    if (glm::length(diff) > FLT_MIN) {
      glm::vec3 rot_axis  = glm::normalize(glm::cross(init, normalized_vec));
      float     rot_angle = acos(glm::dot(init, normalized_vec)) * 57.295f; // 360.0f / (2.0f * 3.14f)
      glm::vec3 cone_pos = end;
      if (vec_length > height){
        cone_pos = start + (vec_length - height) * normalized_vec; // offset cone height
        glPushMatrix();
          glTranslatef(cone_pos.x, cone_pos.y, cone_pos.z);
          glRotatef(rot_angle, rot_axis.x, rot_axis.y, rot_axis.z);
          glutSolidCone(height * 0.25, height, 4, 4); // base, height, slices, stacks
        glPopMatrix();
      }
    }
  }
}

void render_floor(GLfloat w, GLfloat d, int num_w, int num_d) {
  static const GLfloat color[][4] = { { 0.6f, 0.6f, 0.6f, 1.0f },   // white
                                      { 0.3f, 0.3f, 0.3f, 1.0f } }; // gray
  GLfloat center_w = (w * num_w) / 2.0f;
  GLfloat center_d = (d * num_d) / 2.0f;
  glPushMatrix();
    glNormal3f(0.0f, 1.0f, 0.0f); // up vector
    glBegin(GL_QUADS);
    for (int j = 0; j < num_d; ++j) {
      GLfloat dj  = d  * j;
      GLfloat djd = dj + d;
      for (int i = 0; i < num_w; ++i) {
        GLfloat wi  = w  * i;
        GLfloat wiw = wi + w;
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color[(i + j) & 1]);
        glVertex3f(wi  - center_w,  0.0, dj  - center_d);
        glVertex3f(wi  - center_w,  0.0, djd - center_d);
        glVertex3f(wiw - center_w,  0.0, djd - center_d);
        glVertex3f(wiw - center_w,  0.0, dj  - center_d);
      }
    }
    glEnd();
  glPopMatrix();
}

void init_imgui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplGLUT_Init();
  ImGui_ImplGLUT_InstallFuncs();
  ImGui_ImplOpenGL2_Init();
}

void finalize_imgui() {
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGLUT_Shutdown();
  ImGui::DestroyContext();
}

void finalize(void) {
  finalize_imgui();
  return;
}

void find_plane(Plane* plane, glm::vec3& v0, glm::vec3& v1, glm::vec3& v2) {
  glm::vec3 vec0 = v1 - v0;
  glm::vec3 vec1 = v2 - v0;
  plane->v[0] =   vec0.y * vec1.z - vec0.z * vec1.y;
  plane->v[1] = -(vec0.x * vec1.z - vec0.z * vec1.x);
  plane->v[2] =   vec0.x * vec1.y - vec0.y * vec1.x;
  plane->v[3] = -(plane->v[0] * v0.x + plane->v[1] * v0.y + plane->v[2] * v0.z);
}

void calc_shadow_matrix(Shadow* shadow, Plane& plane, Light& light) {
  GLfloat dot = glm::dot(plane.v, light.v);
  shadow->v[0][0] = dot - light.v[0] * plane.v[0];
  shadow->v[1][0] = 0.f - light.v[0] * plane.v[1];
  shadow->v[2][0] = 0.f - light.v[0] * plane.v[2];
  shadow->v[3][0] = 0.f - light.v[0] * plane.v[3];

  shadow->v[0][1] = 0.f - light.v[1] * plane.v[0];
  shadow->v[1][1] = dot - light.v[1] * plane.v[1];
  shadow->v[2][1] = 0.f - light.v[1] * plane.v[2];
  shadow->v[3][1] = 0.f - light.v[1] * plane.v[3];

  shadow->v[0][2] = 0.f - light.v[2] * plane.v[0];
  shadow->v[1][2] = 0.f - light.v[2] * plane.v[1];
  shadow->v[2][2] = dot - light.v[2] * plane.v[2];
  shadow->v[3][2] = 0.f - light.v[2] * plane.v[3];

  shadow->v[0][3] = 0.f - light.v[3] * plane.v[0];
  shadow->v[1][3] = 0.f - light.v[3] * plane.v[1];
  shadow->v[2][3] = 0.f - light.v[3] * plane.v[2];
  shadow->v[3][3] = dot - light.v[3] * plane.v[3];
}

void initialize(int argc, char* argv[]) {
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glClearAccum(0.0f, 0.0f, 0.0f, 0.0f); 
  glClearDepth(1.0);
  glDepthFunc(GL_LESS);

  glEnable(GL_DEPTH_TEST);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  //glEnable(GL_LIGHT1);

  GLfloat ambient[] = { 0.0, 0.0, 0.0, 1.0 };
  GLfloat diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat lmodel_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
  GLfloat local_view[] = { 0.0 };

  glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, specular);

  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, local_view);

//  GLfloat light0pos[] = { 0.0, 5.0, 10.0, 1.0 };
  GLfloat light0pos[] = { 15.0, 15.0, 15.0, 0.0 };
//  static const GLfloat light1pos[] = { 5.0, 3.0, 0.0, 1.0 };
  glLightfv(GL_LIGHT0, GL_POSITION, light0pos);
  g_Context.light.v[0] = light0pos[0];
  g_Context.light.v[1] = light0pos[1];
  g_Context.light.v[2] = light0pos[2];
  g_Context.light.v[3] = light0pos[3];

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_LINE_SMOOTH);
  //glEnable(GL_POLYGON_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
//  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_AUTO_NORMAL);
  glEnable(GL_NORMALIZE);

  init_imgui();
  atexit(finalize);

  glm::vec3 v0(-1.0f, 0.0f,  0.0f);
  glm::vec3 v1(+1.0f, 0.0f,  0.0f);
  glm::vec3 v2(+1.0f, 0.0f, -1.0f);
  find_plane(&g_Context.floor, v0, v1, v2);
  g_Context.scene = new SceneCloth(Cloth::WIDTH, Cloth::DIVISION, Cloth::POS, g_Context.compliance);
}

void restart() {
  if (g_Context.scene) {
    delete g_Context.scene;
    g_Context.scene = nullptr;
  }
  g_Context.scene = new SceneCloth(Cloth::WIDTH, Cloth::DIVISION, Cloth::POS, g_Context.compliance);
}

void display_imgui() {
  ImGui_ImplOpenGL2_NewFrame();
  ImGui_ImplGLUT_NewFrame();

  {
    ImGui::SetNextWindowPos(ImVec2(  10,  10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(270, 130), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug");
    ImGui::Checkbox("Show Depth", &g_Context.debug_info.show_depth);
    ImGui::SliderFloat("DoF",     &g_Context.debug_info.dof,     0.0f,  0.2f);
    ImGui::SliderFloat("focus",   &g_Context.debug_info.focus, -15.0f, 11.5f);
    ImGui::End();

    ImGui::Begin("Params");
    if (ImGui::Button("Restart")) {
      restart();
    }
    ImGui::SliderInt  ("Iterations", &g_Context.num_iteration, 20, 160);
    if (ImGui::Combo("Material", &g_Context.mat_compliance, "Concrete\0Wood\0Leather\0Tendon\0Rubber\0Muscle\0Fat\0")) {
      g_Context.compliance = MAT_COMPLIANCE[g_Context.mat_compliance];
      restart();
    }
    ImGui::Text("Compliance: %0.12f", g_Context.compliance);
    ImGui::End();
  }

  ImGui::Render();
  ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

void display_axis() {
  glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
  glm::vec3 left( -4.0f, 0.1f, 0.0f);
  glm::vec3 right(+4.0f, 0.1f, 0.0f);
  glm::vec3 bottm( 0.0f, -3.0f, 0.0f);
  glm::vec3 top(   0.0f, +3.0f, 0.0f);
  glm::vec3 back(  0.0f, 0.1f, -3.0f);
  glm::vec3 front( 0.0f, 0.1f, +3.0f);
  GLfloat red[]   = { 0.8f, 0.0f, 0.0f, 1.0f };
  GLfloat green[] = { 0.0f, 0.8f, 0.0f, 1.0f };
  GLfloat blue[]  = { 0.0f, 0.0f, 0.8f, 1.0f };
  render_arrow(left,  right, 0.01f, 8, 0.3f, red);
  render_arrow(bottm, top,   0.01f, 8, 0.3f, green);
  render_arrow(back,  front, 0.01f, 8, 0.3f, blue);
}

void display_depth() {
  if (g_Context.debug_info.show_depth == false) { return; }
  GLint view[4];
  GLubyte *buffer;
  glGetIntegerv(GL_VIEWPORT, view);
  buffer = (GLubyte *)malloc(size_t(view[2]) * size_t(view[3]));
  glReadPixels(view[0], view[1], view[2], view[3], GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, buffer);
  glDisable(GL_DEPTH_TEST);
    glDrawPixels(view[2], view[3], GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);
  glEnable(GL_DEPTH_TEST);
  free(buffer);
}

void render_scene(float alpha = 1.0f) {
#if USE_TEST_SCENE
  Material mat[] = {
    mat_emerald, mat_jade, mat_obsidian, mat_pearl, mat_ruby, mat_turquoise, mat_brass, mat_bronze
  };
  for(int i = 0; i < 8; i++) {
    set_material(mat[i], alpha);
    glPushMatrix();
      glTranslatef(-6.0f + 1.0f * (float)i, 0.4f, -3.0f + 2.0f * (float)i);
      float ang = (float)(g_Context.frame % 120) * 3.0f;
      glRotatef(ang, 0.0f, 1.0f, 0.0f);
      glFrontFace(GL_CW);
      glutSolidTeapot(0.5f);
      glFrontFace(GL_CCW);
    glPopMatrix();
  }
#else
  if (g_Context.scene) {
    g_Context.scene->Render(alpha);
  }
#endif
}

void display_actor(float alpha = 1.0f) {
  display_axis();
  render_floor(1.0f, 1.0f, 24, 32); // actual floor

  glDisable(GL_DEPTH_TEST);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glEnable(GL_STENCIL_TEST);
  glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
  glStencilFunc(GL_ALWAYS, 1, 0xffffffff);
  render_floor(1.0f, 1.0f, 24, 32);       // floor pixels just get their stencil set to 1. 
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glEnable(GL_DEPTH_TEST);

  glStencilFunc(GL_EQUAL, 1, 0xffffffff); // draw if ==1
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  glDisable(GL_DEPTH_TEST);
//  glEnable(GL_CULL_FACE);
//  glCullFace(GL_FRONT);
  glPushMatrix();
    glScalef(1.0f, -1.0f, 1.0f); // for reflection on plane(y=0.0f)
    glLightfv(GL_LIGHT0, GL_POSITION, &g_Context.light.v[0]);
    render_scene(0.25f);          // reflection
  glPopMatrix();
  glLightfv(GL_LIGHT0, GL_POSITION, &g_Context.light.v[0]);

//  glDisable(GL_CULL_FACE);
//  glCullFace(GL_BACK);

  calc_shadow_matrix(&g_Context.floor_shadow, g_Context.floor, g_Context.light);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glDisable(GL_LIGHTING);        // force the 50% black
  glColor4f(0.0, 0.0, 0.0, 0.5);
  glPushMatrix();
    glMultMatrixf((GLfloat*)g_Context.floor_shadow.v);
    render_scene();               // projected shadow
  glPopMatrix();
  glEnable(GL_LIGHTING);
  glDisable(GL_POLYGON_OFFSET_FILL);
  glDisable(GL_STENCIL_TEST);

  glEnable(GL_DEPTH_TEST);
  render_scene();                 // actual draw
}

void display(void){
  glClear(GL_ACCUM_BUFFER_BIT);
  int   num_accum = 8;
  struct jitter_point{ GLfloat x, y; };
  jitter_point j8[] = {
    {-0.334818f,  0.435331f},
    { 0.286438f, -0.393495f},
    { 0.459462f,  0.141540f},
    {-0.414498f, -0.192829f},
    {-0.183790f,  0.082102f},
    {-0.079263f, -0.317383f},
    { 0.102254f,  0.299133f},
    { 0.164216f, -0.054399f}
  };
  GLint viewport[4];
  glGetIntegerv (GL_VIEWPORT, viewport);
  for(int i = 0 ; i < num_accum; i++) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glm::vec3 pos(0.0f, 1.6f, 12.0f);
    float eye_jitter = (pos.z - g_Context.debug_info.focus) / pos.z;
    eye_jitter = (eye_jitter < 0.1f) ? 0.1f : eye_jitter;
    pos.x += g_Context.debug_info.dof * j8[i].x * eye_jitter;
    pos.y += g_Context.debug_info.dof * j8[i].y * eye_jitter;
    glm::vec3 tgt(0.0f, 0.0f, 0.0f);
    //glm::vec3      tgt(0.0f, 3.0f, 0.0f);
    glm::vec3 vec = tgt - pos;
    tgt.y = pos.y + vec.y * ((pos.z - g_Context.debug_info.focus) / pos.z);
    tgt.z = g_Context.debug_info.focus;
    gluLookAt(pos.x, pos.y, pos.z, tgt.x, tgt.y, tgt.z, 0.0, 1.0, 0.0); // pos, tgt, up

    display_actor();

    glAccum(GL_ACCUM, 1.0f / num_accum);
  }
  glAccum(GL_RETURN, 1.0f);

  display_depth();
  display_imgui();

  glutSwapBuffers();
  glutPostRedisplay();
}

void reshape_imgui(int width, int height) {
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize.x = (float)width;
  io.DisplaySize.y = (float)height;
}

void reshape(int width, int height){
//  static const GLfloat light0pos[] = { 0.0, 5.0, 10.0, 1.0 };
//  static const GLfloat light1pos[] = { 5.0, 3.0, 0.0, 1.0 };
  glShadeModel(GL_SMOOTH);

  reshape_imgui(width, height);
  glViewport(0, 0, width, height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(30.0, (double)width / (double)height, 1.0f, 100.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

//  gluLookAt(0.0, 1.6, 15.0f, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0); // pos, tgt, up
//  glLightfv(GL_LIGHT0, GL_POSITION, light0pos);
//  glLightfv(GL_LIGHT1, GL_POSITION, light1pos);
}

void keyboard(unsigned char key, int x , int y){
  switch(key) {
  case 's': write_image(GL_RGBA); break;
  case 'd': write_image(GL_DEPTH_COMPONENT); break;
  case 27: exit(0); break; // esc
  }
}

void idle(void){
  GLfloat time = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
  GLfloat dt   = (GLfloat)FIXED_DT;//time - g_Context.time;
  auto&   ctx  = g_Context;
  ctx.time = time;
  if (ctx.scene) {
    ctx.scene->Update(ctx, dt);
  }
#if USE_CAPTURE
  keyboard('s', 0, 0); // screenshot
#endif
  while(1) {
    if (((float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f - time) > FIXED_DT) {
      break; // keep 60fps
    }
  }
  ctx.frame++;
}

void mouse( int button, int state, int x, int y ){
  if (button == GLUT_LEFT_BUTTON) {
    switch(state){
    case GLUT_DOWN:
      break;
    case GLUT_UP:
      break;
    }
  }
}

void motion(int x, int y){

}

int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
#ifdef __FREEGLUT_EXT_H__
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif
  //glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_ACCUM | GLUT_STENCIL);
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_SINGLE | GLUT_ACCUM | GLUT_STENCIL);
  glutInitWindowSize(640, 480);
  glutCreateWindow("Position Based Dynamics");

  initialize(argc, argv);

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(idle);

  //glutMouseFunc(mouse);     // ImGui_ImplGLUT_MouseFunc
  //glutMotionFunc(motion);   // ImGui_ImplGLUT_MotionFunc
  //glutKeyboardFunc(keyboard); // ImGui_ImplGLUT_KeyboardFunc

  glutMainLoop();
  return 0;
}
