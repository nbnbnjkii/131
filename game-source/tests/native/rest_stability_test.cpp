#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace sandbox;using namespace physics;
 std::array<float,3> lo{-30,-5,-30},hi{30,20,30};
 CollisionMesh floor;floor.addBox({-25,-.01f,-25},{25,0,25});floor.build();World w;ObjectDynamics sim;
 w.objects={{8,0,0,0}};w.objects[0].y=.03f;
 for(int i=0;i<1200;i++){sim.update(w,floor,1.f/120,-1,lo,hi);}
 assert(w.objects.size()==1&&!w.objects[0].awake);auto resting=w.objects[0];
 for(int i=0;i<36000;i++){sim.update(w,floor,1.f/120,-1,lo,hi);}
 assert(w.objects.size()==1&&w.objects[0].y==resting.y&&w.objects[0].tiltX==resting.tiltX&&w.objects[0].tiltZ==resting.tiltZ&&w.objects[0].vy==0);
 for(int type:{1,4,5,6,7,8,12}){
  w.objects={{type,0,0,0}};w.objects[0].y=8;w.objects[0].vy=-25;w.objects[0].tiltZ=.3f;w.objects[0].spinZ=1;
  for(int i=0;i<1800;i++){sim.update(w,floor,1.f/120,-1,lo,hi);assert(w.objects.size()==1);auto&o=w.objects[0];auto b=World::bounds(o);float r=std::abs(std::sin(o.tiltZ))*b.w/2+std::abs(std::cos(o.tiltZ)*std::cos(o.tiltX))*b.h/2+std::abs(std::cos(o.tiltZ)*std::sin(o.tiltX))*b.d/2;assert(o.y+b.h/2-r>-.05f);}
  assert(!w.objects[0].awake&&w.objects[0].vy==0&&w.objects[0].spinZ==0);
 }
 // Central support catches small platforms even when all four corner probes miss.
 CollisionMesh platform;platform.addBox({-.2f,-.1f,-.2f},{.2f,0,.2f});platform.build();w.objects={{8,0,0,0}};w.objects[0].y=2;
 for(int i=0;i<1000;i++){sim.update(w,platform,1.f/120,-1,lo,hi);}assert(w.objects.size()==1&&!w.objects[0].awake);
 // Removing the actual support wakes a sleeper; unrelated edits need not wake all bodies.
 CollisionMesh empty;empty.build();float y=w.objects[0].y;for(int i=0;i<60;i++){sim.update(w,empty,1.f/120,-1,lo,hi);}assert(w.objects[0].awake&&w.objects[0].y<y);
 std::cout<<"Five-minute idle stability, thin-floor impacts, settling, central support and support-loss wake OK\n";
}
