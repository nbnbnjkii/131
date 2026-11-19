#include "Renderer.h"
#include "ObjectMesh.h"
#include "CityGraffiti.h"
#include "GravityBeam.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <jni.h>
#include <android/log.h>
#include <mutex>
#include <vector>
#include <optional>
#include <cstddef>
#include "MeshGeometry.h"
#include "LevelPlacement.h"
#include "DebugPlayer.h"
#include <atomic>
#include <fstream>
#include <android/asset_manager.h>

namespace {
std::mutex commandMutex;
std::vector<int> commands;
float lookDx=0,lookDy=0;
// Retain transient controls and camera when Android recreates only the EGL surface.
// Accessed exclusively by the native game thread, never the Java UI thread.
std::optional<sandbox::World> retainedWorld;
std::optional<sandbox::FpvDrone> retainedDrone;
sandbox::HandTool retainedTool=sandbox::HandTool::HAND_EMPTY;
bool retainedFlying=false;
std::atomic<int> loadStage{0},debugNotice{0},loadPercent{0};
std::atomic<float> verticalInput{0};
std::atomic<float> stickRight{0},stickForward{0};
std::atomic<bool> gameplayReady{false};
std::atomic<int> currentTool{0},currentSelection{0},currentSurface{-1},interactionAction{0},freezeState{0};
std::atomic<int> droneMode{0};
std::atomic<bool> droneExplodedFlag{false};
std::atomic<float> droneThrottle{0},droneYawInput{0},dronePitchInput{0},droneRollInput{0};
std::atomic<bool> holdingObject{false};
std::mutex snapshotMutex;
std::array<float,4> snapshot{0,1.7f,0,0};
struct LoadGuard {bool complete=false;~LoadGuard(){if(!complete)loadStage=-1;}};
city::SpawnConfig readSpawnConfig(android_app *app){
 city::SpawnConfig config;std::string text,error;
 std::ifstream file(std::string(app->activity->internalDataPath)+"/spawn.json",std::ios::binary|std::ios::ate);
 if(file&&file.tellg()>0&&file.tellg()<=65536){file.seekg(0);text.assign(std::istreambuf_iterator<char>(file),{});}
 else if(auto *asset=AAssetManager_open(app->activity->assetManager,"config/spawn.json",AASSET_MODE_BUFFER)){
  auto size=AAsset_getLength(asset);if(size>0&&size<=65536){text.resize(size);if(AAsset_read(asset,text.data(),size)!=size)text.clear();}AAsset_close(asset);
 }
 if(!text.empty()&&!city::parseSpawnConfig(text,config,error))__android_log_print(ANDROID_LOG_WARN,"Ekron","Invalid spawn config; automatic spawn: %s",error.c_str());
 return config;
}
const char *vertex=R"(#version 300 es
layout(location=0) in vec3 position;
layout(location=1) in vec3 color;
layout(location=2) in vec3 normal;
layout(location=3) in vec2 uv;
layout(location=4) in float material;
uniform mat4 objectTransform;
uniform vec3 eye;
uniform vec2 angles;
uniform float cameraRoll;
uniform float aspect;
out vec3 tint;
out vec3 worldNormal;
out vec2 texUV;
flat out int layer;
out vec3 toEye;
void main(){
 vec3 p=(objectTransform*vec4(position,1.0)).xyz-eye;
 float cy=cos(angles.x),sy=sin(angles.x),cp=cos(angles.y),sp=sin(angles.y);
 vec3 q=vec3(cy*p.x-sy*p.z,p.y,sy*p.x+cy*p.z);
 q=vec3(q.x,cp*q.y+sp*q.z,-sp*q.y+cp*q.z);
 q.xy=mat2(cos(cameraRoll),sin(cameraRoll),-sin(cameraRoll),cos(cameraRoll))*q.xy;
 float f=1.37638;
 gl_Position=vec4(q.x*f/aspect,q.y*f,-1.001334*q.z-.200134,-q.z);
 tint=color;worldNormal=mat3(objectTransform)*normal;texUV=uv;layer=int(material+.5);toEye=p;
})";
const char *fragment=R"(#version 300 es
precision highp float;
precision highp sampler2DArray;
precision highp sampler2D;
precision highp int;
in vec3 tint;
in vec3 worldNormal;
in vec2 texUV;
flat in int layer;
in vec3 toEye;
uniform sampler2DArray albedo;
uniform bool texturesReady;
uniform bool modelPass;
uniform bool previewPass;
uniform bool modelTextured;
uniform sampler2D modelAlbedo;
uniform vec4 baseColorFactor;
uniform int alphaMode;
uniform float alphaCutoff;
uniform vec3 eye;
uniform float modelRoughness;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 ambientColor;
out vec4 outColor;
void main(){
 vec3 base=pow(tint,vec3(2.2));
 if(texturesReady)base*=texture(albedo,vec3(texUV,float(layer))).rgb;
 if(!modelPass && layer!=4){
  vec3 world=toEye+eye;
  base*=.94+.035*sin(world.x*.21+world.z*.17)+.025*sin(world.y*.33-world.x*.13);
 }
 // Screen-door transparency writes depth only for the retained glass samples;
 // no object-order-dependent blending or opaque blue pane.
 if(!modelPass && layer==7 && mod(floor(gl_FragCoord.x)+floor(gl_FragCoord.y)*2.0,4.0)>0.0)discard;
 float alpha=1.0;
 if(modelPass){
  vec4 sampleColor=modelTextured?texture(modelAlbedo,texUV):vec4(1.0);
  base=baseColorFactor.rgb*sampleColor.rgb;alpha=baseColorFactor.a*sampleColor.a;
  if(alphaMode==1&&alpha<alphaCutoff)discard;
  if(alphaMode!=2)alpha=1.0;
 }
 if(previewPass){base=mix(base,base*vec3(.35,1.25,.55)+vec3(.01,.035,.015),.32);alpha*=.62;}
 vec3 n=normalize(worldNormal);
 if(modelPass&&!gl_FrontFacing)n=-n;
 float diffuse=max(dot(n,normalize(-lightDirection)),0.0);
 // Broad, cool ambient light keeps overcast facades readable without hard shadows.
 float sky=.78+.22*max(n.y,0.0);
 vec3 lit=base*(ambientColor*sky+lightColor*diffuse);
 float roughness=modelPass?modelRoughness:(layer==7?.18:layer==3?.48:.94);
 float strength=modelPass?.035:(layer==7?.28:layer==3?.12:.006);
 vec3 halfVector=normalize(normalize(-lightDirection)-normalize(toEye));
 float shine=pow(max(dot(n,halfVector),0.0),mix(90.0,8.0,roughness));
 lit+=lightColor*shine*strength*diffuse;
 float d=max(length(toEye)-22.0,0.0);
 float fog=1.0-exp(-d*d*.00022);
 vec3 fogColor=pow(vec3(.56,.61,.63),vec3(2.2));
 outColor=vec4(pow(max(mix(lit,fogColor,fog),vec3(0.0)),vec3(1.0/2.2)),alpha);
})";
const char *skyVertex=R"(#version 300 es
out vec2 screen;
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));screen=p*2.0-1.0;gl_Position=vec4(screen,0.0,1.0);})";
const char *skyFragment=R"(#version 300 es
precision highp float;
in vec2 screen;
uniform float aspect;
uniform vec2 angles;
uniform float cameraRoll;
out vec4 outColor;
void main(){
 vec3 ray=normalize(vec3(screen.x*aspect/1.37638,screen.y/1.37638,-1.0));
 ray.xy=mat2(cos(cameraRoll),-sin(cameraRoll),sin(cameraRoll),cos(cameraRoll))*ray.xy;
 float altitude=cos(angles.y)*ray.y-sin(angles.y)*ray.z;
 float blend=smoothstep(0.0,.85,max(altitude,0.0));
 outColor=vec4(mix(vec3(.56,.61,.63),vec3(.32,.41,.51),blend),1.0);
})";
GLuint compile(GLenum type,const char*text){GLuint s=glCreateShader(type);glShaderSource(s,1,&text,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char log[1024];glGetShaderInfoLog(s,sizeof(log),nullptr,log);__android_log_print(ANDROID_LOG_ERROR,"Ekron","Shader: %s",log);glDeleteShader(s);return 0;}return s;}
GLuint link(const char *vsText,const char *fsText){
 const auto vs=compile(GL_VERTEX_SHADER,vsText),fs=compile(GL_FRAGMENT_SHADER,fsText);
 if(!vs||!fs){if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);return 0;}
 GLuint p=glCreateProgram();glAttachShader(p,vs);glAttachShader(p,fs);glLinkProgram(p);glDeleteShader(vs);glDeleteShader(fs);
 GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){char log[1024];glGetProgramInfoLog(p,sizeof(log),nullptr,log);__android_log_print(ANDROID_LOG_ERROR,"Ekron","Program: %s",log);glDeleteProgram(p);return 0;}return p;
}
using Vertex=sandbox::MeshVertex;
void cube(std::vector<Vertex>&v,const sandbox::Box&b){sandbox::appendCube(v,b);}

}

