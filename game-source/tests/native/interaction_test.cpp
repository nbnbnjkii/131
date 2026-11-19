#include "../../sys/android/app/src/main/cpp/Interaction.h"
#include <cassert>
#include <iostream>
int main(){
 sandbox::World w;w.objects.clear();w.x=0;w.y=1.05f;w.z=2;
 physics::CollisionMesh city;city.addBox({-20,-.2f,-20},{20,0,20});city.build();
 sandbox::Object door{3,0,0,0};w.objects.push_back(door);assert(sandbox::interactionTarget(w,city)==0);
 w.z=4;assert(sandbox::interactionTarget(w,city)==-1);w.z=2;
 auto hinge=sandbox::doorPoint(door,{-.55f,1,0});door.doorAngle=1.57079633f;auto still=sandbox::doorPoint(door,{-.55f,1,0});assert(physics::length(hinge-still)<.0001f);
 auto edge=sandbox::doorPoint(door,{.55f,1,0});assert(std::abs(edge.x+.55f)<.0001f&&std::abs(edge.z+1.1f)<.0001f);
 w.objects[0]=door;physics::CollisionMesh open;sandbox::addObjectCollision(open,door);open.build();physics::CollisionMesh::RayHit hit;assert(!open.raycast({0,1.7f,2},{0,0,-1},3,hit));
 w.objects.clear();w.objects.push_back({8,0,0,0});w.y=1.7f;assert(sandbox::interactionTarget(w,city)==0);
 physics::CollisionMesh built;sandbox::addObjectCollision(built,w.objects[0]);built.build();auto exit=sandbox::carExit(w,0,city,built,{-20,-1,-20},{20,10,20});assert(exit);
 auto seat=sandbox::driverSeat(w.objects[0]);assert(seat.y>1&&seat.y<1.9f);
 w.objects[0]={7,0,0,0};w.x=0;w.y=.8f;w.z=2;w.pitch=0;
 assert(sandbox::interactionTarget(w,city)==0);
 auto sofaSeat=sandbox::driverSeat(w.objects[0]);assert(sofaSeat.y==1.48f&&std::abs(sofaSeat.z-.08f)<.0001f);
 physics::CollisionMesh sofa;sandbox::addObjectCollision(sofa,w.objects[0]);sofa.build();assert(sandbox::carExit(w,0,city,sofa,{-20,-1,-20},{20,10,20}));
 auto cube=sandbox::World::bounds({12,0,0,0});assert(cube.w==1&&cube.h==1&&cube.d==1);
 std::cout<<"Interaction range, hinge, open-door passage, seat, safe exit and cube dimensions OK\n";
}
