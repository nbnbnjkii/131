#pragma once
#include "BodyBroadphase.h"
namespace sandbox {
struct PairContact {physics::Vec3 normal{};float depth=0;};
inline bool boxContact(const Object&a,const Object&b,PairContact&contact){
 physics::Vec3 pa[8],pb[8],ca{},cb{};int n=0;
 for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f}){pa[n]=bodyContactPoint(a,x,y,z);pb[n]=bodyContactPoint(b,x,y,z);ca=ca+pa[n];cb=cb+pb[n];n++;}
 ca=ca*.125f;cb=cb*.125f;
 physics::Vec3 ax[3]={physics::normalized(pa[4]-pa[0]),physics::normalized(pa[2]-pa[0]),physics::normalized(pa[1]-pa[0])};
 physics::Vec3 bx[3]={physics::normalized(pb[4]-pb[0]),physics::normalized(pb[2]-pb[0]),physics::normalized(pb[1]-pb[0])};
 contact.depth=1e9f;
 auto test=[&](physics::Vec3 axis){
  if(physics::length(axis)<1e-5f)return true;
  axis=physics::normalized(axis);float al=1e9f,ah=-1e9f,bl=1e9f,bh=-1e9f;
  for(int i=0;i<8;i++){float av=physics::dot(pa[i],axis),bv=physics::dot(pb[i],axis);al=std::min(al,av);ah=std::max(ah,av);bl=std::min(bl,bv);bh=std::max(bh,bv);}
  if(ah<bl||bh<al)return false;
  float depth=std::min(ah-bl,bh-al);
  if(depth<contact.depth){contact.depth=depth;contact.normal=axis*(physics::dot(cb-ca,axis)>=0?1.f:-1.f);}return true;
 };
 for(auto axis:ax)if(!test(axis))return false;
 for(auto axis:bx)if(!test(axis))return false;
 for(auto x:ax)for(auto y:bx)if(!test(physics::cross(x,y)))return false;
 return true;
}
// Continuous SAT for relative translation, with fixed orientations at start.
// Angular motion still uses the existing subdivided city/body corner sweep.
inline bool sweepPair(const Object&a,const Object&endA,const Object&b,const Object&endB,float&time,physics::Vec3&normal){
 physics::Vec3 relativeTravel{endB.x-b.x-endA.x+a.x,endB.y-b.y-endA.y+a.y,endB.z-b.z-endA.z+a.z};
 // Slow contacts are resolved by the discrete graph. Rewinding an already
 // touching stack to t~0 discards gravity's shared motion and pins it in air.
 if(physics::length(relativeTravel)<.02f)return false;
 PairContact initial;
 if(boxContact(a,b,initial)){
  physics::Vec3 travel{endB.x-b.x-endA.x+a.x,endB.y-b.y-endA.y+a.y,endB.z-b.z-endA.z+a.z};
  if(initial.depth<=.0031f&&physics::dot(travel,initial.normal)<-.01f){time=0;normal=initial.normal;return true;}
  if(initial.depth>.001f)return false;
 }
 physics::Vec3 pa[8],pb[8];int n=0;
 for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f}){pa[n]=bodyContactPoint(a,x,y,z);pb[n++]=bodyContactPoint(b,x,y,z);}
 physics::Vec3 ax[3]={physics::normalized(pa[4]-pa[0]),physics::normalized(pa[2]-pa[0]),physics::normalized(pa[1]-pa[0])};
 physics::Vec3 bx[3]={physics::normalized(pb[4]-pb[0]),physics::normalized(pb[2]-pb[0]),physics::normalized(pb[1]-pb[0])};
 physics::Vec3 delta{endB.x-b.x-endA.x+a.x,endB.y-b.y-endA.y+a.y,endB.z-b.z-endA.z+a.z};float enter=0,exit=1;normal={};
 auto test=[&](physics::Vec3 axis){
  if(physics::length(axis)<1e-5f)return true;
  axis=physics::normalized(axis);float al=1e9f,ah=-1e9f,bl=1e9f,bh=-1e9f;
  for(int i=0;i<8;i++){float av=physics::dot(pa[i],axis),bv=physics::dot(pb[i],axis);al=std::min(al,av);ah=std::max(ah,av);bl=std::min(bl,bv);bh=std::max(bh,bv);}
  float speed=physics::dot(delta,axis);
  if(std::abs(speed)<1e-7f)return ah>=bl&&bh>=al;
  float first=(al-bh)/speed,last=(ah-bl)/speed;if(first>last)std::swap(first,last);
  if(first>enter){enter=first;normal=axis*(speed<0?1.f:-1.f);}exit=std::min(exit,last);return enter<=exit;
 };
 for(auto axis:ax)if(!test(axis))return false;
 for(auto axis:bx)if(!test(axis))return false;
 for(auto x:ax)for(auto y:bx)if(!test(physics::cross(x,y)))return false;
 time=enter;return enter>0&&enter<1&&physics::length(normal)>.5f;
}
inline float inverseMass(const Object&o,int index,int held){return o.frozen||o.anchored||o.destroyed||index==held?0:1/World::mass(o);}
inline void pairImpulse(Object&a,Object&b,physics::Vec3 normal,float ia,float ib){
 float sum=ia+ib;if(sum<=0)return;
 physics::Vec3 va{a.vx,a.vy,a.vz},vb{b.vx,b.vy,b.vz};float closing=physics::dot(vb-va,normal);
 if(closing>=0)return;
 float restitution=closing< -2?std::min(contactMaterial(a).restitution,contactMaterial(b).restitution):0;
 float impulse=-(1+restitution)*closing/sum;
 va=va-normal*(impulse*ia);vb=vb+normal*(impulse*ib);
 auto tangent=(vb-va)-normal*physics::dot(vb-va,normal);float speed=physics::length(tangent);
 if(speed>1e-6f){float mu=std::sqrt(contactMaterial(a).dynamicFriction*contactMaterial(b).dynamicFriction);auto friction=tangent*(std::min(speed/sum,mu*impulse)/speed);va=va+friction*ia;vb=vb-friction*ib;}
 if(ia>0){a.vx=va.x;a.vy=va.y;a.vz=va.z;if(impulse*ia>.03f){a.awake=true;a.restTime=0;}}
 if(ib>0){b.vx=vb.x;b.vy=vb.y;b.vz=vb.z;if(impulse*ib>.03f){b.awake=true;b.restTime=0;}}
}
struct PairSolverStats {size_t candidates=0,contacts=0,sweeps=0;float maxPenetration=0;};
inline PairSolverStats solveBodyPairs(World&w,const physics::CollisionMesh&city,BodyBroadphase&broadphase,const std::vector<Object>&before,int held,float dt){
 PairSolverStats stats;broadphase.build(w,held,dt,&before);stats.candidates=broadphase.candidatePairs;
 auto order=physicalOrder(w);std::vector<size_t>rank(w.objects.size());for(size_t i=0;i<order.size();i++)rank[order[i]]=i;
 std::vector<std::pair<size_t,size_t>>pairs;
 for(size_t i:order)for(size_t j:broadphase.neighbours[i])if(rank[j]>rank[i])pairs.push_back({i,j});
 for(auto pair:pairs){auto i=pair.first,j=pair.second;auto&a=w.objects[i];auto&b=w.objects[j];float ia=inverseMass(a,int(i),held),ib=inverseMass(b,int(j),held);if(ia+ib==0)continue;
  if(!a.awake&&!b.awake)continue;
  float time;physics::Vec3 normal;
  if(sweepPair(before[i],a,before[j],b,time,normal)){
   auto rewind=[&](Object&o,const Object&start,float inverse){if(inverse==0)return;o.x=start.x+(o.x-start.x)*time;o.y=start.y+(o.y-start.y)*time;o.z=start.z+(o.z-start.z)*time;};
   rewind(a,before[i],ia);rewind(b,before[j],ib);pairImpulse(a,b,normal,ia,ib);stats.sweeps++;
  }
 }
 // Split positional correction: does not inject kinetic energy or use a
 // restitution bias. Revisit the contact graph in both directions.
 for(int iteration=0;iteration<16;iteration++){
  bool corrected=false;
  for(size_t p=0;p<pairs.size();p++){auto pair=pairs[iteration%2?pairs.size()-1-p:p];auto i=pair.first,j=pair.second;auto&a=w.objects[i];auto&b=w.objects[j];
   float ia=inverseMass(a,int(i),held),ib=inverseMass(b,int(j),held);if(ia+ib==0)continue;
   if(!a.awake&&!b.awake)continue;
   PairContact contact;if(!boxContact(a,b,contact))continue;stats.contacts++;
   pairImpulse(a,b,contact.normal,ia,ib);
   float depth=std::max(0.f,contact.depth-.003f);if(depth<.0002f)continue;
   auto moved=[&](const Object&o,float distance){Object result=o;result.x+=contact.normal.x*distance;result.y+=contact.normal.y*distance;result.z+=contact.normal.z*distance;return result;};
   auto allowed=[&](const Object&o,float distance){auto pose=moved(o,distance);auto bounds=World::bounds(pose);
    if(objectOverlap(city,pose,{bounds.x,bounds.y,bounds.z}))return false;
    if(std::abs(distance)>.02f){auto swept=pose;if(sweepObjectFloor(o,swept,city,true)&&physics::length(physics::Vec3{swept.x-pose.x,swept.y-pose.y,swept.z-pose.z})>.01f)return false;}
    return true;};
   float wa=ia,wb=ib,sum=wa+wb;
   if(wa>0&&!allowed(a,-depth*wa/sum))wa=0;
   if(wb>0&&!allowed(b,depth*wb/sum))wb=0;
   sum=wa+wb;if(sum==0)continue;
   float da=-depth*wa/sum,db=depth*wb/sum;
   if(wa>0&&allowed(a,da)){a=moved(a,da);if(depth>.02f){a.awake=true;a.restTime=0;}corrected=true;}
   if(wb>0&&allowed(b,db)){b=moved(b,db);if(depth>.02f){b.awake=true;b.restTime=0;}corrected=true;}
  }
  if(!corrected)break;
 }
 for(auto pair:pairs){if(!w.objects[pair.first].awake&&!w.objects[pair.second].awake)continue;PairContact contact;if(boxContact(w.objects[pair.first],w.objects[pair.second],contact))stats.maxPenetration=std::max(stats.maxPenetration,contact.depth);}
 return stats;
}
}
