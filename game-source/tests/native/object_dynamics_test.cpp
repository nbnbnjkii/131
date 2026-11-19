#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include "../../sys/android/app/src/main/cpp/ToolInteraction.h"
#include <cassert>
#include <iostream>
using namespace physics;
int main(){
 CollisionMesh city;city.addBox({-20,-3,-20},{20,-2,20});city.addBox({-5,-2,-6},{5,5,-5.8f});city.build();
 std::array<float,3> lo{-20,-4,-20},hi{20,10,20};
 sandbox::World w;w.objects={{5,0,0,0}};w.objects[0].y=2;sandbox::ObjectDynamics sim;
 for(int i=0;i<500;i++)sim.update(w,city,1.f/120,-1,lo,hi);
 assert(w.objects[0].y<-1.8f&&w.objects[0].y>-2.05f); // Pit bottom, never artificial y=0.
 w.objects={{5,0,0,-3}};w.x=0;w.y=.7f;w.z=0;sandbox::ToolInteraction tool;tool.select(w,sandbox::HandTool::GRAVITY_GUN);
 assert(tool.grab(w,nullptr));for(int i=0;i<60;i++)tool.pull(w,1.f/60);assert(tool.release(w));assert(w.objects[0].vz<-10);
 bool bounced=false;for(int i=0;i<120;i++){sim.update(w,city,1.f/120,-1,lo,hi);if(w.objects[0].vz>0)bounced=true;}
 assert(bounced);
 w.objects={{8,0,0,-4}};w.objects[0].y=0;w.y=1;assert(tool.grab(w,nullptr));assert(tool.release(w));assert(w.objects[0].vz<0);
 CollisionMesh slope;slope.add({{-10,-1,-10},{10,1,10},{10,1,-10}});slope.add({{-10,-1,-10},{-10,-1,10},{10,1,10}});slope.build();w.objects={{8,0,0,0}};w.objects[0].y=2;w.objects[0].tiltZ=.1f;
 for(int i=0;i<600;i++)sim.update(w,slope,1.f/120,-1,lo,hi);
 assert(w.objects.size()==1&&w.objects[0].y<1.f&&std::abs(w.objects[0].tiltZ)>.05f);
 std::cout<<"Pit gravity, ballistic block/car impulse, wall bounce and terrain tilt OK\n";
}
