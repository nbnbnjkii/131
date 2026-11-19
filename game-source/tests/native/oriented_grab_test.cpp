#include "../../sys/android/app/src/main/cpp/Interaction.h"
#include "../../sys/android/app/src/main/cpp/ToolInteraction.h"
#include <cassert>
#include <iostream>
int main(){
 for(int type:{3,0,12,8})for(int rotation=0;rotation<4;rotation++){
  sandbox::Object o{type,rotation,0,0};o.tiltX=1.57079632679f;
  auto b=sandbox::World::bounds(o);o.y=b.d/2-b.h/2;
  physics::CollisionMesh mesh;sandbox::addObjectCollision(mesh,o);mesh.build();
  assert(mesh.groundHeight(0,0,5,-5)<b.d+.01f);
  if(type==3){physics::PlayerController player;assert(!player.overlaps(mesh,0,b.d+1.72f,0));
   physics::CollisionMesh::RayHit hit;assert(!mesh.raycast({-4,b.d+.4f,0},{1,0,0},8,hit));}
 }
 sandbox::World w;w.objects={{12,0,0,-3}};w.x=0;w.y=.5f;w.z=0;
 sandbox::ToolInteraction t;t.tool=sandbox::HandTool::GRAVITY_GUN;assert(t.grab(w,nullptr));
 physics::CollisionMesh wall;wall.addBox({.6f,-5,-20},{.7f,5,20});wall.build();
 w.x=3;w.z=-4;
 for(int i=0;i<240;i++){t.pull(w,1.f/120,&wall);assert(!sandbox::objectOverlap(wall,w.objects[0],t.display));}
 assert(t.display.z< -6);assert(t.display.x<.12f);
 // Camera remains authoritative after a blocked pull, without re-grabbing.
 w.x=-3;w.yaw=1;
 for(int i=0;i<240;i++)t.pull(w,1.f/120,&wall);
 assert(t.display.x< -5);assert(t.holding());
 std::cout<<"Tilted colliders in four yaw orientations, no phantom door wall, wall sliding and camera retarget OK\n";
}
