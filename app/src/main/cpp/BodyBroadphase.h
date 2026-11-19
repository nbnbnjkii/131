#pragma once
#include "BodyContacts.h"
#include <tuple>
namespace sandbox {
inline std::vector<size_t> physicalOrder(const World&w){
 std::vector<size_t> order;for(size_t i=0;i<w.objects.size();i++)if(!w.objects[i].destroyed)order.push_back(i);
 std::sort(order.begin(),order.end(),[&](size_t a,size_t b){
  const auto &x=w.objects[a],&y=w.objects[b];
  return std::tie(x.y,x.x,x.z,x.type,x.rotation,x.tiltX,x.tiltZ,x.vx,x.vy,x.vz,x.spinX,x.spinZ,x.doorAngle,x.frozen,x.anchored,x.awake,x.restTime)
       < std::tie(y.y,y.x,y.z,y.type,y.rotation,y.tiltX,y.tiltZ,y.vx,y.vy,y.vz,y.spinX,y.spinZ,y.doorAngle,y.frozen,y.anchored,y.awake,y.restTime);
 });return order;
}
class BodyBroadphase {
 struct Cached {Object pose{};physics::CollisionMesh mesh;bool valid=false;};
 std::vector<Cached> cache_;
 std::vector<Object> graphPoses_;
 int graphHeld_=-2;
public:
 std::vector<std::vector<size_t>> neighbours;
 size_t candidatePairs=0,meshBuilds=0,graphBuilds=0,graphReuses=0;
 void build(const World&w,int held,float dt,const std::vector<Object>*before=nullptr){
  bool unchanged=graphHeld_==held&&graphPoses_.size()==w.objects.size();
  for(size_t i=0;i<w.objects.size()&&unchanged;i++){
   const auto&o=w.objects[i];const auto&p=graphPoses_[i];
   if((o.awake&&!o.frozen&&!o.anchored)||o.type!=p.type||o.rotation!=p.rotation||o.x!=p.x||o.y!=p.y||o.z!=p.z||o.tiltX!=p.tiltX||o.tiltZ!=p.tiltZ||o.doorAngle!=p.doorAngle||o.destroyed!=p.destroyed)unchanged=false;
  }
  if(unchanged){meshBuilds=0;graphReuses++;return;}
  graphPoses_=w.objects;graphHeld_=held;graphBuilds++;
  struct Cell {int x,y,z;size_t index;};std::vector<Cell> cells;
  auto order=physicalOrder(w);std::vector<size_t> rank(w.objects.size());for(size_t i=0;i<order.size();i++)rank[order[i]]=i;
  neighbours.assign(w.objects.size(),{});cache_.resize(w.objects.size());candidatePairs=0;meshBuilds=0;
  for(size_t i:order){const auto&o=w.objects[i];if(int(i)==held||o.destroyed)continue;
   physics::Bounds bounds;
   for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f}){
    auto p=bodyContactPoint(o,x,y,z);bounds.add(p);bounds.add(p+physics::Vec3{o.vx*dt,o.vy*dt,o.vz*dt});
    if(before&&i<before->size())bounds.add(bodyContactPoint((*before)[i],x,y,z));
   }
   bounds.lo=bounds.lo-physics::Vec3{.3f,.3f,.3f};bounds.hi=bounds.hi+physics::Vec3{.3f,.3f,.3f};
   auto cell=[](float p){return int(std::floor(std::clamp(p,-256.f,256.f)/4));};
   for(int x=cell(bounds.lo.x);x<=cell(bounds.hi.x);x++)for(int y=cell(bounds.lo.y);y<=cell(bounds.hi.y);y++)for(int z=cell(bounds.lo.z);z<=cell(bounds.hi.z);z++)cells.push_back({x,y,z,i});
  }
  std::sort(cells.begin(),cells.end(),[&](const Cell&a,const Cell&b){return std::make_tuple(a.x,a.y,a.z,rank[a.index])<std::make_tuple(b.x,b.y,b.z,rank[b.index]);});
  for(size_t first=0;first<cells.size();){size_t end=first+1;
   while(end<cells.size()&&cells[first].x==cells[end].x&&cells[first].y==cells[end].y&&cells[first].z==cells[end].z)end++;
   for(size_t a=first;a<end;a++)for(size_t b=a+1;b<end;b++){auto i=cells[a].index,j=cells[b].index;if(i!=j){neighbours[i].push_back(j);neighbours[j].push_back(i);}}
   first=end;
  }
  for(auto&list:neighbours){std::sort(list.begin(),list.end(),[&](size_t a,size_t b){return rank[a]<rank[b];});list.erase(std::unique(list.begin(),list.end()),list.end());candidatePairs+=list.size();}
  candidatePairs/=2;
 }
 const physics::CollisionMesh&solid(const World&w,size_t i){
  auto &c=cache_[i];const auto&o=w.objects[i];const auto&p=c.pose;
  if(!c.valid||p.type!=o.type||p.rotation!=o.rotation||p.x!=o.x||p.y!=o.y||p.z!=o.z||p.tiltX!=o.tiltX||p.tiltZ!=o.tiltZ||p.doorAngle!=o.doorAngle){
   c.mesh=physics::CollisionMesh{};appendBodySolid(c.mesh,o);c.mesh.build();c.pose=o;c.valid=true;meshBuilds++;
  }return c.mesh;
 }
 bool overlaps(const World&w,size_t index,Object candidate,physics::Vec3 centre)const{
  auto b=World::bounds(candidate);candidate.x=centre.x;candidate.y=centre.y-b.h/2;candidate.z=centre.z;
  for(size_t j:neighbours[index])if(!w.objects[j].destroyed&&bodiesOverlap(candidate,w.objects[j]))return true;
  return false;
 }
};
}
