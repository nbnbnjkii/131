#include "../../sys/android/app/src/main/cpp/SceneCollision.h"
#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include "../../sys/android/app/src/main/cpp/ToolInteraction.h"
#include "../../sys/android/app/src/main/cpp/GravityBeam.h"
#include "../../sys/android/app/src/main/cpp/RaycastPlacement.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace physics;using namespace sandbox;
 city::Scene scene;scene.minimum={-20,-2,-20};scene.maximum={20,10,20};city::Material m;m.playerFence=true;scene.materials.push_back(m);city::Batch fence;fence.material=0;
 fence.vertices={{-5,0,0,0,0,1,0,0},{5,0,0,0,0,1,0,0},{5,3,0,0,0,1,0,0},{-5,0,0,0,0,1,0,0},{5,3,0,0,0,1,0,0},{-5,3,0,0,0,1,0,0}};
 scene.batches.push_back(fence);city::buildSceneCollision(scene);assert(scene.collision->intersectsBox({0,1,.1f},{.05f,.05f,.05f}));
 World w;w.objects={{3,0,0,2}};w.objects[0].y=.5f;w.objects[0].vz=-16;ObjectDynamics sim;bool bounced=false;
 for(int i=0;i<20;i++){sim.update(w,*scene.collision,1.f/120,-1,scene.minimum,scene.maximum);if(w.objects[0].vz>0)bounced=true;assert(w.objects[0].z>.15f);}assert(bounced);
 Object overlap{12,0,0,0};w.x=10;CollisionMesh empty;empty.build();assert(!RaycastPlacement::valid(w,overlap,*scene.collision,empty,scene.minimum,scene.maximum));
 // A horizontal door is picked through the transformed box, not its old upright bounds.
 w.objects={{3,0,0,0}};w.objects[0].tiltX=1.57079633f;w.objects[0].y=-1.05f;w.x=0;w.y=1.7f;w.z=2;w.pitch=-.6f;
 ToolInteraction tools;tools.select(w,HandTool::GRAVITY_GUN);assert(tools.grab(w,nullptr));auto initial=tools.display;
 CollisionMesh ground;ground.addBox({-20,-.1f,-20},{20,0,20});ground.build();
 tools.pull(w,1.f/120,&ground);assert(length(tools.display-initial)<.2f&&tools.display.y>initial.y);
 auto ends=gravityBeamEnds(w,tools.display);assert(length(ends[1]-tools.display)<.00001f);assert(length(ends[1]-ends[0])>.1f);
 std::vector<MeshVertex> beam;appendGravityBeam(beam,w,tools.display);assert(beam.size()==48);
 scene.batches[0].decorativeWire=true;city::buildSceneCollision(scene);assert(scene.collision->size()==0&&scene.fenceBarriers->size()==0);
 std::cout<<"Thick fence bounce, placement clearance, flat-door grab, smooth pull, beam endpoint and ghost wire OK\n";
}
