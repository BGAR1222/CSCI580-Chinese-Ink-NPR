#include "RenderTarget.h"

#include <stdexcept>
#include <string>

RenderTarget::RenderTarget(int width, int height)
    : width(0),
      height(0),
      framebuffer(0),
      colorTexture(0),
      depthRenderbuffer(0)
{
    Create(width, height);
}

RenderTarget::~RenderTarget()
{
    Destroy();
}

void RenderTarget::Resize(int newWidth, int newHeight)
{
    if (newWidth <= 0 || newHeight <= 0)
        return;

    if (newWidth == width && newHeight == height)
        return;

    Destroy();
    Create(newWidth, newHeight);
}

void RenderTarget::Create(int newWidth, int newHeight)
{
    if (newWidth <= 0 || newHeight <= 0)
        throw std::invalid_argument(
            "RenderTarget dimensions must be positive."
        );

    width = newWidth;
    height = newHeight;

    // Create and bind the framebuffer object.
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Create one floating-point color texture.
    //
    // RGB stores the encoded view-space normal.
    // Alpha stores linear view-space depth.
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA16F,
        width,
        height,
        0,
        GL_RGBA,
        GL_FLOAT,
        nullptr
    );

    // Nearest filtering prevents the G-buffer values from being
    // interpolated when they are sampled for edge detection.
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        colorTexture,
        0
    );

    // Create a depth buffer so the closest model surface is kept.
    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(
        GL_RENDERBUFFER,
        depthRenderbuffer
    );

    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH_COMPONENT24,
        width,
        height
    );

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER,
        depthRenderbuffer
    );

    const GLenum drawAttachment = GL_COLOR_ATTACHMENT0;

    glDrawBuffers(
        1,
        &drawAttachment
    );

    const GLenum status =
        glCheckFramebufferStatus(GL_FRAMEBUFFER);

    // Stop using the new objects before checking for failure.
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        const std::string message =
            "G-buffer framebuffer is incomplete. Status: " +
            std::to_string(
                static_cast<unsigned int>(status)
            );

        Destroy();
        throw std::runtime_error(message);
    }
}

void RenderTarget::Destroy()
{
    if (depthRenderbuffer != 0)
    {
        glDeleteRenderbuffers(
            1,
            &depthRenderbuffer
        );

        depthRenderbuffer = 0;
    }

    if (colorTexture != 0)
    {
        glDeleteTextures(
            1,
            &colorTexture
        );

        colorTexture = 0;
    }

    if (framebuffer != 0)
    {
        glDeleteFramebuffers(
            1,
            &framebuffer
        );

        framebuffer = 0;
    }

    width = 0;
    height = 0;
}

void RenderTarget::BindForWriting() const
{
    glBindFramebuffer(
        GL_FRAMEBUFFER,
        framebuffer
    );

    glViewport(
        0,
        0,
        width,
        height
    );
}

void RenderTarget::BindColorTexture(
    GLenum textureUnit
) const
{
    glActiveTexture(textureUnit);
    glBindTexture(
        GL_TEXTURE_2D,
        colorTexture
    );
}

void RenderTarget::BindDefault()
{
    glBindFramebuffer(
        GL_FRAMEBUFFER,
        0
    );
}

GLuint RenderTarget::GetFramebuffer() const
{
    return framebuffer;
}

GLuint RenderTarget::GetColorTexture() const
{
    return colorTexture;
}

int RenderTarget::GetWidth() const
{
    return width;
}

int RenderTarget::GetHeight() const
{
    return height;
}