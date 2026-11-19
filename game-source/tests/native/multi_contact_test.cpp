#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include "../../sys/android/app/src/main/cpp/ToolInteraction.h"
#include <cassert>
#include <iostream>
using namespace sandbox;using namespace physics;
static std::array<float,3>lo{-30,-5,-30},hi{30,20,30};
static float radius(const Object&o){auto b=World::bounds(o);return std::abs(std::sin(o.tiltZ))*b.w/2+std::abs(std::cos(o.tiltZ)*std::cos(o.tiltX))*b.h/2+std::abs(std::cos(o.tiltZ)*std::sin(o.tiltX))*b.d/2;}
static bool penetrates(const Object&a,const Object&b){
 Vec3 pa[8],pb[8];int n=0;for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f}){pa[n]=objectCorner(a,x,y,z);pb[n++]=objectCorner(b,x,y,z);}
 Vec3 ax[3]={normalized(pa[4]-pa[0]),normalized(pa[2]-pa[0]),normalized(pa[1]-pa[0])};
 Vec3 bx[3]={normalized(pb[4]-pb[0]),normalized(pb[2]-pb[0]),normalized(pb[1]-pb[0])};
 auto separated=[&](Vec3 axis){if(length(axis)<1e-5f)return false;axis=normalized(axis);float al=1e9f,ah=-1e9f,bl=1e9f,bh=-1e9f;for(int i=0;i<8;i++){float av=dot(pa[i],axis),bv=dot(pb[i],axis);al=std::min(al,av);ah=std::max(ah,av);bl=std::min(bl,bv);bh=std::max(bh,bv);}return std::min(ah,bh)-std::max(al,bl)<.02f;};
 for(auto axis:ax)if(separated(axis))return false;
 for(auto axis:bx)if(separated(axis))return false;
 for(auto x:ax)for(auto y:bx)if(separated(cross(x,y)))return false;
 return true;
}
static void stable(World&w,const CollisionMesh&scene){
 ObjectDynamics sim;
 for(int i=0;i<3600;i++)sim.update(w,scene,1.f/120,-1,lo,hi);
 auto rest=w.objects;assert(rest.size()==4);
 for(size_t i=0;i<rest.size();i++)for(size_t j=0;j<i;j++)assert(!penetrates(rest[i],rest[j]));
 for(auto&o:rest){assert(!o.awake||o.frozen);assert(o.vx==0&&o.vy==0&&o.vz==0&&o.spinX==0&&o.spinZ==0);}
 for(int i=0;i<7200;i++)sim.update(w,scene,1.f/120,-1,lo,hi);
 assert(w.objects.size()==rest.size());
 for(size_t i=0;i<rest.size();i++){
  auto&a=rest[i];auto&b=w.objects[i];assert(a.x==b.x&&a.y==b.y&&a.z==b.z&&a.tiltX==b.tiltX&&a.tiltZ==b.tiltZ);
 }
}
int main(){
 CollisionMesh scene;scene.addBox({-20,-.2f,-20},{20,0,20});scene.addBox({2.6f,0,-10},{2.8f,8,10});scene.build();
 for(bool reverse:{false,true}){
  World w;w.objects={{8,0,0,0},{3,0,0,0},{0,0,0,0},{12,0,0,0}};
  w.objects[0].y=.05f;
  w.objects[1].tiltX=1.57079633f;w.objects[1].y=2.2f-1.15f;
  w.objects[2].tiltX=1.57079633f;w.objects[2].y=2.6f-1.4f;
  w.objects[3].y=3.5f;
  if(reverse)std::reverse(w.objects.begin(),w.objects.end());
  stable(w,scene);
  for(auto&o:w.objects)assert(o.y+World::bounds(o).h/2-radius(o)>-.05f);
 }
 // Four different shapes driven into the same static wall, in separate lanes.
 World againstWall;againstWall.objects={{8,0,.58f,-6},{12,0,2.f,-2},{3,0,1.7f,2},{0,0,1.f,6}};
 againstWall.objects[2].tiltZ=.3f;
 againstWall.objects[3].tiltX=1.57079633f;
 for(auto&o:againstWall.objects){o.y=radius(o)-World::bounds(o).h/2+.08f;o.vx=2;}
 stable(againstWall,scene);
 for(auto&o:againstWall.objects)for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f})assert(objectCorner(o,x,y,z).x<2.63f);
 // Uneven tilted support remains solid through repeated freeze/thaw cycles.
 World w;w.objects={{1,0,0,0},{12,0,-.8f,0},{3,0,.6f,0},{12,0,.6f,0}};
 w.objects[0].y=.8f;w.objects[0].tiltZ=.12f;w.objects[0].frozen=true;
 w.objects[1].y=1.3f;w.objects[2].tiltX=1.57079633f;w.objects[2].y=1.6f-1.15f;w.objects[3].y=2.5f;
 stable(w,scene);auto support=w.objects[0];assert(support.frozen&&support.y==.8f&&support.tiltZ==.12f);
 w.x=0;w.z=4;w.y=1;w.pitch=0;ToolInteraction tools;tools.select(w,HandTool::GRAVITY_GUN);
 // Exercise the same public freeze command, not a test-only state transition.
 w.objects={{12,0,0,0}};w.objects[0].y=1;w.objects[0].frozen=true;
 for(int cycle=0;cycle<12;cycle++){
  assert(tools.freeze(w,nullptr));assert(!w.objects[0].frozen);
  assert(w.objects[0].vx==0&&w.objects[0].vy==0&&w.objects[0].vz==0&&w.objects[0].spinX==0&&w.objects[0].spinZ==0);
  if(cycle<11){assert(tools.freeze(w,nullptr));assert(w.objects[0].frozen);}
 }
 assert(w.objects[0].vx==0&&w.objects[0].vy==0&&w.objects[0].vz==0&&w.objects[0].spinX==0&&w.objects[0].spinZ==0);
 ObjectDynamics sim;float before=w.objects[0].y;sim.update(w,scene,1.f/120,-1,lo,hi);assert(w.objects[0].y<before&&w.objects[0].vx==0&&w.objects[0].vz==0);
 std::cout<<"Mixed four-body stacks, reversed insertion, wall, tilted frozen support, two-minute rest and thaw without impulses OK\n";
}
