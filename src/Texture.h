// Texture.h
// Everything to do with getting images onto the GPU:
//   - 2D textures from a PNG/JPG file (wood floor, stone, rug, font ...)
//   - 2D textures from bytes in memory (the images embedded inside .glb models)
//   - a cubemap made of 6 images (the night sky)
// Images are decoded with Apple's ImageIO / CoreGraphics frameworks.
#pragma once

#include "GLCommon.h"
#include <string>

namespace Texture {

// How the colour values in an image should be treated.
//   Color  -> the image stores sRGB colours (albedo/base colour). OpenGL is told
//             GL_SRGB8_ALPHA8 so it converts them to linear light when sampled.
//   Data   -> the image stores numbers, not colours (normal maps, roughness).
//             Uploaded as plain GL_RGBA8 so the values are used unchanged.
enum class Kind { Color, Data };

// Load an image file. 'maxSize' shrinks very large images (e.g. 4096x4096
// textures inside some models) to save GPU memory. Returns 0 on failure.
GLuint load2D(const std::string& path, Kind kind, int maxSize = 2048);

// Same, but the encoded PNG/JPG bytes are already in memory (embedded texture).
GLuint load2DFromMemory(const unsigned char* bytes, size_t length, Kind kind,
                        int maxSize = 2048);

// Load 6 images into one cubemap texture. Order: +X, -X, +Y, -Y, +Z, -Z.
GLuint loadCubemap(const std::string faces[6]);

// TEXTURE FILTERING: how a texture is sampled when it is shown smaller or
// larger than its real size.
//   0 Nearest   : pick the single closest texel (blocky, shimmers far away)
//   1 Bilinear  : blend the 4 closest texels, no mipmaps (still shimmers)
//   2 Trilinear : blend between the two nearest mipmap levels (smooth, blurry at angles)
//   3 Trilinear + anisotropic 8x : extra samples along the slope (sharp floor)
void setFiltering(GLuint texture, int mode);

// Makes a 1x1 texture of a single colour (used when a model has no map).
GLuint solid(unsigned char r, unsigned char g, unsigned char b, Kind kind);

}  // namespace Texture
