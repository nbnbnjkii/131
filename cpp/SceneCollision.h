#pragma once
#include "GltfScene.h"

namespace city {
// Alpha-textured fences must not disappear from physics merely because they use BLEND.
// Subdivide only translucent triangles, keeping fully transparent regions empty.
inline float collisionAlpha(const Scene &s,const Material &m,float u,float v){
 if(m.image<0||size_t(m.image)>=s.images.size())return m.color[3];
 const auto &im=s.images[m.image];
 if(im.width<=0||im.height<=0||im.rgba.empty())return m.color[3];
 auto wrap=[](float t,int mode){
  if(mode==33071)return std::clamp(t,0.f,1.f);
  if(mode==33648){float p=t-2*std::floor(t/2);return p<=1?p:2-p;}
  return t-std::floor(t);
 };
 int x=std::min(im.width-1,int(wrap(u,m.wrapS)*im.width));
 int y=std::min(im.height-1,int(wrap(v,m.wrapT)*im.height));
 return m.color[3]*im.rgba[(size_t(y)*im.width+x)*4+3]/255.f;
}
inline void addAlphaCollision(physics::CollisionMesh &mesh,const Scene&s,const Material&m,Vertex a,Vertex b,Vertex c,int depth=0){
 auto position=[](Vertex v){return physics::Vec3{v.x,v.y,v.z};};
 auto midpoint=[](Vertex p,Vertex q){return Vertex{(p.x+q.x)/2,(p.y+q.y)/2,(p.z+q.z)/2,0,0,0,(p.u+q.u)/2,(p.v+q.v)/2};};
 float ab=physics::length(position(a)-position(b)),bc=physics::length(position(b)-position(c)),ca=physics::length(position(c)-position(a));
 if(std::max({ab,bc,ca})>.10f&&depth<18){
  if(ab>=bc&&ab>=ca){auto d=midpoint(a,b);addAlphaCollision(mesh,s,m,a,d,c,depth+1);addAlphaCollision(mesh,s,m,d,b,c,depth+1);}
  else if(bc>=ca){auto d=midpoint(b,c);addAlphaCollision(mesh,s,m,a,b,d,depth+1);addAlphaCollision(mesh,s,m,a,d,c,depth+1);}
  else{auto d=midpoint(c,a);addAlphaCollision(mesh,s,m,a,b,d,depth+1);addAlphaCollision(mesh,s,m,d,b,c,depth+1);}
  return;
 }
 float cutoff=m.alphaMode==1?m.cutoff:.5f;
 // Interior samples avoid making a transparent cell solid just from a neighbouring edge texel.
 for(auto weights:std::array<std::array<float,3>,4>{{{1.f/3,1.f/3,1.f/3},{.6f,.2f,.2f},{.2f,.6f,.2f},{.2f,.2f,.6f}}}){
  float u=a.u*weights[0]+b.u*weights[1]+c.u*weights[2],v=a.v*weights[0]+b.v*weights[1]+c.v*weights[2];
  if(collisionAlpha(s,m,u,v)>=cutoff){mesh.add({position(a),position(b),position(c)});return;}
 }
}
inline void buildSceneCollision(Scene &s){
 auto mesh=std::make_shared<physics::CollisionMesh>();
 auto barriers=std::make_shared<physics::CollisionMesh>();
 for(const auto &batch:s.batches){
  if(batch.decorativeWire)continue;
  const Material *m=batch.material>=0&&size_t(batch.material)<s.materials.size()?&s.materials[batch.material]:nullptr;
  for(size_t i=0;i+2<batch.vertices.size();i+=3){auto a=batch.vertices[i],b=batch.vertices[i+1],c=batch.vertices[i+2];
   if(m&&m->playerFence){
    physics::Vec3 points[3]={{a.x,a.y,a.z},{b.x,b.y,b.z},{c.x,c.y,c.z}};
    auto normal=physics::normalized(physics::cross(points[1]-points[0],points[2]-points[0]));
    // Finite, 24 cm thick triangular prism over the actual fence panel.
    // Unlike a batch AABB, this preserves diagonal panels and real gaps.
    if(std::abs(normal.y)<.15f){
     auto offset=normal*.12f;
     physics::Vec3 front[3],back[3];for(int k=0;k<3;k++){front[k]=points[k]+offset;back[k]=points[k]-offset;}
     mesh->add({front[0],front[1],front[2]});mesh->add({back[2],back[1],back[0]});
     for(int k=0;k<3;k++){int next=(k+1)%3;mesh->add({front[k],back[k],back[next]});mesh->add({front[k],back[next],front[next]});}
    }
    // Extrude only the fence's XZ segment, not its AABB: diagonal panels remain narrow.
    if(std::abs(normal.y)<.15f){
     int first=0,last=1;float longest=0;
     for(int p=0;p<3;p++)for(int q=p+1;q<3;q++){
      float length=std::hypot(points[p].x-points[q].x,points[p].z-points[q].z);
      if(length>longest){longest=length;first=p;last=q;}
     }
     if(longest>.01f){
      auto p=points[first],q=points[last];float bottom=s.minimum[1]-256,top=s.maximum[1]+256;
      barriers->add({{p.x,bottom,p.z},{q.x,bottom,q.z},{q.x,top,q.z}});
      barriers->add({{p.x,bottom,p.z},{q.x,top,q.z},{p.x,top,p.z}});
     }
    }
   }
   if(m&&m->playerFence)continue;
   if(m&&m->alphaMode!=0)addAlphaCollision(*mesh,s,*m,a,b,c);
   else mesh->add({{a.x,a.y,a.z},{b.x,b.y,b.z},{c.x,c.y,c.z}});
  }
 }
 mesh->build();s.collision=mesh;
 barriers->build();s.fenceBarriers=barriers;
}
}
