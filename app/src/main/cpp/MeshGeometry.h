#pragma once
#include "SandboxWorld.h"
#include <vector>

namespace sandbox {
struct MeshVertex {
    float x,y,z,r,g,b,nx,ny,nz,u,v,layer;
};
inline void appendPlayerBody(std::vector<MeshVertex>&vertices,float x,float feetY,float z,float yaw){
    // Simple "bolvanка": torso + head, positioned at player feet. Yaw is accepted for future use
    // (current shape is roughly symmetric so rotation is not visually critical).
    appendCube(vertices,{x,feetY+0.85f,z,0.5f,1.7f,0.5f,0.22f,0.44f,0.82f,sandbox::Plain});
    appendCube(vertices,{x,feetY+1.88f,z,0.30f,0.30f,0.30f,0.10f,0.32f,0.68f,sandbox::Plain});
}

inline void appendCube(std::vector<MeshVertex>&vertices,const Box&box) {
    static constexpr int faces[6][6]={{0,1,2,0,2,3},{5,4,7,5,7,6},{4,0,3,4,3,7},{1,5,6,1,6,2},{3,2,6,3,6,7},{4,5,1,4,1,0}};
    static constexpr float corners[8][3]={{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1},{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1}};
    static constexpr float normals[6][3]={{0,0,1},{0,0,-1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0}};
    // One repeat every 2 world metres, independent of wall size.
    constexpr float repeatPerMetre=.5f;
    for(int face=0;face<6;face++)for(int index:faces[face]) {
        const auto *c=corners[index],*n=normals[face];
        float x=box.x+c[0]*box.w/2,y=box.y+c[1]*box.h/2,z=box.z+c[2]*box.d/2;
        float u=face<2?x:face<4?z:x;
        float v=face<4?y:z;
        // Textured albedo is already colored; only subtly tint it with existing palette.
        const bool plain=box.material==Plain;
        float r=plain?box.r:.8f+box.r*.2f,g=plain?box.g:.8f+box.g*.2f,b=plain?box.b:.8f+box.b*.2f;
        vertices.push_back({x,y,z,r,g,b,n[0],n[1],n[2],u*repeatPerMetre,v*repeatPerMetre,static_cast<float>(box.material)});
    }
}
}