extern "C" JNIEXPORT void JNICALL Java_com_example_realms_MainActivity_nativeReady(JNIEnv*,jobject,jboolean ready){gameplayReady=ready;if(!ready){stickRight=0;stickForward=0;verticalInput=0;}}
extern "C" JNIEXPORT void JNICALL Java_com_example_realms_MainActivity_nativeDroneInput(JNIEnv*,jobject,jfloat throttle,jfloat yaw,jfloat pitch,jfloat roll){
 auto clean=[](float v){return gameplayReady&&std::isfinite(v)?std::clamp(v,-1.f,1.f):0.f;};
 droneThrottle=clean(throttle);droneYawInput=clean(yaw);dronePitchInput=clean(pitch);droneRollInput=clean(roll);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_realms_MainActivity_nativeLook(JNIEnv*,jobject,jfloat dx,jfloat dy){
 if(!gameplayReady||!std::isfinite(dx)||!std::isfinite(dy))return;
 std::lock_guard<std::mutex> lock(commandMutex);lookDx=std::clamp(lookDx+dx,-400.f,400.f);lookDy=std::clamp(lookDy+dy,-400.f,400.f);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_realms_MainActivity_nativeMove(JNIEnv*,jobject,jfloat right,jfloat forward){
 stickRight=gameplayReady&&std::isfinite(right)?std::clamp(right,-1.f,1.f):0;
 stickForward=gameplayReady&&std::isfinite(forward)?std::clamp(forward,-1.f,1.f):0;
}
extern "C" JNIEXPORT void JNICALL Java_com_example_realms_MainActivity_nativeCommand(JNIEnv*,jobject,jint command){
 std::lock_guard<std::mutex> lock(commandMutex);if(commands.size()<64)commands.push_back(command);
}

extern "C" JNIEXPORT jfloatArray JNICALL Java_com_example_realms_MainActivity_nativeStatus(JNIEnv *env,jobject){
 std::array<float,14> values{};{std::lock_guard<std::mutex> lock(snapshotMutex);std::copy(snapshot.begin(),snapshot.end(),values.begin());}
 values[13]=float(droneMode.load());
 values[12]=float(freezeState.load());
 values[11]=float(interactionAction.load());
 values[9]=float(currentSelection.load());values[10]=float(currentSurface.load());
 values[8]=float(loadPercent.load());
 values[6]=holdingObject?1.f:0.f;values[7]=float(currentTool.load());
 values[4]=float(loadStage.load());values[5]=float(debugNotice.load());
 auto result=env->NewFloatArray(values.size());if(result)env->SetFloatArrayRegion(result,0,values.size(),values.data());return result;
}
extern "C" JNIEXPORT void JNICALL Java_com_example_realms_MainActivity_nativeVertical(JNIEnv*,jobject,jfloat value){
 verticalInput=std::isfinite(value)?std::clamp(value,-1.f,1.f):0.f;
}
extern "C" JNIEXPORT jfloat JNICALL Java_com_example_realms_MainActivity_nativeDroneExploded(JNIEnv*,jobject){
 return droneExplodedFlag.load()?1.f:0.f;
}

Renderer::Renderer(android_app *app):app_(app),last_(std::chrono::steady_clock::now()){
 loadStage=0;debugNotice=0;currentTool=0;holdingObject=false;LoadGuard loadGuard;flying_=retainedWorld?retainedFlying:false;
 savePath_=std::string(app->activity->internalDataPath)+"/sandbox-v1.txt";
 if(retainedWorld)world_=*retainedWorld;else world_.load(savePath_);
 currentSelection=world_.selected;currentSurface=world_.surface;
 if(retainedDrone){drone_=*retainedDrone;if(drone_.piloting()){drone_.mode=sandbox::FpvDrone::Hover;drone_.velocity={};}}
 tools_.tool=retainedWorld?retainedTool:sandbox::HandTool::HAND_EMPTY;
 building_=tools_.tool==sandbox::HandTool::SPAWN_GUN;currentTool=int(tools_.tool);
 display_=eglGetDisplay(EGL_DEFAULT_DISPLAY);
 if(display_==EGL_NO_DISPLAY||!eglInitialize(display_,nullptr,nullptr))return;
 const EGLint attributes[]={EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};
 EGLConfig config;EGLint count=0;if(!eglChooseConfig(display_,attributes,&config,1,&count)||count<1)return;
 EGLint format;eglGetConfigAttrib(display_,config,EGL_NATIVE_VISUAL_ID,&format);ANativeWindow_setBuffersGeometry(app_->window,0,0,format);
 surface_=eglCreateWindowSurface(display_,config,app->window,nullptr);
 const EGLint contextAttributes[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};context_=eglCreateContext(display_,config,EGL_NO_CONTEXT,contextAttributes);
 if(surface_==EGL_NO_SURFACE||context_==EGL_NO_CONTEXT||!eglMakeCurrent(display_,surface_,surface_,context_))return;
 program_=link(vertex,fragment);skyProgram_=link(skyVertex,skyFragment);if(!program_)return;
 loadStage=1;loadPercent=2;
 auto manager=app_->activity->assetManager;auto navigation=std::make_shared<sandbox::World>(world_);
 assetsJob_=std::async(std::launch::async,[manager,navigation](){
  PreparedAssets result;result.textures=TextureLoader::prepare(manager);loadPercent=12;loadStage=2;
  result.valid=city::GltfRenderer::prepare(manager,"models/abandoned_city_street_for_games.glb",result.city,result.log);loadPercent=58;
  if(!result.valid)return result;
  loadStage=3;
  navigation->walls=result.city.colliders;navigation->wallSegments=result.city.wallSegments;
  city::SpawnPoint anchor;
  if(city::findRoadSpawn(*navigation,result.city.minimum,result.city.maximum,-3.4f,32.7f,anchor)){
   navigation->x=anchor.x;navigation->z=anchor.z;
   if(city::findCarParking(*navigation,result.city.minimum,result.city.maximum,anchor.x+3,anchor.z-9,result.parkingX,result.parkingZ)){
    float ground=result.city.collision->groundHeight(result.parkingX,result.parkingZ,2,-10);result.parkingY=std::isfinite(ground)?ground:0;
    city::LoadOptions options;options.level=false;options.buildPhysics=false;options.targetExtent=4.025f;options.yaw=1.57079632679f;
    result.carValid=city::GltfRenderer::prepare(manager,"models/uaz_469_1973_classic.glb",result.car,result.log,options);
   }
  }
  city::LoadOptions remoteOptions;remoteOptions.level=false;remoteOptions.buildPhysics=false;remoteOptions.targetExtent=.38f;
  result.remoteValid=city::GltfRenderer::prepare(manager,"models/chasing_m2_rc_underwater_drone_remote.glb",result.remote,result.log,remoteOptions);
  remoteOptions.targetExtent=.50f;
  result.droneValid=city::GltfRenderer::prepare(manager,"models/fpv_drone.glb",result.drone,result.log,remoteOptions);
  city::LoadOptions playerOptions;playerOptions.level=false;playerOptions.buildPhysics=false;playerOptions.targetExtent=1.8f;
  result.playerValid=city::GltfRenderer::prepare(manager,"models/player.glb",result.player,result.log,playerOptions);
  if(!result.remoteValid||!result.droneValid||!result.playerValid)result.valid=false;
  loadPercent=80;return result;
 });
 glGenVertexArrays(1,&vao_);glGenBuffers(1,&buffer_);glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,buffer_);
 glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,x)));
 glEnableVertexAttribArray(1);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,r)));
 glEnableVertexAttribArray(2);glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,nx)));
 glEnableVertexAttribArray(3);glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));
 glEnableVertexAttribArray(4);glVertexAttribPointer(4,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,layer)));
 glEnable(GL_DEPTH_TEST);eglSwapInterval(display_,1);
 loadGuard.complete=true;
}
void Renderer::advanceLoading(){
 if(!assetsInstalled_){
  if(!assetsJob_.valid()||assetsJob_.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
  PreparedAssets data;
  try{data=assetsJob_.get();}catch(const std::exception&e){__android_log_print(ANDROID_LOG_ERROR,"Ekron","Loading: %s",e.what());loadStage=-1;return;}
  if(!data.valid){loadStage=-1;return;}
  texturesReady_=textures_.upload(data.textures);
  city_.beginUpload(std::move(data.city));
  if(data.carValid)car_.beginUpload(std::move(data.car));
  if(data.remoteValid)remote_.beginUpload(std::move(data.remote));
  if(data.droneValid)droneModel_.beginUpload(std::move(data.drone));
  world_.scenery.clear();world_.walls=city_.scene().colliders;world_.wallSegments=city_.scene().wallSegments;
  savePath_=std::string(app_->activity->internalDataPath)+"/city-glb-v1.txt";
  bool restoredWorld=retainedWorld.has_value();
  if(!retainedWorld){world_.objects.clear();restoredWorld=world_.load(savePath_);}
  carParking_={8,0,data.parkingX,data.parkingZ};carParking_.y=data.parkingY;carParkingValid_=data.carValid;
  bool exists=std::any_of(world_.objects.begin(),world_.objects.end(),[](const sandbox::Object&o){return o.type==8;});
  if(data.carValid&&!restoredWorld&&!exists&&world_.objects.size()<sandbox::World::limit){sandbox::Object car{8,0,data.parkingX,data.parkingZ};car.y=data.parkingY+.5f;world_.objects.push_back(car);}
  const auto config=readSpawnConfig(app_);
  if(!retainedWorld&&config.enabled){world_.x=config.x;world_.y=config.y;world_.z=config.z;world_.yaw=config.yawDegrees*.01745329252f;world_.pitch=0;flying_=config.startFlying;}
  player_.setBoundary(city_.scene().minimum[0],city_.scene().maximum[0],city_.scene().minimum[2],city_.scene().maximum[2]);
  player_.reset(world_.x,world_.y,world_.z);rebuildBuiltCollision();assetsInstalled_=true;loadStage=4;
 }
 std::string log;
 auto start=std::chrono::steady_clock::now();
 for(int i=0;i<8;i++){
  if(city_.uploading())city_.uploadStep(log);
  if(car_.uploading())car_.uploadStep(log);
  if(remote_.uploading())remote_.uploadStep(log);
  if(droneModel_.uploading())droneModel_.uploadStep(log);
  if(std::chrono::steady_clock::now()-start>std::chrono::milliseconds(4))break;
 }
 if(!log.empty())__android_log_print(ANDROID_LOG_WARN,"Ekron","%s",log.c_str());
 loadPercent=80+int(19*(city_.uploadProgress()+car_.uploadProgress())*.5f);
 if(!city_.uploading()&&!car_.uploading()&&!remote_.uploading()&&!droneModel_.uploading()&&!playerModel_.uploading()){
  if(!city_.ready()){loadStage=-1;return;}
  loadPercent=99;loadStage=5;
 }
}
Renderer::~Renderer(){
 if(seatedCar_>=0){world_.objects[seatedCar_].anchored=carWasAnchored_;world_.x=seatFrom_.x;world_.y=seatFrom_.y;world_.z=seatFrom_.z;seatedCar_=-1;}
 if(assetsJob_.valid())assetsJob_.wait();
 tools_.cancel(world_);
 if(saveJob_.valid())saveJob_.get();
 if(saveDirty_)world_.save(savePath_);
 if(assetsInstalled_){retainedWorld=world_;retainedDrone=drone_;retainedTool=tools_.tool;retainedFlying=flying_;}
 if(display_!=EGL_NO_DISPLAY){if(context_!=EGL_NO_CONTEXT&&surface_!=EGL_NO_SURFACE&&eglMakeCurrent(display_,surface_,surface_,context_)){droneModel_.release();playerModel_.release();remote_.release();car_.release();city_.release();textures_.release();if(skyProgram_)glDeleteProgram(skyProgram_);if(buffer_)glDeleteBuffers(1,&buffer_);if(vao_)glDeleteVertexArrays(1,&vao_);if(program_)glDeleteProgram(program_);}eglMakeCurrent(display_,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(context_!=EGL_NO_CONTEXT)eglDestroyContext(display_,context_);if(surface_!=EGL_NO_SURFACE)eglDestroySurface(display_,surface_);eglTerminate(display_);}
}
void Renderer::resetInput(){verticalInput=0;leftId_=rightId_=-1;moveX_=moveY_=0;last_=std::chrono::steady_clock::now();}
void Renderer::handleInput(){
 std::vector<int> pending;{std::lock_guard<std::mutex> lock(commandMutex);pending.swap(commands);if(gameplayReady){if(drone_.mode==sandbox::FpvDrone::Flying)drone_.look(lookDx,lookDy);else if(!drone_.piloting()){world_.yaw-=lookDx*.004f;world_.pitch=std::clamp(world_.pitch-lookDy*.004f,-1.3f,1.3f);}}lookDx=lookDy=0;}
 if(!gameplayReady){pending.clear();resetInput();drone_.leave();droneThrottle=droneYawInput=dronePitchInput=droneRollInput=0;}
 if(roadJob_.valid()&&roadJob_.wait_for(std::chrono::seconds(0))==std::future_status::ready){
  auto point=roadJob_.get();if(point){world_.x=(*point)[0];world_.z=(*point)[1];
   float ground=city_.scene().collision->groundHeight(world_.x,world_.z,2,-10);
   world_.y=std::isfinite(ground)?ground+1.7f:1.7f;world_.yaw=world_.pitch=0;flying_=false;player_.reset(world_.x,world_.y,world_.z);resetInput();}
 }
 for(int command:pending){bool changed=false;
  if(command==45&&city_.ready()){drone_.toggleView(*city_.scene().collision,builtCollision_);continue;}
  if(command==42){
   if(drone_.piloting())drone_.leave();
   else if(tools_.tool==sandbox::HandTool::DRONE_REMOTE){tools_.select(world_,sandbox::HandTool::HAND_EMPTY);building_=false;}
   droneThrottle=droneYawInput=dronePitchInput=droneRollInput=0;resetInput();continue;
  }
  if(drone_.piloting())continue;
  if(command==50){thirdPerson_=!thirdPerson_;camSmoothInit_=false;continue;}
  if(command==44&&city_.ready()&&tools_.tool==sandbox::HandTool::DRONE_REMOTE){drone_.returnHome({world_.x,world_.y,world_.z},world_.yaw,*city_.scene().collision,builtCollision_);continue;}
  if(command==40&&city_.ready()&&seatedCar_<0){
   if(drone_.take({world_.x,world_.y,world_.z},world_.yaw,*city_.scene().collision,builtCollision_)){
    tools_.select(world_,sandbox::HandTool::HAND_EMPTY);building_=false;placementPreview_.reset();rebuildBuiltCollision();
   }continue;
  }
  if(command==41&&city_.ready()){drone_.place(*city_.scene().collision,builtCollision_);continue;}
  if(command==43&&city_.ready()&&tools_.tool==sandbox::HandTool::DRONE_REMOTE){
   if(drone_.connect({world_.x,world_.y,world_.z},world_.yaw,world_.pitch,*city_.scene().collision,builtCollision_)){resetInput();}continue;
  }
  if(command==36&&city_.ready()){
   if(seatedCar_>=0){
    auto exit=sandbox::carExit(world_,seatedCar_,*city_.scene().collision,builtCollision_,city_.scene().minimum,city_.scene().maximum);
    if(exit){auto&o=world_.objects[seatedCar_];o.anchored=carWasAnchored_;world_.x=exit->x;world_.y=exit->y;world_.z=exit->z;seatedCar_=-1;player_.reset(world_.x,world_.y,world_.z);resetInput();}else debugNotice=5;
   }else if(!tools_.holding()){
    int target=sandbox::interactionTarget(world_,*city_.scene().collision);
    if(target>=0){auto&o=world_.objects[target];if(o.type==3)o.doorTarget=o.doorTarget>.5f?0:1.57079633f;
     else{tools_.cancel(world_);seatedCar_=target;seatFrom_={world_.x,world_.y,world_.z};seatTime_=0;carWasAnchored_=o.anchored;o.anchored=true;o.vx=o.vy=o.vz=0;flying_=false;world_.yaw=o.rotation*1.57079633f+(o.type==7?3.14159265f:-1.57079633f);world_.pitch=0;resetInput();}}
   }
   continue;
  }
  if(seatedCar_>=0)continue;

  if(command>=20&&command<=24){tools_.select(world_,static_cast<sandbox::HandTool>(command-20));building_=tools_.tool==sandbox::HandTool::SPAWN_GUN;rebuildBuiltCollision();}
  else if(command==34&&carParkingValid_){
   tools_.cancel(world_);
   auto parkingWorld=world_;auto previous=std::find_if(parkingWorld.objects.begin(),parkingWorld.objects.end(),[](const sandbox::Object&o){return o.type==8;});
   if(previous!=parkingWorld.objects.end())parkingWorld.objects.erase(previous);
   auto restored=sandbox::carRespawn(parkingWorld,carParking_,*city_.scene().collision,city_.scene().minimum,city_.scene().maximum);
   if(restored){
    bool exists=std::any_of(world_.objects.begin(),world_.objects.end(),[](const sandbox::Object&o){return o.type==8;});
    if(exists||world_.objects.size()<sandbox::World::limit){auto first=std::find_if(world_.objects.begin(),world_.objects.end(),[](const sandbox::Object&o){return o.type==8;});if(first!=world_.objects.end())*first=*restored;else world_.objects.push_back(*restored);changed=true;debugNotice=0;}else debugNotice=3;
   }else debugNotice=3;
   rebuildBuiltCollision();
  }
  else if(command==35)respawnPlayer();
  else if(command>=300&&command<=312){
   if(!building_)spawnReturnTool_=(tools_.tool==sandbox::HandTool::GRAVITY_GUN||tools_.tool==sandbox::HandTool::DRONE_REMOTE)?tools_.tool:sandbox::HandTool::HAND_EMPTY;
   tools_.select(world_,sandbox::HandTool::SPAWN_GUN);building_=true;
   world_.selected=command-300;world_.surface=sandbox::World::fixedSurface(world_.selected);
   placementPreview_.reset();rebuildBuiltCollision();
  }
  else if(command>=100&&command<=112&&building_){world_.selected=command-100;world_.surface=sandbox::World::fixedSurface(world_.selected);}
  else if(command==31&&tools_.tool==sandbox::HandTool::SPAWN_GUN)world_.rotation=(world_.rotation+3)%4;
  else if(command==301)spawnExplodingBarrel();
  else if(command==302)spawnWoodenCrate();
  else if(command==303)spawnMetalCrate();
  else if(command==304)spawnConcreteBlock();
  else if(command==32&&tools_.tool==sandbox::HandTool::GRAVITY_GUN){
   if(tools_.holding())changed=tools_.release(world_);
   else tools_.grab(world_,city_.scene().collision.get());
   rebuildBuiltCollision();
  }
  else if(command==38&&tools_.tool==sandbox::HandTool::GRAVITY_GUN){changed=tools_.freeze(world_,city_.scene().collision.get());}
  else if(command==33&&tools_.tool==sandbox::HandTool::GRAVITY_GUN)tools_.rotate(world_,city_.scene().collision.get());
  else if(command==1&&building_)world_.selected=(world_.selected+1)%8;
  else if(command==2&&building_)world_.rotation=(world_.rotation+1)%4;
  else if((command==3||(command>=200&&command<=212))&&building_&&city_.ready()){
   if(command>=200){world_.selected=command-200;world_.surface=sandbox::World::fixedSurface(world_.selected);}
   auto point=sandbox::RaycastPlacement::commitPreview(world_,placementPreview_,*city_.scene().collision,builtCollision_,city_.scene().minimum,city_.scene().maximum);
   if(point&&(point->type!=8||car_.ready())){world_.objects.push_back(*point);placementPreview_.reset();changed=true;
    tools_.select(world_,spawnReturnTool_);building_=false;}
  }
  else if(command==4&&tools_.tool==sandbox::HandTool::SPAWN_GUN)changed=world_.remove();
  else if(command==6){flying_=!flying_;verticalInput=0;player_.reset(world_.x,world_.y,world_.z);}
  else if(command==8)player_.jump();
  else if(command==9){crouched_=!crouched_;prone_=false;}
  else if(command==10){prone_=!prone_;crouched_=false;}
  else if(command==7&&!roadJob_.valid()&&city_.ready()){
   // Copy immutable navigation data once. BFS and allocations run only on the worker.
   auto navigation=std::make_shared<sandbox::World>();navigation->walls=world_.walls;navigation->wallSegments=world_.wallSegments;navigation->objects=world_.objects;
   auto lo=city_.scene().minimum,hi=city_.scene().maximum;
   roadJob_=std::async(std::launch::async,[navigation,lo,hi]()->std::optional<std::array<float,2>>{city::SpawnPoint p;if(city::findRoadSpawn(*navigation,lo,hi,-3.4f,32.7f,p))return std::array<float,2>{p.x,p.z};return {};});
  }
  if(changed){saveDirty_=true;rebuildBuiltCollision();}
 }
 currentTool=int(tools_.tool);holdingObject=tools_.holding();currentSelection=world_.selected;currentSurface=world_.surface;
 if(saveJob_.valid()&&saveJob_.wait_for(std::chrono::seconds(0))==std::future_status::ready){if(!saveJob_.get())__android_log_print(ANDROID_LOG_ERROR,"Ekron","Save failed");}
 if(saveDirty_&&!saveJob_.valid()){
  sandbox::World saved;saved.objects=world_.objects;if(seatedCar_>=0)saved.objects[seatedCar_].anchored=carWasAnchored_;if(tools_.holding())saved.objects[tools_.held]=tools_.original;auto path=savePath_;
  saveJob_=std::async(std::launch::async,[saved=std::move(saved),path](){return saved.save(path);});saveDirty_=false;
 }
 auto *buffer=android_app_swap_input_buffers(app_);if(!buffer)return;
 // Both movement and look are handled by sibling Android views with split pointer IDs.
 // Drain the old GameActivity motion stream: processing it too would rotate twice.
 android_app_clear_motion_events(buffer);android_app_clear_key_events(buffer);
}
void Renderer::render(){
 if(!program_||surface_==EGL_NO_SURFACE)return;
 if(loadStage>=0&&loadStage<5)advanceLoading();
 eglQuerySurface(display_,surface_,EGL_WIDTH,&width_);eglQuerySurface(display_,surface_,EGL_HEIGHT,&height_);if(width_<1||height_<1)return;
 const auto now=std::chrono::steady_clock::now();float dt=std::clamp(std::chrono::duration<float>(now-last_).count(),0.f,.05f);last_=now;
 if(gameplayReady&&city_.ready()){
  size_t objectCountBefore=world_.objects.size();
  bool moving=dynamics_.update(world_,*city_.scene().collision,dt,tools_.held,city_.scene().minimum,city_.scene().maximum,&tools_.held,&seatedCar_);
  if(world_.objects.size()!=objectCountBefore)saveDirty_=true;
  if(moving){rebuildBuiltCollision();motionSaveTime_+=dt;if(motionSaveTime_>.75f){saveDirty_=true;motionSaveTime_=0;}}
  else if(motionSaveTime_>0){saveDirty_=true;motionSaveTime_=0;}
 }
 if(gameplayReady){
  bool doorsMoved=false;
  for(auto&o:world_.objects)if(o.type==3&&!o.frozen&&std::abs(o.doorAngle-o.doorTarget)>.0001f){
   float old=o.doorAngle;o.doorAngle+=std::clamp(o.doorTarget-o.doorAngle,-dt*2.2f,dt*2.2f);
   physics::CollisionMesh sweep;sandbox::addObjectCollision(sweep,o);sweep.build();
   if(player_.overlaps(sweep,world_.x,world_.y,world_.z)){o.doorAngle=old;o.doorTarget=old;}else doorsMoved=true;
  }
  if(doorsMoved)rebuildBuiltCollision();
  if(seatedCar_>=0){auto seat=sandbox::driverSeat(world_.objects[seatedCar_]);seatTime_=std::min(1.f,seatTime_+dt/.65f);float t=seatTime_*seatTime_*(3-2*seatTime_);auto p=seatFrom_+(seat-seatFrom_)*t;world_.x=p.x;world_.y=p.y;world_.z=p.z;}
 }
 if(gameplayReady&&seatedCar_<0&&!drone_.piloting()){if(flying_)city::fly(world_,stickRight.load(),stickForward.load(),verticalInput.load(),dt);
 else if(city_.ready())player_.update(*city_.scene().collision,stickRight.load(),stickForward.load(),world_.yaw,dt,crouched_,world_.x,world_.y,world_.z,&builtCollision_,prone_,city_.scene().fenceBarriers.get());
 else world_.move(stickRight.load(),stickForward.load(),dt);}
 if(city_.ready()){
  if(seatedCar_<0&&!flying_&&sandbox::needsPlayerRecovery(world_,city_.scene().minimum,city_.scene().maximum))respawnPlayer();
  const auto &lo=city_.scene().minimum,&hi=city_.scene().maximum;
  world_.x=std::clamp(world_.x,lo[0]+.25f,hi[0]-.25f);world_.z=std::clamp(world_.z,lo[2]+.25f,hi[2]-.25f);
 }
 interactionAction=0;
 if(seatedCar_>=0)interactionAction=4;
 else if(city_.ready()&&!tools_.holding()){int target=sandbox::interactionTarget(world_,*city_.scene().collision);if(target>=0){const auto&o=world_.objects[target];interactionAction=(o.type==8||o.type==7)?3:o.doorTarget>.5f?2:1;}}
 if(city_.ready()){
  if(drone_.mode==sandbox::FpvDrone::InHands&&gameplayReady){
   if(tools_.tool!=sandbox::HandTool::HAND_EMPTY)drone_.mode=sandbox::FpvDrone::Absent;
   else {auto p=physics::Vec3{world_.x-std::sin(world_.yaw)*.75f,world_.y-.35f,world_.z-std::cos(world_.yaw)*.75f};
    if(!drone_.blocked(p,*city_.scene().collision,builtCollision_)){drone_.position=p;drone_.yaw=world_.yaw;}}
  }
  if(gameplayReady){
  float droneSpeedBefore=physics::length(drone_.velocity);
  drone_.update(dt,droneRollInput.load(),dronePitchInput.load(),droneThrottle.load(),droneYawInput.load(),0,*city_.scene().collision,builtCollision_);
  if(!drone_.exploded&&drone_.mode==sandbox::FpvDrone::Flying){
   drone_.maxSpeedSinceTakeoff=std::max(drone_.maxSpeedSinceTakeoff,droneSpeedBefore);
   float droneSpeedAfter=physics::length(drone_.velocity);
   // Sudden speed drop = hit something
   if(droneSpeedBefore>8.f&&droneSpeedAfter<droneSpeedBefore*.3f){
    drone_.exploded=true;
    drone_.explosionTime=0;
    drone_.remoteDisabled=true;
    explosionFlash=1.f;
    explosionX=drone_.position.x;explosionY=drone_.position.y;explosionZ=drone_.position.z;
    // Spawn debris
    for(int i=0;i<12;i++){
     debris[i].pos={drone_.position.x,drone_.position.y,drone_.position.z};
     float ang1=6.2831853f*(float(i)/12.f);
     float ang2=3.1415926f*(float(i%4))/4.f;
     float spd=3.f+std::sin(float(i))*2.f;
     debris[i].vel={std::cos(ang1)*std::cos(ang2)*spd,std::sin(ang2)*spd+4.f,std::sin(ang1)*std::cos(ang2)*spd};
     debris[i].rot=0;
     debris[i].rotSpeed=(float(i)-6.f)*2.f;
     debris[i].life=2.f;
    }
    // Disable drone tool
    if(tools_.tool==sandbox::HandTool::DRONE_REMOTE)tools_.tool=sandbox::HandTool::HAND_EMPTY;
  }
   }
  }
  // Update explosion
  if(drone_.exploded){
   drone_.explosionTime+=dt;
   explosionFlash=std::max(0.f,explosionFlash-dt*2.f);
   for(int i=0;i<12;i++){
    if(debris[i].life>0){
     debris[i].pos=debris[i].pos+debris[i].vel*dt;
     debris[i].vel.y-=12.f*dt;
     debris[i].rot+=debris[i].rotSpeed*dt;
     debris[i].life-=dt;
    }
   }
  }
  if(drone_.piloting()||drone_.mode==sandbox::FpvDrone::Hover||drone_.mode==sandbox::FpvDrone::Returning){
   drone_.position.x=std::clamp(drone_.position.x,city_.scene().minimum[0]+.25f,city_.scene().maximum[0]-.25f);
   drone_.position.z=std::clamp(drone_.position.z,city_.scene().minimum[2]+.25f,city_.scene().maximum[2]-.25f);
   drone_.position.y=std::min(drone_.position.y,city_.scene().maximum[1]+15);
  }
  if(drone_.position.y<city_.scene().minimum[1]-5){
   if(drone_.piloting())drone_.leave();else if(drone_.mode==sandbox::FpvDrone::Ground)drone_.mode=sandbox::FpvDrone::Absent;
  }
  if(tools_.tool==sandbox::HandTool::DRONE_REMOTE&&!drone_.piloting()&&((drone_.linked&&(drone_.mode==sandbox::FpvDrone::Hover||drone_.mode==sandbox::FpvDrone::Ground||drone_.mode==sandbox::FpvDrone::Returning))||drone_.aimed({world_.x,world_.y,world_.z},world_.yaw,world_.pitch,*city_.scene().collision,builtCollision_)))interactionAction=6;
 }
 if(city_.ready()&&drone_.outsideView()&&drone_.piloting())drone_.updateChase(*city_.scene().collision,builtCollision_);
 droneMode=int(drone_.mode);droneExplodedFlag=drone_.exploded;
 physics::Vec3 cameraEye;float cameraYaw,cameraPitch;
 if(drone_.piloting()){cameraEye=drone_.camera();cameraYaw=drone_.viewYaw();cameraPitch=drone_.viewPitch();}
 else if(thirdPerson_&&city_.ready()){
  float dist=cameraDistance_,height=.5f;
  physics::Vec3 desired{world_.x-std::sin(world_.yaw)*dist,world_.y+height,world_.z-std::cos(world_.yaw)*dist};
  physics::Vec3 from{world_.x,world_.y,world_.z};
  physics::Vec3 dir=desired-from;
  float maxDist=physics::length(dir);
  if(maxDist>.001f){dir=dir*(1.f/maxDist);physics::CollisionMesh::RayHit hit;
   if(city_.scene().collision&&city_.scene().collision->raycast(from,dir,maxDist,hit))cameraEye=from+dir*(hit.distance-.15f);
   else cameraEye=desired;
  }else cameraEye=desired;
  if(!camSmoothInit_){camSmooth_=cameraEye;camSmoothInit_=true;}
  float smooth=1.f-std::exp(-8.f*dt);
  cameraEye=camSmooth_+(cameraEye-camSmooth_)*smooth;camSmooth_=cameraEye;
  cameraYaw=world_.yaw;cameraPitch=-.3f;
 }else{cameraEye=physics::Vec3{world_.x,world_.y,world_.z};cameraYaw=world_.yaw;cameraPitch=world_.pitch;}
 if(gameplayReady)tools_.pull(world_,dt,city_.scene().collision.get());
 std::vector<Vertex> vertices;vertices.reserve((world_.scenery.size()+world_.objects.size()*10+1)*36);
 if(city_.ready())sandbox::appendCityGraffiti(vertices,*city_.scene().collision);
 for(const auto &b:world_.scenery)cube(vertices,b);for(size_t i=0;i<world_.objects.size();i++){
  if(world_.objects[i].type==8)continue;
  size_t objectBegin=vertices.size();
  if(int(i)==tools_.held){auto visual=world_.objects[i];visual.x=tools_.display.x;visual.z=tools_.display.z;size_t begin=vertices.size();sandbox::appendObject(vertices,visual);float dy=tools_.display.y-sandbox::World::bounds(visual).y;for(size_t j=begin;j<vertices.size();j++)vertices[j].y+=dy;}
  else sandbox::appendObject(vertices,world_.objects[i]);
  sandbox::animateDoorMesh(world_.objects[i],vertices,objectBegin);
  {const auto&o=world_.objects[i];auto b=sandbox::World::bounds(o);
   if(int(i)==tools_.held){b.x=tools_.display.x;b.y=tools_.display.y;b.z=tools_.display.z;}
   float tiltX=o.tiltX,tiltZ=o.tiltZ;
   float cx=std::cos(tiltX),sx=std::sin(tiltX),cz=std::cos(tiltZ),sz=std::sin(tiltZ);
   for(size_t j=objectBegin;j<vertices.size();j++){auto &v=vertices[j];float x=v.x-b.x,y=v.y-b.y,z=v.z-b.z;float py=cx*y-sx*z,pz=sx*y+cx*z;v.x=b.x+cz*x-sz*py;v.y=b.y+sz*x+cz*py;v.z=b.z+pz;
    float ny=cx*v.ny-sx*v.nz,nz=sx*v.ny+cx*v.nz;float nx=cz*v.nx-sz*ny;v.ny=sz*v.nx+cz*ny;v.nx=nx;v.nz=nz;}
  }
 }
 int highlight=tools_.holding()?tools_.held:tools_.target(world_,city_.scene().collision.get());
 freezeState=highlight>=0&&world_.objects[highlight].frozen?1:0;
 if(tools_.holding()&&highlight>=0&&tools_.tool==sandbox::HandTool::GRAVITY_GUN){
  auto bounds=sandbox::World::bounds(world_.objects[highlight]);physics::Vec3 target{bounds.x,bounds.y,bounds.z};
  if(tools_.holding())target=tools_.display;
  sandbox::appendGravityBeam(vertices,world_,target);
 }
 // Player body (3D model) visible only in third-person mode.
 if(thirdPerson_&&city_.ready()&&!drone_.piloting()&&seatedCar_<0&&playerModel_.ready()){
  // Walking animation: vertical bob when moving.
  float moveSpeed=std::hypot(stickRight.load(),stickForward.load());
  if(moveSpeed>.01f)playerWalkTime_+=dt*8.f;else playerWalkTime_=0;
  float bob=std::sin(playerWalkTime_)*(.05f*moveSpeed);
  float cy=std::cos(world_.yaw),sy=std::sin(world_.yaw);
  physics::Vec3 foot{world_.x,player_.feet().y+bob,world_.z};
  // Build transform: rotate around Y, then translate.
  float m[16]={cy,0,-sy,0,0,1,0,0,sy,0,cy,0,foot.x,foot.y,foot.z,1};
  glUniformMatrix4fv(glGetUniformLocation(program_,"objectTransform"),1,GL_FALSE,m);
  playerModel_.draw(program_,cameraEye.x,cameraEye.z);
 }
 // Item in hand: render held object at hand position.
 if(thirdPerson_&&city_.ready()&&!drone_.piloting()&&seatedCar_<0&&tools_.held>=0&&tools_.held<(int)world_.objects.size()){
  const auto&held=world_.objects[tools_.held];
  float hx=world_.x+std::sin(world_.yaw)*.45f-std::cos(world_.yaw)*.25f;
  float hy=player_.feet().y+1.2f+std::sin(playerWalkTime_)*(.02f*std::hypot(stickRight.load(),stickForward.load()));
  float hz=world_.z+std::cos(world_.yaw)*.45f+std::sin(world_.yaw)*.25f;
  float hm[16]={1,0,0,0,0,1,0,0,0,0,1,0,hx,hy,hz,1};
  glUniformMatrix4fv(glGetUniformLocation(program_,"objectTransform"),1,GL_FALSE,hm);
  std::vector<Vertex> heldVerts;sandbox::appendObject(heldVerts,held);
  glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,buffer_);
  glBufferData(GL_ARRAY_BUFFER,heldVerts.size()*sizeof(Vertex),heldVerts.data(),GL_STREAM_DRAW);
  glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(heldVerts.size()));
 }
 placementPreview_.reset();
 if(building_&&seatedCar_<0&&city_.ready()){
  placementPreview_=sandbox::RaycastPlacement::preview(world_,*city_.scene().collision,builtCollision_,city_.scene().minimum,city_.scene().maximum);
  if(placementPreview_&&placementPreview_->type==8&&!car_.ready())placementPreview_.reset();
 }
 const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 auto drawCar=[&](const sandbox::Object&o,bool held,bool hologram,physics::Vec3 sortEye){auto b=sandbox::World::bounds(o);physics::Vec3 target{b.x,b.y,b.z};if(held)target=tools_.display;
  const auto &lo=car_.scene().minimum,&hi=car_.scene().maximum;physics::Vec3 source{(lo[0]+hi[0])/2,(lo[1]+hi[1])/2,(lo[2]+hi[2])/2};
  float ax=o.tiltX,az=o.tiltZ,ay=o.rotation*1.57079632679f;
  auto rotate=[&](physics::Vec3 p){float x=std::cos(ay)*p.x+std::sin(ay)*p.z,z=-std::sin(ay)*p.x+std::cos(ay)*p.z;float y=std::cos(ax)*p.y-std::sin(ax)*z;z=std::sin(ax)*p.y+std::cos(ax)*z;return physics::Vec3{std::cos(az)*x-std::sin(az)*y,std::sin(az)*x+std::cos(az)*y,z};};
  auto x=rotate({1,0,0}),y=rotate({0,1,0}),z=rotate({0,0,1}),t=target-rotate(source);
  float matrix[16]={x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,t.x,t.y,t.z,1};
  glUniformMatrix4fv(glGetUniformLocation(program_,"objectTransform"),1,GL_FALSE,matrix);car_.draw(program_,sortEye.x,sortEye.z,hologram);
 };
 // Both displays render the same world snapshot, but with independent cameras.
 // No physics update, player HUD, remote viewmodel or drone self-mesh in feed.
 auto drawWorld=[&](physics::Vec3 cameraEye,float cameraYaw,float cameraPitch,float viewRoll,int viewX,int viewY,int viewWidth,int viewHeight){
 glViewport(viewX,viewY,viewWidth,viewHeight);glClearColor(.56f,.61f,.63f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
 glBindVertexArray(vao_);
 if(skyProgram_){glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glUseProgram(skyProgram_);
  glUniform1f(glGetUniformLocation(skyProgram_,"aspect"),float(viewWidth)/viewHeight);
  glUniform2f(glGetUniformLocation(skyProgram_,"angles"),cameraYaw,cameraPitch);
  glUniform1f(glGetUniformLocation(skyProgram_,"cameraRoll"),viewRoll);
  glDrawArrays(GL_TRIANGLES,0,3);glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);
 }
 glUseProgram(program_);
 glUniform1i(glGetUniformLocation(program_,"previewPass"),0);
 glUniformMatrix4fv(glGetUniformLocation(program_,"objectTransform"),1,GL_FALSE,identity);
 textures_.bind();glUniform1i(glGetUniformLocation(program_,"albedo"),0);
 glUniform1i(glGetUniformLocation(program_,"texturesReady"),texturesReady_?1:0);
 glUniform1i(glGetUniformLocation(program_,"modelPass"),0);
 glUniform1i(glGetUniformLocation(program_,"modelAlbedo"),1);
 glUniform3f(glGetUniformLocation(program_,"lightDirection"),-.45f,-1.f,-.25f);
 glUniform3f(glGetUniformLocation(program_,"lightColor"),.55f,.52f,.48f);
 glUniform3f(glGetUniformLocation(program_,"ambientColor"),.55f,.59f,.65f);
 glUniform1f(glGetUniformLocation(program_,"cameraRoll"),viewRoll);
 glUniform3f(glGetUniformLocation(program_,"eye"),cameraEye.x,cameraEye.y,cameraEye.z);glUniform2f(glGetUniformLocation(program_,"angles"),cameraYaw,cameraPitch);glUniform1f(glGetUniformLocation(program_,"aspect"),float(viewWidth)/viewHeight);
 glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,buffer_);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));
 city_.draw(program_,cameraEye.x,cameraEye.z);

 for(size_t i=0;i<world_.objects.size();i++)if(world_.objects[i].type==8)drawCar(world_.objects[i],int(i)==tools_.held,false,cameraEye);

 };
 drawWorld(cameraEye,cameraYaw,cameraPitch,drone_.piloting()?drone_.viewRoll():0,0,0,width_,height_);

 glUniformMatrix4fv(glGetUniformLocation(program_,"objectTransform"),1,GL_FALSE,identity);
 if(placementPreview_){
  glUniform1i(glGetUniformLocation(program_,"previewPass"),1);
  if(placementPreview_->type==8)drawCar(*placementPreview_,false,true,cameraEye);
  else{
   std::vector<Vertex> ghost;sandbox::appendObject(ghost,*placementPreview_,true,true);
   glUniform1i(glGetUniformLocation(program_,"modelPass"),0);textures_.bind();
   glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_TRUE);
   glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,buffer_);glBufferData(GL_ARRAY_BUFFER,ghost.size()*sizeof(Vertex),ghost.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(ghost.size()));
  }
  glDisable(GL_BLEND);glDepthMask(GL_TRUE);glUniform1i(glGetUniformLocation(program_,"previewPass"),0);
  glUniformMatrix4fv(glGetUniformLocation(program_,"objectTransform"),1,GL_FALSE,identity);
 }
 if(drone_.mode!=sandbox::FpvDrone::Absent&&(drone_.mode!=sandbox::FpvDrone::Flying||drone_.viewWeight()>.15f)&&droneModel_.ready()){
  const auto&lo=droneModel_.scene().minimum;const auto&hi=droneModel_.scene().maximum;
  physics::Vec3 center{(lo[0]+hi[0])*.5f,(lo[1]+hi[1])*.5f,(lo[2]+hi[2])*.5f};
  auto rotate=[&](physics::Vec3 p){float cr=std::cos(drone_.roll),sr=std::sin(drone_.roll),cp=std::cos(drone_.pitch),sp=std::sin(drone_.pitch),cy=std::cos(drone_.yaw),sy=std::sin(drone_.yaw);
   float x=cr*p.x+sr*p.y,y=-sr*p.x+cr*p.y,z=sp*y+cp*p.z;y=cp*y-sp*p.z;return physics::Vec3{cy*x+sy*z,y,-sy*x+cy*z};};
  auto x=rotate({1,0,0}),y=rotate({0,1,0}),z=rotate({0,0,1}),t=drone_.position-rotate(center);
  const float matrix[16]={x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,t.x,t.y,t.z,1};
  glUniformMatrix4fv(glGetUniformLocation(program_,"objectTransform"),1,GL_FALSE,matrix);
  if(!drone_.exploded){
   droneModel_.draw(program_,cameraEye.x,cameraEye.z);
   // Spinning propellers (only in flight)
   if(drone_.mode==sandbox::FpvDrone::Flying){
    drone_.propellerSpin+=dt*25.f;
    float ps=drone_.propellerSpin;
    const auto&lo=droneModel_.scene().minimum;const auto&hi=droneModel_.scene().maximum;
    float cx=(lo[0]+hi[0])*.5f,cy=(lo[1]+hi[1])*.5f,cz=(lo[2]+hi[2])*.5f;
    float half=.35f;
    float offsets[4][3]={{-half,0,-half},{half,0,-half},{-half,0,half},{half,0,half}};
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
    for(int p=0;p<4;p++){
     float a=ps+(float(p)*1.5707963f);
     float ca=std::cos(a),sa=std::sin(a);
     float pm[16]={ca,0,sa,0,0,1,0,0,-sa,0,ca,0,drone_.position.x+offsets[p][0],drone_.position.y+offsets[p][1]+.05f,drone_.position.z+offsets[p][2],1};
     glUniformMatrix4fv(glGetUniformLocation(program_,"objectTransform"),1,GL_FALSE,pm);
     glUniform4f(glGetUniformLocation(program_,"baseColorFactor"),.8f,.8f,.9f,.35f);
     glDrawArrays(GL_TRIANGLE_FAN,0,12);
    }
    glDisable(GL_BLEND);glDepthMask(GL_TRUE);
   }
  }else{
   // Explosion flash
   if(explosionFlash>.01f){
    float fm[16]={1,0,0,0,0,1,0,0,0,0,1,0,explosionX,explosionY,explosionZ,1};
    glUniformMatrix4fv(glGetUniformLocation(program_,"objectTransform"),1,GL_FALSE,fm);
    glUniform4f(glGetUniformLocation(program_,"baseColorFactor"),1.f,1.f,1.f,explosionFlash*.5f);
    glDrawArrays(GL_TRIANGLE_FAN,0,12);
   }
   // Debris cubes
   for(int i=0;i<12;i++){
    if(debris[i].life>0){
     float dm[16]={1,0,0,0,0,1,0,0,0,0,1,0,debris[i].pos.x,debris[i].pos.y,debris[i].pos.z,1};
     glUniformMatrix4fv(glGetUniformLocation(program_,"objectTransform"),1,GL_FALSE,dm);
     glUniform4f(glGetUniformLocation(program_,"baseColorFactor"),.5f,.4f,.3f,1.f);
     glDrawArrays(GL_TRIANGLE_FAN,0,12);
    }
   }
  }
 }
 // Viewmodel uses camera-local coordinates and its own depth range, not a
 // world object: it cannot collide with a wall or drift when looking around.
 if(gameplayReady&&tools_.tool==sandbox::HandTool::DRONE_REMOTE&&remote_.ready()&&!drone_.piloting()&&seatedCar_<0){
  glClear(GL_DEPTH_BUFFER_BIT);
  glUniform1f(glGetUniformLocation(program_,"cameraRoll"),0);
  glUniform3f(glGetUniformLocation(program_,"eye"),0,0,0);
  glUniform2f(glGetUniformLocation(program_,"angles"),0,0);
  const auto&lo=remote_.scene().minimum;const auto&hi=remote_.scene().maximum;
  float x=-(lo[0]+hi[0])*.5f,y=-.18f-(lo[1]+hi[1])*.5f,z=-.55f-(lo[2]+hi[2])*.5f;
  float transform[16]={1,0,0,0,0,1,0,0,0,0,1,0,x,y,z,1};
  glUniformMatrix4fv(glGetUniformLocation(program_,"objectTransform"),1,GL_FALSE,transform);
  remote_.draw(program_,0,0);
 }
 // A native UI monitor above the controller, using a scissored second view.
 // This avoids readback/Bitmap copies and never samples the player's camera.
 if(gameplayReady&&tools_.tool==sandbox::HandTool::DRONE_REMOTE&&!drone_.piloting()&&seatedCar_<0&&drone_.mode!=sandbox::FpvDrone::Absent&&drone_.mode!=sandbox::FpvDrone::InHands){
  int monitorWidth=std::max(16,int(std::min(width_*.22f,height_*.46f)));
  int monitorHeight=monitorWidth*9/16;
  int monitorX=(width_-monitorWidth)/2,monitorY=int(height_*.20f);
  glEnable(GL_SCISSOR_TEST);glScissor(monitorX-3,monitorY-3,monitorWidth+6,monitorHeight+6);
  glClearColor(.12f,.65f,.58f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glScissor(monitorX,monitorY,monitorWidth,monitorHeight);
  drawWorld(drone_.feedEye(),drone_.feedYaw(),drone_.feedPitch(),drone_.roll,monitorX,monitorY,monitorWidth,monitorHeight);
  glDisable(GL_SCISSOR_TEST);glViewport(0,0,width_,height_);
 }
 if(eglSwapBuffers(display_,surface_)==EGL_TRUE){
  {std::lock_guard<std::mutex> lock(snapshotMutex);snapshot={world_.x,world_.y,world_.z,flying_?1.f:0.f};}
  if(loadStage==5){loadStage=6;loadPercent=100;}
 }else loadStage=-1;
}

