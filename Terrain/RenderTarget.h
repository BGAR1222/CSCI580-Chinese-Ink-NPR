#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include "openGLHeader.h"

class RenderTarget
{
public:
    RenderTarget(int width, int height);
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    // Recreates the framebuffer textures when the window size changes.
    void Resize(int width, int height);

    // Bind this framebuffer as the current drawing destination.
    void BindForWriting() const;

    // Bind its color texture so another shader can sample it.
    void BindColorTexture(GLenum textureUnit) const;

    // Return drawing to the normal application window.
    static void BindDefault();

    GLuint GetFramebuffer() const;
    GLuint GetColorTexture() const;

    int GetWidth() const;
    int GetHeight() const;

private:
    void Create(int width, int height);
    void Destroy();

    int width;
    int height;

    GLuint framebuffer;
    GLuint colorTexture;
    GLuint depthRenderbuffer;
};

#endif