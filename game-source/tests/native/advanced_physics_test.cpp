#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
using namespace sandbox;using namespace physics;
static std::array<float,3>lo{-70,-10,-70},hi{70,40,70};
static void checkPairs(const World&w,float tolerance){for(size_t i=0;i<w.objects.size();i++)for(size_t j=0;j<i;j++){PairContact contact;if(boxContact(w.objects[i],w.objects[j],contact))assert(contact.depth<tolerance);}}
static World pile(int permutation,const CollisionMesh&scene){
 World w;ObjectDynamics sim;
 // A frozen foundation, twelve cubes and two wide horizontal panels.
 w.objects={{1,0,0,0}};w.objects[0].frozen=true;w.objects[0].y=.01f;
 for(int level=0;level<6;level++)for(float x:{-.53f,.53f}){Object o{12,0,x,0};o.y=.25f+level*1.03f;w.objects.push_back(o);}
 for(int level=0;level<2;level++){Object panel{0,0,0,0};panel.tiltX=1.57079633f;panel.y=6.6f+level*.4f-1.4f;w.objects.push_back(panel);}
 if(permutation==1)std::reverse(w.objects.begin(),w.objects.end());
 if(permutation>1){std::mt19937 random(permutation);std::shuffle(w.objects.begin(),w.objects.end(),random);}
 for(int i=0;i<3600;i++)sim.update(w,scene,1.f/120,-1,lo,hi);
 assert(w.objects.size()==15);checkPairs(w,.025f);
 for(auto&o:w.objects)assert(!o.awake||o.frozen);
 auto rest=w.objects;for(int i=0;i<3600;i++)sim.update(w,scene,1.f/120,-1,lo,hi);
 for(size_t i=0;i<rest.size();i++){assert(w.objects[i].x==rest[i].x&&w.objects[i].y==rest[i].y&&w.objects[i].z==rest[i].z);assert(w.objects[i].tiltX==rest[i].tiltX&&w.objects[i].tiltZ==rest[i].tiltZ);}
 std::sort(w.objects.begin(),w.objects.end(),[](const Object&a,const Object&b){return std::tie(a.type,a.x,a.z,a.y)<std::tie(b.type,b.x,b.z,b.y);});return w;
}
int main(){
 // Independent spring compression, rebound damping and loss of one wheel.
 Object car{8,0,0,0};car.y=.1f;
 auto uneven=[](float x,float z,float,float){return x<0&&z<0?.06f:0.f;};
 auto result=stepSuspension(car,1.f/120,uneven);assert(result.contacts==4);
 assert(car.wheels[0].compression>car.wheels[1].compression+.05f&&car.wheels[0].load>car.wheels[1].load);
 assert(car.spinX!=0&&car.spinZ!=0);
 car.vy=0;car.spinX=car.spinZ=0;
 result=stepSuspension(car,0,[](float x,float z,float,float){return x<0&&z<0?-INFINITY:0.f;});assert(result.contacts==3&&!car.wheels[0].grounded&&car.wheels[0].load==0);
 car.frozen=true;float frozenY=car.y,frozenV=car.vy;
 result=stepSuspension(car,1.f/120,uneven);assert(result.force==0&&car.y==frozenY&&car.vy==frozenV);car.frozen=false;
 float v=car.vy;result=stepSuspension(car,1.f/120,[](float,float,float,float){return -INFINITY;});assert(result.contacts==0&&car.vy==v);
 CollisionMesh scene;scene.addBox({-65,-.2f,-65},{65,0,65});scene.addBox({3,0,-10},{3.2f,12,10});scene.addBox({-10,0,3},{3.2f,12,3.2f});scene.build();
 auto a=pile(0,scene);
 for(int permutation=1;permutation<5;permutation++){
  auto b=pile(permutation,scene);assert(a.objects.size()==b.objects.size());
  for(size_t i=0;i<a.objects.size();i++){assert(std::abs(a.objects[i].x-b.objects[i].x)<1e-5f&&std::abs(a.objects[i].y-b.objects[i].y)<1e-5f&&std::abs(a.objects[i].z-b.objects[i].z)<1e-5f);}
 }
 // Exact relative-translation CCD: a small fast body crosses a thin panel.
 Object projectile{12,0,-5,0},panel{3,1,0,0};projectile.y=.6f;panel.frozen=true;
 Object end=projectile;end.x=5;float time;Vec3 normal;
 assert(sweepPair(projectile,end,panel,panel,time,normal)&&time>0&&time<1&&normal.x>.9f);
 Object touching=projectile;touching.x=-.57f;Object crossing=touching;crossing.x=4;
 assert(sweepPair(touching,crossing,panel,panel,time,normal)&&time<.001f);
 World impact;impact.objects={projectile,panel};impact.objects[0].vx=1200;
 ObjectDynamics sim;sim.update(impact,scene,1.f/120,-1,lo,hi);
 assert(impact.objects.size()==2&&impact.objects[0].x<0&&impact.objects[1].x==0);checkPairs(impact,.015f);
 // Two heavy simultaneous moving bodies, not just dynamic versus frozen.
 impact.objects={{8,0,-4,-3},{8,0,4,-3},{12,0,0,-3}};impact.objects[0].y=impact.objects[1].y=.2f;impact.objects[2].y=.4f;
 impact.objects[0].vx=60;impact.objects[1].vx=-60;
 for(int i=0;i<600;i++){sim.update(impact,scene,1.f/120,-1,lo,hi);checkPairs(impact,.03f);}
 assert(impact.objects.size()==3);checkPairs(impact,.025f);
 // Orthogonal wall/corner impacts for several shapes.
 for(int type:{3,8,12}){
  World corner;corner.objects={{type,0,0,0}};corner.objects[0].y=2;
  corner.objects[0].vx=corner.objects[0].vz=20;ObjectDynamics cornerSim;
  for(int i=0;i<1200;i++)cornerSim.update(corner,scene,1.f/120,-1,lo,hi);
  assert(corner.objects.size()==1);
  for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f}){auto p=objectCorner(corner.objects[0],x,y,z);assert(p.x<3.04f&&p.z<3.04f&&p.y>-.04f);}
 }
 // Inclined surface: suspension should settle without a perpetual alignment snap.
 CollisionMesh ramp;ramp.addBox({-20,-.2f,-20},{20,0,20});
 ramp.add({{-6,.2f,-6},{6,1.4f,6},{6,1.4f,-6}});ramp.add({{-6,.2f,-6},{-6,.2f,6},{6,1.4f,6}});ramp.build();
 World slope;slope.objects={{8,0,0,0}};slope.objects[0].y=1.3f;ObjectDynamics slopeSim;
 for(int i=0;i<3600;i++)slopeSim.update(slope,ramp,1.f/120,-1,lo,hi);
 assert(slope.objects.size()==1&&!slope.objects[0].awake&&std::abs(slope.objects[0].tiltZ-std::atan(.1f))<.04f);
 auto onSlope=slope.objects[0];for(int i=0;i<1200;i++)slopeSim.update(slope,ramp,1.f/120,-1,lo,hi);
 assert(slope.objects[0].x==onSlope.x&&slope.objects[0].y==onSlope.y&&slope.objects[0].z==onSlope.z);
 // 500 sleeping bodies: broad phase should be local, cached geometry reused.
 World crowd;for(int x=0;x<25;x++)for(int z=0;z<20;z++){Object o{12,0,-35.f+x*2.5f,-30.f+z*2.5f};o.y=.005f;o.awake=false;crowd.objects.push_back(o);}
 BodyBroadphase broad;broad.build(crowd,-1,1.f/120);assert(broad.candidatePairs<15000);
 broad.solid(crowd,0);size_t builds=broad.meshBuilds;broad.solid(crowd,0);assert(broad.meshBuilds==builds);
 broad.build(crowd,-1,1.f/120);assert(broad.graphReuses==1);
 crowd.objects[0].x+=.1f;broad.build(crowd,-1,1.f/120);assert(broad.graphBuilds==2);crowd.objects[0].x-=.1f;
 auto start=std::chrono::steady_clock::now();ObjectDynamics many;
 for(int i=0;i<120;i++)many.update(crowd,scene,1.f/60,-1,lo,hi);
 double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 assert(crowd.objects.size()==500);for(auto&o:crowd.objects)assert(!o.awake);
 std::cout<<"Independent suspension, 15-body stack/permutation, fast relative CCD, heavy impact and 500 sleepers OK; sandbox CPU "<<ms/120<<" ms/frame (not Android FPS)\n";
}
