#pragma once
#include <GLES3/gl3.h>
#include <array>
#include <vector>
struct AAssetManager;

// Layers must match the material IDs in SandboxWorld.h.
// All GL methods, including destruction, require the owning current EGL context.
class TextureLoader {
public:
    static constexpr int Size = 512;
    static constexpr int Layers = 8; // asphalt, concrete, brick, rust, untextured white, wood
    TextureLoader() = default;
    ~TextureLoader();
    TextureLoader(const TextureLoader&) = delete;
    TextureLoader& operator=(const TextureLoader&) = delete;
    using Prepared=std::array<std::vector<unsigned char>,Layers>;
    static Prepared prepare(AAssetManager *assets);
    bool upload(const Prepared &images);
    bool load(AAssetManager *assets);
    void bind(GLuint unit = 0) const;
    void release();
private:
    GLuint texture_ = 0;
};
