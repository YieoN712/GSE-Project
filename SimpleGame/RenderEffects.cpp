#include "RenderEffects.h"
#include <iostream>
#include <algorithm>

namespace {
GLuint Program(const char* vs,const char* fs) {
    GLuint program=glCreateProgram();
    for(int i=0;i<2;++i) {
        GLuint s=glCreateShader(i==0?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER);
        const char* code=i==0?vs:fs; glShaderSource(s,1,&code,nullptr);glCompileShader(s);
        GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
        if(!ok) {char log[2048]{};glGetShaderInfoLog(s,2048,nullptr,log);std::cerr<<log<<"\n";glDeleteShader(s);glDeleteProgram(program);return 0;}
        glAttachShader(program,s);glDeleteShader(s);
    }
    glLinkProgram(program);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
    if(!ok) {char log[2048]{};glGetProgramInfoLog(program,2048,nullptr,log);std::cerr<<log<<"\n";glDeleteProgram(program);return 0;}
    return program;
}
const char* worldVS=R"GLSL(#version 120
varying vec3 world, normal;
varying vec4 tint, shadow;
uniform mat4 lightMatrix;
void main() {
 world=gl_Vertex.xyz; normal=gl_Normal; tint=gl_Color;
 shadow=lightMatrix*gl_Vertex;
 gl_Position=ftransform();
})GLSL";
const char* worldFS=R"GLSL(#version 120
varying vec3 world, normal;
varying vec4 tint, shadow;
uniform sampler2D shadowMap;
uniform int material;
uniform float shadowEnabled;
float hash(vec2 p) {return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
void main() {
 vec3 n=normalize(normal); vec3 color=tint.rgb;
 vec2 uv=abs(n.y)>.5?world.xz:(abs(n.x)>.5?world.zy:world.xy);
 float grain=hash(floor(uv*80.0));
 float roughness=.85;
 if(material==1) { // grass: fine blades and broad, nonuniform patches
   float patch=sin(uv.x*.85)*sin(uv.y*.71);
   color*=.85+.16*grain+.10*patch;
   color+=vec3(.025,.045,-.01)*step(.91,hash(floor(uv*16.0)));
 } else if(material==2) { // fine aggregate asphalt
   color*=.78+.30*grain;
   color+=vec3(.045)*step(.975,grain); roughness=.95;
 } else if(material==3) { // concrete paving joints
   vec2 tile=fract(uv*1.5);
   float joint=step(.035,tile.x)*step(.035,tile.y);
   color*=mix(.63,.95+.08*grain,joint);
 } else if(material==4) { // modern brick/stucco facade
   vec2 brick=vec2(uv.x*2.0+mod(floor(uv.y*4.0),2.0)*.5,uv.y*4.0);
   vec2 f=fract(brick);
   color*=mix(.70,.92+.13*hash(floor(brick)),step(.04,f.x)*step(.07,f.y));
 } else if(material==5) { // standing-seam roof
   color*=.87+.10*grain;
   color*=mix(.62,1.0,step(.055,fract(uv.x*2.3))); roughness=.35;
 } else if(material==6) { // blue glass, restrained sky reflection
   color=mix(color,vec3(.66,.82,.85),.22+.16*sin(uv.y*1.8));roughness=.12;
 } else if(material==7) {
   color*=.86+.12*sin(uv.x*21.0+sin(uv.y*3.0))+.05*grain;
 } else if(material==8) {color*=.96+.06*grain;}
 vec3 sun=normalize(vec3(-.45,.82,-.35));
 float diffuse=max(dot(n,sun),0.0);
 vec3 p=shadow.xyz/shadow.w*.5+.5;
 float shade=0.0;
 if(shadowEnabled>.5 && p.x>.002 && p.x<.998 && p.y>.002 && p.y<.998 && p.z>0.0 && p.z<1.0) {
   float bias=max(.00035,.0015*(1.0-diffuse));
   for(int x=-2;x<=2;++x) for(int y=-2;y<=2;++y) {
     float depth=texture2D(shadowMap,p.xy+vec2(float(x),float(y))/2048.0).r;
     shade+=(p.z-bias>depth)?1.0:0.0;
   }
   shade/=25.0;
 }
 vec3 ambient=vec3(.66,.72,.79)*(.75+.13*max(n.y,0.0));
 vec3 lit=ambient+vec3(1.0,.91,.73)*diffuse*(1.0-shade*.80)*.58;
 vec3 halfway=normalize(sun+normalize(vec3(.57,.57,.57)));
 float spec=pow(max(dot(n,halfway),0.0),mix(70.0,8.0,roughness))*(1.0-roughness)*.22*(1.0-shade);
 gl_FragColor=vec4(color*lit+spec,tint.a);
})GLSL";
const char* postVS=R"GLSL(#version 120
varying vec2 uv;
void main(){uv=gl_MultiTexCoord0.xy;gl_Position=gl_Vertex;}
)GLSL";
const char* postFS=R"GLSL(#version 120
varying vec2 uv;
uniform sampler2D scene;
uniform vec2 texel;
float luma(vec3 c){return dot(c,vec3(.299,.587,.114));}
void main(){
 vec3 c=texture2D(scene,uv).rgb;
 vec3 a=texture2D(scene,uv+vec2(texel.x,0)).rgb;
 vec3 b=texture2D(scene,uv-vec2(texel.x,0)).rgb;
 vec3 d=texture2D(scene,uv+vec2(0,texel.y)).rgb;
 vec3 e=texture2D(scene,uv-vec2(0,texel.y)).rgb;
 float edge=max(max(luma(a),luma(b)),max(luma(d),luma(e)))-min(min(luma(a),luma(b)),min(luma(d),luma(e)));
 c=mix(c,(a+b+d+e+c*4.0)/8.0,smoothstep(.08,.28,edge)*.55);
 vec3 bloom=vec3(0);
 for(int x=-2;x<=2;++x) for(int y=-2;y<=2;++y)
   bloom+=max(texture2D(scene,uv+vec2(float(x),float(y))*texel*3.0).rgb-.82,0.0);
 c+=bloom/25.0*.22;
 c=(c-.5)*1.035+.5;
 c*=vec3(1.025,1.005,.973);
 vec2 v=uv*2.0-1.0;c*=1.0-.09*dot(v,v);
 gl_FragColor=vec4(clamp(c,0.0,1.0),1);
})GLSL";
}
RenderEffects::RenderEffects() {
    if(!GLEW_VERSION_2_0 || !(GLEW_VERSION_3_0 || GLEW_ARB_framebuffer_object)) {
        std::cerr<<"Quality effects unavailable: using compatibility renderer.\n";return;
    }
    sceneProgram=Program(worldVS,worldFS);postProgram=Program(postVS,postFS);
    if(!sceneProgram || !postProgram) return;
    materialLocation=glGetUniformLocation(sceneProgram,"material");
    glGenTextures(1,&shadowTexture);glBindTexture(GL_TEXTURE_2D,shadowTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,2048,2048,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1,&shadowFbo);glBindFramebuffer(GL_FRAMEBUFFER,shadowFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,shadowTexture,0);
    glDrawBuffer(GL_NONE);glReadBuffer(GL_NONE);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) {
        std::cerr<<"Shadow framebuffer unavailable.\n";glDeleteFramebuffers(1,&shadowFbo);shadowFbo=0;
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);glDrawBuffer(GL_BACK);glReadBuffer(GL_BACK);
    glBindTexture(GL_TEXTURE_2D,0);enabled=true;
}
void RenderEffects::ReleaseScene() {
    if(sceneFbo)glDeleteFramebuffers(1,&sceneFbo);
    if(sceneTexture)glDeleteTextures(1,&sceneTexture);
    if(depthBuffer)glDeleteRenderbuffers(1,&depthBuffer);
    sceneFbo=sceneTexture=depthBuffer=0;
}
RenderEffects::~RenderEffects() {
    ReleaseScene();if(shadowFbo)glDeleteFramebuffers(1,&shadowFbo);
    if(shadowTexture)glDeleteTextures(1,&shadowTexture);
    if(sceneProgram)glDeleteProgram(sceneProgram);if(postProgram)glDeleteProgram(postProgram);
}
void RenderEffects::Resize(int width,int height) {
    if(w==width && h==height) return;
    ReleaseScene();w=width;h=height;
    glGenTextures(1,&sceneTexture);glBindTexture(GL_TEXTURE_2D,sceneTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glGenRenderbuffers(1,&depthBuffer);glBindRenderbuffer(GL_RENDERBUFFER,depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,w,h);
    glGenFramebuffers(1,&sceneFbo);glBindFramebuffer(GL_FRAMEBUFFER,sceneFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,sceneTexture,0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depthBuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);glReadBuffer(GL_COLOR_ATTACHMENT0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) {
        std::cerr<<"Scene framebuffer unavailable: postprocessing disabled.\n";ReleaseScene();
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);glBindTexture(GL_TEXTURE_2D,0);
}
bool RenderEffects::BeginShadow(float x,float z) {
    if(!enabled || !shadowFbo)return false;
    shadowPass=true;glUseProgram(0);glBindFramebuffer(GL_FRAMEBUFFER,shadowFbo);
    glViewport(0,0,2048,2048);glClear(GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);
    glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(2,4);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(-48,48,-48,48,-100,100);
    // Save projection * view directly without a separate math dependency.
    glRotatef(55,1,0,0);glRotatef(128,0,1,0);glTranslatef(-x,0,-z);
    glGetFloatv(GL_PROJECTION_MATRIX,lightMatrix);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();return true;
}
void RenderEffects::BeginScene(int width,int height) {
    shadowPass=false;glDisable(GL_POLYGON_OFFSET_FILL);
    if(enabled)Resize(width,height);
    sceneActive=enabled && sceneFbo;
    if(enabled)glBindFramebuffer(GL_FRAMEBUFFER,sceneActive?sceneFbo:0);
    glViewport(0,0,width,height);glEnable(GL_DEPTH_TEST);
    if(enabled) {
        glUseProgram(sceneProgram);glUniformMatrix4fv(glGetUniformLocation(sceneProgram,"lightMatrix"),1,GL_FALSE,lightMatrix);
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,shadowTexture);
        glUniform1i(glGetUniformLocation(sceneProgram,"shadowMap"),0);
        glUniform1f(glGetUniformLocation(sceneProgram,"shadowEnabled"),shadowFbo?1.f:0.f);
    }
}
void RenderEffects::Material(int m) { if(enabled && !shadowPass)glUniform1i(materialLocation,m); }
void RenderEffects::Unlit() { glUseProgram(0);glBindTexture(GL_TEXTURE_2D,0); }
void RenderEffects::Composite() {
    Unlit();if(!sceneActive)return;
    glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,w,h);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
    glUseProgram(postProgram);glBindTexture(GL_TEXTURE_2D,sceneTexture);
    glUniform1i(glGetUniformLocation(postProgram,"scene"),0);
    glUniform2f(glGetUniformLocation(postProgram,"texel"),1.f/w,1.f/h);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex2f(-1,-1);glTexCoord2f(1,0);glVertex2f(1,-1);
    glTexCoord2f(1,1);glVertex2f(1,1);glTexCoord2f(0,1);glVertex2f(-1,1);glEnd();
    Unlit();glEnable(GL_BLEND);
}
