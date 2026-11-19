#include "../../sys/android/app/src/main/cpp/WorldRecovery.h"
#include "../../sys/android/app/src/main/cpp/MaterialRules.h"
#include <cassert>
#include <limits>
#include <iostream>
int main(){
 using namespace sandbox;using namespace physics;
 World w;w.walls.clear();w.wallSegments.clear();w.objects.clear();w.x=-3.4f;w.y=1.7f;w.z=32.7f;
 std::array<float,3> lo{-20,-3,-40},hi{20,14,40};
 assert(!needsPlayerRecovery(w,lo,hi));w.y=-7.1f;assert(needsPlayerRecovery(w,lo,hi));w.y=8;w.z=42;assert(needsPlayerRecovery(w,lo,hi));w.z=32.7f;assert(!needsPlayerRecovery(w,lo,hi));
 CollisionMesh city,built;city.addBox({-20,-.2f,-40},{20,0,40});city.build();built.build();
 auto point=recoveryPoint(w,city,built);assert(point&&std::abs(point->x+3.4f)<.01f&&std::abs(point->z-32.7f)<.01f);
 built.addBox({-4.5f,0,31.5f},{-2.5f,3,33.5f});built.build();point=recoveryPoint(w,city,built);assert(point);PlayerController p;assert(!p.overlaps(built,point->x,point->y,point->z));
 Object parking{8,0,4,20};auto car=carRespawn(w,parking,city,lo,hi);assert(car&&car->type==8&&car->y>=0);
 w.objects.push_back(*car);auto again=carRespawn(w,parking,city,lo,hi);assert(again&&(std::abs(again->x-car->x)>1||std::abs(again->z-car->z)>1)); // Other spawned cars also occupy parking space.
 assert(materialForName("Dining_Chair").layer==Wood);assert(materialForName("CABINET").roughness>.9f);assert(materialForName("Old_Sofa").layer==Fabric);assert(materialForName("Metal_Door").layer==Rust);assert(materialForName("Interior_Wall").layer==Concrete);
 std::cout<<"Void detection, safe spawn, blocked-spawn avoidance, UAZ recovery and named material rules OK\n";
}
