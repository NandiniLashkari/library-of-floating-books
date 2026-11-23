// Texture.cpp
#include "Texture.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

#include <vector>
#include <iostream>
#include <algorithm>

// Not in the core headers, but supported by every macOS GPU driver.
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

namespace {

// Decoded image in plain RGBA bytes. Row 0 is the TOP of the picture.
struct Pixels {
    int width = 0, height = 0;
    std::vector<unsigned char> rgba;
};

// Decode an ImageIO image source into RGBA pixels, shrinking it so neither
// side is bigger than maxSize (keeps the aspect ratio).
bool decode(CGImageSourceRef source, int maxSize, Pixels& out) {
    if (!source) return false;
    CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
    if (!image) return false;

    int w = static_cast<int>(CGImageGetWidth(image));
    int h = static_cast<int>(CGImageGetHeight(image));
    while (w > maxSize || h > maxSize) { w /= 2; h /= 2; }   // halve until it fits

    out.width = w;
    out.height = h;
    out.rgba.assign(static_cast<size_t>(w) * h * 4, 0);

    // Draw the image into a bitmap we own; CoreGraphics does the scaling.
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        out.rgba.data(), w, h, 8, w * 4, space,
        kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast);
    if (ctx) {
        CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
        CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), image);
        CGContextRelease(ctx);
    }
    CGColorSpaceRelease(space);
    CGImageRelease(image);
    return ctx != nullptr;
}

// Upload RGBA pixels as a mipmapped, repeating 2D texture.
GLuint upload2D(const Pixels& px, Texture::Kind kind) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLenum internalFormat = (kind == Texture::Kind::Color) ? GL_SRGB8_ALPHA8
                                                           : GL_RGBA8;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, px.width, px.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, px.rgba.data());

    // Mipmaps = smaller pre-filtered copies, so distant surfaces don't shimmer.
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   // tile in u
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   // tile in v
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Anisotropic filtering keeps the floor sharp at grazing angles.
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
    glGetError();   // ignore if the driver does not support it

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

}  // namespace

namespace Texture {

GLuint load2D(const std::string& path, Kind kind, int maxSize) {
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault, reinterpret_cast<const UInt8*>(path.c_str()),
        static_cast<CFIndex>(path.size()), false);
    if (!url) return 0;
    CGImageSourceRef source = CGImageSourceCreateWithURL(url, nullptr);
    CFRelease(url);

    Pixels px;
    bool ok = decode(source, maxSize, px);
    if (source) CFRelease(source);
    if (!ok) {
        std::cerr << "Could not load texture: " << path << '\n';
        return 0;
    }
    std::cout << "Loaded texture " << path << " (" << px.width << "x"
              << px.height << ")\n";
    return upload2D(px, kind);
}

GLuint load2DFromMemory(const unsigned char* bytes, size_t length, Kind kind,
                        int maxSize) {
    CFDataRef data = CFDataCreate(kCFAllocatorDefault, bytes,
                                  static_cast<CFIndex>(length));
    CGImageSourceRef source = CGImageSourceCreateWithData(data, nullptr);
    CFRelease(data);

    Pixels px;
    bool ok = decode(source, maxSize, px);
    if (source) CFRelease(source);
    if (!ok) {
        std::cerr << "Could not decode embedded texture\n";
        return 0;
    }
    return upload2D(px, kind);
}

GLuint loadCubemap(const std::string faces[6]) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (int i = 0; i < 6; ++i) {
        CFURLRef url = CFURLCreateFromFileSystemRepresentation(
            kCFAllocatorDefault, reinterpret_cast<const UInt8*>(faces[i].c_str()),
            static_cast<CFIndex>(faces[i].size()), false);
        CGImageSourceRef source = url ? CGImageSourceCreateWithURL(url, nullptr)
                                      : nullptr;
        if (url) CFRelease(url);

        Pixels px;
        if (!decode(source, 2048, px)) {
            std::cerr << "Could not load cubemap face: " << faces[i] << '\n';
        } else {
            // GL_TEXTURE_CUBE_MAP_POSITIVE_X + i walks +X,-X,+Y,-Y,+Z,-Z
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB8_ALPHA8,
                         px.width, px.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         px.rgba.data());
        }
        if (source) CFRelease(source);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Clamp so the seams between faces don't show.
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    std::cout << "Loaded sky cubemap\n";
    return tex;
}

void setFiltering(GLuint texture, int mode) {
    glBindTexture(GL_TEXTURE_2D, texture);
    GLint minFilter = GL_LINEAR_MIPMAP_LINEAR;   // trilinear
    GLint magFilter = GL_LINEAR;
    float anisotropy = 1.0f;
    if (mode == 0) { minFilter = GL_NEAREST; magFilter = GL_NEAREST; }
    if (mode == 1) { minFilter = GL_LINEAR; }
    if (mode == 3) { anisotropy = 8.0f; }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
    glGetError();   // ignore if anisotropy is not supported
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint solid(unsigned char r, unsigned char g, unsigned char b, Kind kind) {
    Pixels px;
    px.width = px.height = 1;
    px.rgba = {r, g, b, 255};
    return upload2D(px, kind);
}

}  // namespace Texture
