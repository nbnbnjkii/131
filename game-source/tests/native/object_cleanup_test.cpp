#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include "../../sys/android/app/src/main/cpp/ToolInteraction.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace sandbox;using namespace physics;
 CollisionMesh empty;empty.build();std::array<float,3> lo{-20,-3,-20},hi{20,15,20};ObjectDynamics sim;World w;
 w.objects={{8,0,21,0},{12,0,0,0},{5,0,0,3}};w.objects[1].y=-51;w.objects[2].anchored=true;int held=2,seat=-1;
 assert(sim.update(w,empty,1.f/60,held,lo,hi,&held,&seat));assert(w.objects.size()==1&&held==0&&w.objects[0].type==5);
 w.objects.clear();w.objects.push_back({8,0,0,0});w.objects[0].y=-51;w.objects[0].frozen=true;sim.update(w,empty,1.f/60,-1,lo,hi);assert(w.objects.empty());
 for(int i=0;i<600;i++){sim.update(w,empty,1.f/60,-1,lo,hi);}assert(w.objects.empty());
 CollisionMesh floor;floor.addBox({-20,-1,-20},{20,0,20});floor.build();w.objects={{8,0,0,0}};w.objects[0].y=3;w.objects[0].tiltZ=1.2f;
 for(int i=0;i<360;i++){sim.update(w,floor,1.f/120,-1,lo,hi);}assert(w.objects.size()==1&&std::isfinite(w.objects[0].tiltZ));
 w.objects={{8,0,0,-3}};w.x=0;w.y=1;w.z=0;ToolInteraction tool;tool.select(w,HandTool::GRAVITY_GUN);assert(tool.grab(w,nullptr));assert(tool.release(w));assert(w.objects[0].spinX==0&&w.objects[0].spinZ==0&&!w.objects[0].anchored&&!w.objects[0].frozen);
 w.objects[0].tiltZ=.8f;assert(tool.freeze(w,nullptr));
 const char*path="/tmp/ekron-tilt-freeze.txt";assert(w.save(path));World restored;assert(restored.load(path));assert(restored.objects[0].frozen&&std::abs(restored.objects[0].tiltZ-.8f)<.0001f);std::remove(path);
 float angle=w.objects[0].tiltZ;for(int i=0;i<120;i++){sim.update(w,floor,1.f/120,-1,lo,hi);}assert(w.objects[0].tiltZ==angle);
 std::cout<<"Out-of-map deletion, no repeated rescue, index repair, non-upright landing and throw spin OK\n";
}