void Renderer::rebuildBuiltCollision(){
 builtCollision_=physics::CollisionMesh{};
 for(size_t i=0;i<world_.objects.size();i++){if(int(i)==tools_.held)continue;const auto&o=world_.objects[i];sandbox::addObjectCollision(builtCollision_,o);}
 builtCollision_.build();
}

void Renderer::respawnPlayer(){
 if(!city_.scene().collision)return;
 tools_.cancel(world_);rebuildBuiltCollision();
 auto point=sandbox::recoveryPoint(world_,*city_.scene().collision,builtCollision_);
 if(!point){debugNotice=2;return;}
 world_.x=point->x;world_.y=point->y;world_.z=point->z;world_.yaw=world_.pitch=0;flying_=false;
 player_.reset(world_.x,world_.y,world_.z);stickRight=stickForward=verticalInput=0;resetInput();debugNotice=4;
}













void Renderer::spawnExplodingBarrel(){
 if(city_.ready()&&!drone_.piloting()&&seatedCar_<0){
  sandbox::Object barrel{9,0,world_.x+2,world_.z+2};
  barrel.y=1.7f; world_.objects.push_back(barrel);
 }
}

void Renderer::spawnWoodenCrate(){
 if(city_.ready()&&!drone_.piloting()&&seatedCar_<0){
  sandbox::Object crate{5,0,world_.x+2,world_.z+2};
  crate.y=1.7f; world_.objects.push_back(crate);
 }
}

void Renderer::spawnMetalCrate(){
 if(city_.ready()&&!drone_.piloting()&&seatedCar_<0){
  sandbox::Object crate{3,0,world_.x+2,world_.z+2};
  crate.y=1.7f; world_.objects.push_back(crate);
 }
}

void Renderer::spawnConcreteBlock(){
 if(city_.ready()&&!drone_.piloting()&&seatedCar_<0){
  sandbox::Object block{1,0,world_.x+2,world_.z+2};
  block.y=1.8f; world_.objects.push_back(block);
 }
}
