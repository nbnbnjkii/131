#include "TextureLoader.h"
#include <android/asset_manager.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <algorithm>
#include <array>
#include <vector>

#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 8192
#include "third_party/stb_image.h"

namespace {
// Fallback exists even if assets is null or a PNG is missing/truncated/too large.
std::vector<unsigned char> fallback(int layer) {
    constexpr unsigned char colors[8][3]={{48,50,53},{137,135,129},{136,72,47},{124,68,35},{255,255,255},{135,91,49},{43,56,48},{142,177,182}};
    std::vector<unsigned char> out(TextureLoader::Size * TextureLoader::Size * 4);
    for(int y=0;y<TextureLoader::Size;y++)for(int x=0;x<TextureLoader::Size;x++) {
        const int shade=layer==4?0:((x/32+y/32)%2)*7;
        const auto p=(y*TextureLoader::Size+x)*4;
        for(int c=0;c<3;c++)out[p+c]=static_cast<unsigned char>(colors[layer][c]+shade);
        out[p+3]=255;
    }
    return out;
}
std::vector<unsigned char> decode(AAssetManager *assets,const char *name,int layer) {
    AAsset *asset=assets?AAssetManager_open(assets,name,AASSET_MODE_BUFFER):nullptr;
    if(!asset){__android_log_print(ANDROID_LOG_WARN,"Ekron","Texture missing: %s; using fallback",name);return fallback(layer);}
    const auto length=AAsset_getLength64(asset);
    std::vector<unsigned char> bytes;
    if(length>0&&length<=32*1024*1024) {
        bytes.resize(static_cast<size_t>(length));
        size_t offset=0;
        while(offset<bytes.size()) {
            const int read=AAsset_read(asset,bytes.data()+offset,bytes.size()-offset);
            if(read<=0)break;
            offset+=static_cast<size_t>(read);
        }
        if(offset!=bytes.size())bytes.clear();
    }
    AAsset_close(asset);
    int w=0,h=0,channels=0;
    // Check dimensions before allocating a decoded RGBA image (avoid decompression bombs).
    if(bytes.empty()||!stbi_info_from_memory(bytes.data(),static_cast<int>(bytes.size()),&w,&h,&channels)||w<1||h<1||w>4096||h>4096) {
        __android_log_print(ANDROID_LOG_WARN,"Ekron","Invalid texture header: %s; using fallback",name);return fallback(layer);
    }
    auto *pixels=stbi_load_from_memory(bytes.data(),static_cast<int>(bytes.size()),&w,&h,&channels,4);
    if(!pixels){__android_log_print(ANDROID_LOG_WARN,"Ekron","Texture decode failed: %s; using fallback",name);return fallback(layer);}
    std::vector<unsigned char> out(TextureLoader::Size*TextureLoader::Size*4);
    // Normalize replacements to array dimensions; supplied assets are already 512x512.
    for(int y=0;y<TextureLoader::Size;y++)for(int x=0;x<TextureLoader::Size;x++) {
        const int sx=x*w/TextureLoader::Size,sy=y*h/TextureLoader::Size;
        std::copy_n(pixels+(sy*w+sx)*4,4,out.data()+(y*TextureLoader::Size+x)*4);
    }
    stbi_image_free(pixels);
    return out;
}
}
TextureLoader::~TextureLoader(){release();}
void TextureLoader::release(){
    // If surface teardown made the context unavailable, EGL destruction owns cleanup.
    if(texture_ && eglGetCurrentContext()!=EGL_NO_CONTEXT)glDeleteTextures(1,&texture_);
    texture_=0;
}
void TextureLoader::bind(GLuint unit)const{glActiveTexture(GL_TEXTURE0+unit);glBindTexture(GL_TEXTURE_2D_ARRAY,texture_);}
TextureLoader::Prepared TextureLoader::prepare(AAssetManager *assets){
 Prepared images;const std::array<const char*,4> names={"textures/asphalt.png","textures/concrete.png","textures/brick.png","textures/rust.png"};
 for(int layer=0;layer<Layers;layer++)images[layer]=layer<4?decode(assets,names[layer],layer):layer==5?decode(assets,"textures/wood.png",layer):layer==6?decode(assets,"textures/fabric.png",layer):layer==7?decode(assets,"textures/glass.png",layer):fallback(layer);
 return images;
}
bool TextureLoader::load(AAssetManager *assets){return upload(prepare(assets));}
bool TextureLoader::upload(const Prepared &images){

    release();
    glGenTextures(1,&texture_);if(!texture_)return false;
    bind();
    // sRGB albedo is automatically decoded to linear RGB before lighting.
    glTexStorage3D(GL_TEXTURE_2D_ARRAY,10,GL_SRGB8_ALPHA8,Size,Size,Layers);
    for(int layer=0;layer<Layers;layer++) {
        const auto &rgba=images[layer];
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,layer,Size,Size,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    }
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    const auto error=glGetError();
    if(error!=GL_NO_ERROR){__android_log_print(ANDROID_LOG_ERROR,"Ekron","Texture array GL error: 0x%x",error);release();return false;}
    return true;
}
