#include "../../sys/android/app/src/main/cpp/SceneCollision.h"
#include "../../sys/android/app/src/main/cpp/PlayerPhysics.h"
#include <cassert>
#include <iostream>
using namespace physics;
city::Scene fence(bool diagonal){
 city::Scene s;s.minimum={-10,-2,-10};s.maximum={10,10,10};city::Material m;m.playerFence=true;s.materials.push_back(m);
 float end=diagonal?2:0;
 city::Batch b;b.material=0;b.vertices={{-2,0,-end,0,0,1,0,0},{2,0,end,0,0,1,1,0},{2,.5f,end,0,0,1,1,1},{-2,0,-end,0,0,1,0,0},{2,.5f,end,0,0,1,1,1},{-2,.5f,-end,0,0,1,0,1}};
 s.batches.push_back(b);city::buildSceneCollision(s);return s;
}
int main(){
 CollisionMesh floor;floor.addBox({-20,-.2f,-20},{20,0,20});floor.build();
 for(bool diagonal:{false,true})for(int side:{-1,1}){
  auto s=fence(diagonal);assert(s.fenceBarriers&&s.fenceBarriers->size()==4);
  PlayerController p;float x=diagonal?-.8f*side:0,y=1.7f,z=.8f*side;p.reset(x,y,z);
  for(int i=0;i<60;i++)p.update(floor,0,0,0,1.f/120,false,x,y,z,nullptr,false,s.fenceBarriers.get());
  p.jump();
  for(int i=0;i<180;i++)p.update(floor,diagonal?float(side):0,float(side),0,1.f/120,false,x,y,z,nullptr,false,s.fenceBarriers.get());
  float distance=diagonal?(z-x)*side:z*side;
  assert(distance>.24f);assert(std::abs(y-1.7f)<.02f); // No crossing, no climbable invisible top.
  x=diagonal?-.8f*side:0;z=.8f*side;y=6;p.reset(x,y,z);
  for(int i=0;i<180;i++)p.update(floor,diagonal?float(side):0,float(side),0,1.f/120,false,x,y,z,nullptr,false,s.fenceBarriers.get());
  assert((diagonal?(z-x)*side:z*side)>.24f); // Jump/drop from a higher object cannot cross either.
 }
 auto window=fence(false);window.materials[0].playerFence=false;city::buildSceneCollision(window);assert(window.fenceBarriers->size()==0);
 CollisionMesh::RayHit hit;auto s=fence(false);assert(!s.collision->raycast({0,3,1},{0,0,-1},3,hit)); // Synthetic extension is NOT in gameplay raycasts.
 std::cout<<"Unjumpable straight/diagonal fences, both sides, elevated approach and no phantom roof/raycast OK\n";
}
