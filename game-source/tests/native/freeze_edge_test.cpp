#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include "../../sys/android/app/src/main/cpp/ToolInteraction.h"
#include "../../sys/android/app/src/main/cpp/Interaction.h"
#include "../../sys/android/app/src/main/cpp/CityGraffiti.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace sandbox;using namespace physics;
 World w;w.objects={{8,0,0,-3}};w.objects[0].y=4;w.x=0;w.y=5;w.z=0;
 ToolInteraction tool;tool.select(w,HandTool::GRAVITY_GUN);assert(tool.freeze(w,nullptr));assert(w.objects[0].frozen);
 CollisionMesh floor;floor.addBox({-20,-2,-20},{20,-1,20});floor.build();ObjectDynamics sim;
 std::array<float,3> lo{-20,-3,-20},hi{20,20,20};
 for(int i=0;i<240;i++){sim.update(w,floor,1.f/120,-1,lo,hi);}assert(w.objects[0].y==4);
 const char*path="/tmp/ekron-freeze-save.txt";assert(w.save(path));World loaded;assert(loaded.load(path));assert(loaded.objects[0].frozen);std::remove(path);
 assert(tool.freeze(w,nullptr)&&!w.objects[0].frozen);for(int i=0;i<60;i++)sim.update(w,floor,1.f/120,-1,lo,hi);assert(w.objects[0].y<3);
 // One side over a high narrow cornice, lower floor on the other side.
 CollisionMesh ledge;ledge.addBox({-20,-2,-20},{20,-1,20});ledge.addBox({-3,-1,-5},{-.7f,5,5});ledge.build();
 w.objects={{8,0,.4f,0}};w.objects[0].y=5.02f;bool tilted=false,fell=false;
 // No synthetic horizontal kick: allow gravity torque to tip the chassis.
 for(int i=0;i<1200;i++){sim.update(w,ledge,1.f/120,-1,lo,hi);if(std::abs(w.objects[0].tiltZ)>.05f)tilted=true;if(w.objects[0].y<3)fell=true;}
 assert(tilted&&fell);
 Object car{8,0,0,0};auto seat=driverSeat(car);assert(seat.x>.3f&&seat.z<0); // Front left, not rear bench.
 std::vector<MeshVertex> graffiti;CollisionMesh facade;facade.addBox({-10,0,20},{-9,10,35});facade.build();appendCityGraffiti(graffiti,facade);assert(!graffiti.empty());
 std::cout<<"Freeze toggle/persistence, edge loss of support, tumbling, front seat and facade graffiti OK\n";
}
