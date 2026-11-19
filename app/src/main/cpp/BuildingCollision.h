#pragma once
#include "SandboxWorld.h"
#include <array>

namespace city {
using CollisionPoint=std::array<float,3>;
// Only opaque, tall, near-vertical facade triangles. No ground coverage mask,
// fences with alpha, short debris, poles, overhead lintels or roof geometry.
inline void addBuildingWall(std::vector<sandbox::WallSegment>&out,
                            CollisionPoint a,CollisionPoint b,CollisionPoint c,bool opaque=true){
    if(!opaque)return;
    float ux=b[0]-a[0],uy=b[1]-a[1],uz=b[2]-a[2];
    float vx=c[0]-a[0],vy=c[1]-a[1],vz=c[2]-a[2];
    float nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
    float length=std::sqrt(nx*nx+ny*ny+nz*nz);
    float low=std::min({a[1],b[1],c[1]}),high=std::max({a[1],b[1],c[1]});
    float span=std::hypot(std::max({a[0],b[0],c[0]})-std::min({a[0],b[0],c[0]}),std::max({a[2],b[2],c[2]})-std::min({a[2],b[2],c[2]}));
    if(length<.0001f||std::abs(ny)/length>.2f||high-low<1.8f||low>.4f||high<1.9f||span<.8f)return;
    // Slice the ACTUAL triangle, instead of filling its projected bounding box.
    // Two torso heights preserve real door openings below lintels.
    for(float height:{.85f,1.45f}){
        std::vector<CollisionPoint> points;
        std::array<CollisionPoint,3> vertices{a,b,c};
        for(int i=0;i<3;i++){
            auto p=vertices[i],q=vertices[(i+1)%3];
            if((p[1]<=height&&q[1]>height)||(q[1]<=height&&p[1]>height)){
                float t=(height-p[1])/(q[1]-p[1]);points.push_back({p[0]+t*(q[0]-p[0]),height,p[2]+t*(q[2]-p[2])});
            }
        }
        if(points.size()==2&&std::hypot(points[0][0]-points[1][0],points[0][2]-points[1][2])>.02f)
            out.push_back({points[0][0],points[0][2],points[1][0],points[1][2]});
    }
}
}
