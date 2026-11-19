#include "../../sys/android/app/src/main/cpp/MeshGeometry.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../../sys/android/app/src/main/cpp/third_party/stb_image.h"
#include <cassert>
#include "../../sys/android/app/src/main/cpp/ObjectMesh.h"
#include <iostream>
#include <string>

int main() {
    for(int type:{0,1,2,3,4,5,6,7,9,10,11,12})for(int rotation=0;rotation<4;rotation++){
        sandbox::Object o{type,rotation,2,3};o.y=1;
        std::vector<sandbox::MeshVertex> real,ghost;sandbox::appendObject(real,o);sandbox::appendObject(ghost,o,true,true);
        assert(real.size()==ghost.size());
        for(size_t i=0;i<real.size();i++){const auto&a=real[i];const auto&b=ghost[i];assert(a.x==b.x&&a.y==b.y&&a.z==b.z&&a.u==b.u&&a.v==b.v&&a.layer==b.layer&&a.r==b.r&&a.g==b.g&&a.b==b.b);}
    }
    for(int type:{0,9,10,11,3,4,5,6,7}){
        sandbox::Object item{type,0,0,0};item.surface=sandbox::Rust;
        std::vector<sandbox::MeshVertex> mesh;sandbox::appendObject(mesh,item);
        assert(!mesh.empty());for(const auto&v:mesh){if((type==3||type==6)&&v.layer==sandbox::Plain)continue;assert(v.layer==sandbox::World::fixedSurface(type));assert(v.r==1&&v.g==1&&v.b==1);}
    }
    for(int rotation=0;rotation<4;rotation++){
        for(int type:{3,6}){
            sandbox::Object item{type,rotation,0,0};std::vector<sandbox::MeshVertex> mesh;sandbox::appendObject(mesh,item);
            auto bounds=sandbox::World::bounds(item);int metal=0;
            for(const auto&v:mesh){assert(std::abs(v.x)<=bounds.w/2+.0001f&&std::abs(v.z)<=bounds.d/2+.0001f);assert(v.y>=-.0001f&&v.y<=bounds.h+.0001f);if(v.layer==sandbox::Plain)metal++;}
            assert(metal>400);if(type==6)assert(mesh.size()>9*36);
        }

        sandbox::Object window{2,rotation,0,0};window.surface=sandbox::Brick;
        std::vector<sandbox::MeshVertex> mesh;sandbox::appendObject(mesh,window);
        int frame=0,glass=0;for(const auto&v:mesh){if(v.layer==sandbox::Wood)frame++;if(v.layer==sandbox::Glass)glass++;}
        assert(frame==5*36&&glass==36&&mesh.size()==6*36);
    }

    for(const char *name:{"asphalt","concrete","brick","rust","wood","fabric","glass"}) {
        std::string path="sys/android/app/src/main/assets/textures/"+std::string(name)+".png";
        int w=0,h=0,n=0;
        auto *p=stbi_load(path.c_str(),&w,&h,&n,4);
        assert(p && w==512 && h==512);
        for(int y=0;y<h;y++)for(int c=0;c<4;c++)assert(p[(y*w)*4+c]==p[(y*w+w-1)*4+c]);
        for(int x=0;x<w;x++)for(int c=0;c<4;c++)assert(p[x*4+c]==p[((h-1)*w+x)*4+c]);
        stbi_image_free(p);
        std::cout<<name<<": PNG decode, 512x512, seamless edges OK\n";
    }
    int w,h,n;
    const unsigned char corrupt[]={137,80,78,71,0,0,0,0};
    assert(!stbi_load_from_memory(corrupt,sizeof(corrupt),&w,&h,&n,4));
    assert(!stbi_load("/this-file-does-not-exist.png",&w,&h,&n,4));
    std::vector<sandbox::MeshVertex> vertices;
    sandbox::appendCube(vertices,{0,3,0,12,6,.4f,1,1,1,sandbox::Concrete});
    assert(vertices.size()==36);
    for(auto v:vertices) {
        assert(std::abs(v.nx*v.nx+v.ny*v.ny+v.nz*v.nz-1)<.00001f);
        assert(v.layer==sandbox::Concrete);
    }
    // Front wall 12 m wide repeats six times, not once stretched across the facade.
    float uMin=vertices[0].u,uMax=uMin;
    for(int i=0;i<6;i++){uMin=std::min(uMin,vertices[i].u);uMax=std::max(uMax,vertices[i].u);}
    assert(std::abs(uMax-uMin-6)<.00001f);
    assert(vertices[0].nz==1 && vertices[24].ny==1);
    const float expectedU[6]={6,6,.2f,.2f,6,6},expectedV[6]={3,3,3,3,.2f,.2f};
    for(int face=0;face<6;face++){
        float minU=1e6f,maxU=-1e6f,minV=1e6f,maxV=-1e6f;
        for(int i=face*6;i<face*6+6;i++){minU=std::min(minU,vertices[i].u);maxU=std::max(maxU,vertices[i].u);minV=std::min(minV,vertices[i].v);maxV=std::max(maxV,vertices[i].v);}
        assert(std::abs(maxU-minU-expectedU[face])<.00001f);assert(std::abs(maxV-minV-expectedV[face])<.00001f);
    }
    std::vector<sandbox::MeshVertex> adjacent;sandbox::appendCube(adjacent,{12,3,0,12,6,.4f,1,1,1,sandbox::Concrete});
    for(int i=0;i<6;i++)for(int j=0;j<6;j++)if(std::abs(vertices[i].x-adjacent[j].x)<.00001f&&std::abs(vertices[i].y-adjacent[j].y)<.00001f){assert(vertices[i].u==adjacent[j].u&&vertices[i].v==adjacent[j].v);}

    std::cout<<"Mesh normals, material IDs and metre-based UV tiling OK\n";
}
