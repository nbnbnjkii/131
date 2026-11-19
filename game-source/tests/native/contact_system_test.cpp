#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include "../../sys/android/app/src/main/cpp/ToolInteraction.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace sandbox;using namespace physics;
 std::array<float,3>lo{-30,-5,-30},hi{30,20,30};CollisionMesh floor;floor.addBox({-25,-.2f,-25},{25,0,25});floor.build();
 // Friction stops, never reverses, and is independent of substep frequency.
 for(int type:{3,8,11,12}){
  Object a{type,0,0,0},b=a;a.vx=b.vx=4;
  for(int i=0;i<30;i++)applyContactFriction(a,1.f/60);
  for(int i=0;i<60;i++)applyContactFriction(b,1.f/120);
  assert(a.vx>=0&&b.vx>=0&&std::abs(a.vx-b.vx)<.002f);
  for(int i=0;i<600;i++)applyContactFriction(a,1.f/120);
  assert(a.vx==0&&a.vz==0);
 }
 // Frozen objects are immovable SOLID supports, not ghost objects.
 World w;ObjectDynamics sim;w.objects={{12,0,0,0},{12,0,0,0}};
 w.objects[0].frozen=true;w.objects[0].y=1;w.objects[1].y=5;
 for(int i=0;i<1200;i++)sim.update(w,floor,1.f/120,-1,lo,hi);
 assert(w.objects.size()==2&&w.objects[0].y==1&&w.objects[0].frozen);
 assert(std::abs(w.objects[1].y-2)<.03f&&!w.objects[1].awake);
 w.objects[0].destroyed=true;sim.update(w,floor,1.f/120,-1,lo,hi);
 for(int i=0;i<600;i++)sim.update(w,floor,1.f/120,-1,lo,hi);
 assert(w.objects.size()==1&&w.objects[0].y<.03f&&!w.objects[0].awake);
 // Dynamic impact wakes a sleeper and transfers momentum using masses.
 w.objects={{12,0,-.6f,0},{12,0,.6f,0}};w.objects[0].y=w.objects[1].y=.01f;
 w.objects[0].vx=4;w.objects[1].awake=false;
 for(int i=0;i<30;i++)sim.update(w,floor,1.f/120,-1,lo,hi);
 assert(w.objects.size()==2&&w.objects[1].vx>0&&w.objects[1].awake);
 // A tilted body's solid is not its upright box.
 Object tilted{3,0,0,0};tilted.tiltX=1.57079633f;tilted.y=-1;
 CollisionMesh solid;appendBodySolid(solid,tilted);solid.build();
 assert(std::isfinite(solid.groundHeight(0,.8f,2,-2)));
 assert(!std::isfinite(solid.groundHeight(.9f,0,2,-2)));
 // No arbitrary angular impulse is injected into a centre-launched car.
 w.objects={{8,0,0,-3}};w.x=w.z=0;w.y=1.7f;w.pitch=0;
 ToolInteraction tools;tools.select(w,HandTool::GRAVITY_GUN);assert(tools.grab(w,nullptr));
 assert(tools.release(w));assert(w.objects[0].spinX==0&&w.objects[0].spinZ==0&&w.objects[0].awake);
 // Existing wheel-supported parked cars must remain on their wheels.
 w.objects={{8,0,0,0}};w.objects[0].y=.1f;w.objects[0].tiltZ=.1f;
 for(int i=0;i<2400;i++)sim.update(w,floor,1.f/120,-1,lo,hi);
 assert(w.objects.size()==1&&!w.objects[0].awake&&std::abs(w.objects[0].tiltZ)<.01f);
 auto parked=w.objects[0];for(int i=0;i<7200;i++)sim.update(w,floor,1.f/120,-1,lo,hi);
 assert(w.objects[0].x==parked.x&&w.objects[0].y==parked.y&&w.objects[0].z==parked.z);
 std::cout<<"Coulomb friction, frozen solid support/removal, rotated support, centre car throw and four-wheel rest OK\n";
}
