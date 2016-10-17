/*
 * Win32GLContext.cpp
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "Win32GLContext.h"
#include "../../Ext/GLExtensions.h"
#include "../../Ext/GLExtensionLoader.h"
#include "../../../CheckedCast.h"
#include "../../../../Core/Helper.h"
#include <LLGL/Platform/NativeHandle.h>
#include <LLGL/Log.h>
#include <algorithm>


namespace LLGL
{


/*
 * GLContext class
 */

std::unique_ptr<GLContext> GLContext::Create(RenderContextDescriptor& desc, Window& window, GLContext* sharedContext)
{
    Win32GLContext* sharedContextWGL = (sharedContext != nullptr ? LLGL_CAST(Win32GLContext*, sharedContext) : nullptr);
    return MakeUnique<Win32GLContext>(desc, window, sharedContextWGL);
}


/*
 * Win32GLContext class
 */

Win32GLContext::Win32GLContext(RenderContextDescriptor& desc, Window& window, Win32GLContext* sharedContext) :
    GLContext   ( sharedContext ),
    desc_       ( desc          ),
    window_     ( window        )
{
    if (sharedContext)
    {
        auto sharedContextWGL = LLGL_CAST(Win32GLContext*, sharedContext);
        CreateContext(sharedContextWGL);
    }
    else
        CreateContext(nullptr);
}

Win32GLContext::~Win32GLContext()
{
    DeleteContext();
}

bool Win32GLContext::SetSwapInterval(int interval)
{
    /* Load GL extension "wglSwapIntervalEXT" to set swap interval */
    if (wglSwapIntervalEXT || LoadSwapIntervalProcs())
        return (wglSwapIntervalEXT(interval) == TRUE);
    else
        return false;
}

bool Win32GLContext::SwapBuffers()
{
    return (::SwapBuffers(context_.hDC) == TRUE);
}


/*
 * ======= Private: =======
 */

bool Win32GLContext::Activate(bool activate)
{
    if (activate)
        return (wglMakeCurrent(context_.hDC, context_.hGLRC) == TRUE);
    else
        return (wglMakeCurrent(0, 0) == TRUE);
}

static void ErrAntiAliasingNotSupported()
{
    Log::StdErr() << "multi-sample anti-aliasing is not supported" << std::endl;
}

/*
TODO:
- When anti-aliasing and extended-profile-selection is enabled,
  maximal 2 contexts should be created (and not 3).
*/
void Win32GLContext::CreateContext(Win32GLContext* sharedContext)
{
    /* If a shared context has passed, use its pre-selected pixel format */
    if (desc_.multiSampling.enabled && sharedContext)
        CopyPixelFormat(*sharedContext);

    /* First setup device context and choose pixel format */
    SetupDeviceContextAndPixelFormat();

    /* Create standard render context first */
    auto stdRenderContext = CreateGLContext(false, sharedContext);

    if (!stdRenderContext)
        throw std::runtime_error("failed to create standard OpenGL render context");

    /* Check for multi-sample anti-aliasing */
    if (desc_.multiSampling.enabled && !hasSharedContext_)
    {
        /* Setup anti-aliasing after creating a standard render context. */
        if (SetupAntiAliasing())
        {
            /* Delete old standard render context */
            DeleteGLContext(stdRenderContext);

            /*
            For anti-aliasing we must recreate the window,
            because a pixel format can be choosen only once for a Win32 window,
            then update device context and pixel format
            */
            RecreateWindow();

            /* Create a new render context -> now with anti-aliasing pixel format */
            stdRenderContext = CreateGLContext(false, sharedContext);

            if (!stdRenderContext)
                Log::StdErr() << "failed to create multi-sample anti-aliasing" << std::endl;
        }
        else
        {
            /* Print warning and disable anti-aliasing */
            ErrAntiAliasingNotSupported();

            desc_.multiSampling.enabled = false;
            desc_.multiSampling.samples = 0;
        }
    }

    context_.hGLRC = stdRenderContext;

    /* Check for extended render context */
    if (desc_.profileOpenGL.extProfile && !hasSharedContext_)
    {
        /*
        Load profile selection extension (wglCreateContextAttribsARB) via current context,
        then create new context with extended settings.
        */
        if (wglCreateContextAttribsARB || LoadCreateContextProcs())
        {
            auto extRenderContext = CreateGLContext(true, sharedContext);
            
            if (extRenderContext)
            {
                /* Use the extended profile and delete the old standard render context */
                context_.hGLRC = extRenderContext;
                DeleteGLContext(stdRenderContext);
            }
            else
            {
                /* Print warning and disbale profile selection */
                Log::StdErr() << "failed to create extended OpenGL profile" << std::endl;
                desc_.profileOpenGL.extProfile = false;
            }
        }
        else
        {
            /* Print warning and disable profile settings */
            Log::StdErr() << "failed to select OpenGL profile" << std::endl;
            desc_.profileOpenGL.extProfile = false;
        }
    }

    /* Check if context creation was successful */
    if (!context_.hGLRC)
        throw std::runtime_error("failed to create OpenGL render context");

    if (wglMakeCurrent(context_.hDC, context_.hGLRC) != TRUE)
        throw std::runtime_error("failed to activate OpenGL render context");

    /*
    Share resources with previous render context (only for compatibility profile).
    -> Only do this, if this context has its own GL hardware context (hasSharedContext_ == false),
       but a shared render context was passed (sharedContext != null).
    */
    if (sharedContext && !hasSharedContext_ && !desc_.profileOpenGL.extProfile)
    {
        if (!wglShareLists(sharedContext->context_.hGLRC, context_.hGLRC))
            throw std::runtime_error("failed to share resources from OpenGL render context");
    }

    /* Query GL version of final render context */
    //QueryGLVersion();

    /* Setup v-sync interval */
    SetSwapInterval(desc_.vsync.enabled ? static_cast<int>(desc_.vsync.interval) : 0);
}

void Win32GLContext::DeleteContext()
{
    if (!hasSharedContext_)
    {
        /* Deactivate context before deletion */
        if (GLContext::Active() == this)
            Activate(false);

        DeleteGLContext(context_.hGLRC);
    }
}

void Win32GLContext::DeleteGLContext(HGLRC& renderContext)
{
    /* Delete GL render context */
    if (!wglDeleteContext(renderContext))
        Log::StdErr() << "failed to delete OpenGL render context" << std::endl;
    else
        renderContext = 0;
}

HGLRC Win32GLContext::CreateGLContext(bool useExtProfile, Win32GLContext* sharedContext)
{
    /* Create hardware render context */
    HGLRC renderContext = 0;

    if (!sharedContext || !sharedContext->context_.hGLRC /* || createOwnHardwareContext == true*/)
    {
        /* Create own hardware context */
        hasSharedContext_ = false;

        if (useExtProfile)
        {
            renderContext = CreateExtContextProfile(
                (sharedContext != nullptr ? sharedContext->context_.hGLRC : nullptr)
            );
        }
        else
            renderContext = CreateStdContextProfile();
    }
    else
    {
        /* Use shared render context */
        hasSharedContext_ = true;
        renderContext = sharedContext->context_.hGLRC;
    }
    
    if (!renderContext)
        return 0;
        
    /* Activate new render context */
    if (wglMakeCurrent(context_.hDC, renderContext) != TRUE)
    {
        /* Print error and delete unusable render context */
        Log::StdErr() << "failed to active OpenGL render context (wglMakeCurrent)" << std::endl;
        DeleteGLContext(renderContext);
        return 0;
    }

    /* Query GL version of current render context */
    //QueryGLVersion();

    return renderContext;
}

HGLRC Win32GLContext::CreateStdContextProfile()
{
    /* Create OpenGL "Compatibility Profile" render context */
    return wglCreateContext(context_.hDC);
}

static void ConvertGLVersion(const OpenGLVersion version, GLint& major, GLint& minor)
{
    if (version == OpenGLVersion::OpenGL_Latest)
    {
        major = 4;
        minor = 5;
    }
    else
    {
        auto ver = static_cast<int>(version);
        major = ver / 100;
        minor = (ver % 100) / 10;
    }
}

HGLRC Win32GLContext::CreateExtContextProfile(HGLRC sharedGLRC)
{
    bool useCoreProfile = desc_.profileOpenGL.coreProfile;
    
    /* Initialize GL version number */
    GLint major = 0, minor = 0;
    ConvertGLVersion(desc_.profileOpenGL.version, major, minor);

    /* Setup extended attributes to select the OpenGL profile */
    const int attribList[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB,  major,
        WGL_CONTEXT_MINOR_VERSION_ARB,  minor,
        #ifdef LLGL_DEBUG
        WGL_CONTEXT_FLAGS_ARB,          WGL_CONTEXT_DEBUG_BIT_ARB /*| WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB*/,
        #endif
        WGL_CONTEXT_PROFILE_MASK_ARB,   (useCoreProfile ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB),
        0, 0
    };

    /* Create OpenGL "Core Profile" or "Compatibility Profile" render context */
    HGLRC renderContext = wglCreateContextAttribsARB(context_.hDC, sharedGLRC, attribList);

    /* Check for errors */
    DWORD error = GetLastError();

    if (error == ERROR_INVALID_VERSION_ARB)
        Log::StdErr() << "invalid version for OpenGL profile" << std::endl;
    else if (error == ERROR_INVALID_PROFILE_ARB)
        Log::StdErr() << "invalid OpenGL profile" << std::endl;
    else
        return renderContext;

    return 0;
}

void Win32GLContext::SetupDeviceContextAndPixelFormat()
{
    /* Get device context from window */
    NativeHandle nativeHandle;
    window_.GetNativeHandle(&nativeHandle);
    context_.hDC = GetDC(nativeHandle.window);

    /* Select suitable pixel format */
    SelectPixelFormat();
}

void Win32GLContext::SelectPixelFormat()
{
    /* Setup pixel format attributes */
    const auto colorDepth = static_cast<BYTE>(desc_.videoMode.colorDepth);

    PIXELFORMATDESCRIPTOR formatDesc
    {
        sizeof(PIXELFORMATDESCRIPTOR),  // Structure size
        1,                              // Version number
        ( PFD_DRAW_TO_WINDOW |          // Format must support draw-to-window
          PFD_SUPPORT_OPENGL |          // Format must support OpenGL
          PFD_DOUBLEBUFFER   |          // Must support double buffering
          PFD_SWAP_EXCHANGE ),          // Hint to the driver to exchange the back- with the front buffer
        PFD_TYPE_RGBA,                  // Request an RGBA format
        colorDepth,                     // Select color bit depth (cColorBits)
        0, 0, 0, 0, 0, 0,               // Color bits ignored
        8,                              // Request an alpha buffer of 8 bits (cAlphaBits)
        0,                              // Shift bit ignored
        0,                              // No accumulation buffer
        0, 0, 0, 0,                     // Accumulation bits ignored
        24,                             // Z-Buffer bits (cDepthBits)
        1,                              // Stencil buffer bits (cStencilBits)
        0,                              // No auxiliary buffer
        0,                              // Main drawing layer (No longer used)
        0,                              // Reserved
        0, 0, 0                         // Layer masks ignored
    };
    
    /* Try to find suitable pixel format */
    const bool wantAntiAliasFormat = (desc_.multiSampling.enabled && !context_.pixelFormatsMS.empty());

    std::size_t msPixelFormatIndex = 0;
    bool wasStandardFormatUsed = false;

    while (true)
    {
        if (wantAntiAliasFormat && msPixelFormatIndex < GLPlatformContext::maxNumPixelFormatsMS)
        {
            /* Choose anti-aliasing pixel format */
            context_.pixelFormat = context_.pixelFormatsMS[msPixelFormatIndex++];
        }
        
        if (!context_.pixelFormat)
        {
            /* Choose standard pixel format */
            context_.pixelFormat = ChoosePixelFormat(context_.hDC, &formatDesc);
            
            if (wantAntiAliasFormat)
                ErrAntiAliasingNotSupported();
            
            wasStandardFormatUsed = true;
        }
        
        /* Check for errors */
        if (!context_.pixelFormat)
            throw std::runtime_error("failed to select pixel format");
        
        /* Set pixel format */
        auto wasFormatSelected = SetPixelFormat(context_.hDC, context_.pixelFormat, &formatDesc);
        
        if (!wasFormatSelected)
        {
            if (wasStandardFormatUsed)
                throw std::runtime_error("failed to set pixel format");
        }
        else
        {
            /* Format was selected -> quit with success */
            break;
        }
    }
}

bool Win32GLContext::SetupAntiAliasing()
{
    /*
    Load GL extension "wglChoosePixelFormatARB" to choose anti-aliasing pixel formats
    A valid (standard) GL context must be created at this time, before an extension can be loaded!
    */
    if (!wglChoosePixelFormatARB && !LoadPixelFormatProcs())
        return false;

    /* Setup pixel format for anti-aliasing */
    const auto queriedMultiSamples = desc_.multiSampling.samples;

    while (desc_.multiSampling.samples > 0)
    {
        float attribsFlt[] = { 0.0f, 0.0f };

        int attribsInt[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
            WGL_COLOR_BITS_ARB,     desc_.videoMode.colorDepth,
            WGL_ALPHA_BITS_ARB,     8,
            WGL_DEPTH_BITS_ARB,     24,
            WGL_STENCIL_BITS_ARB,   1,
            WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
            WGL_SAMPLE_BUFFERS_ARB, (desc_.multiSampling.enabled ? GL_TRUE : GL_FALSE),
            WGL_SAMPLES_ARB,        static_cast<int>(desc_.multiSampling.samples),
            0, 0
        };

        /* Choose new pixel format with anti-aliasing */
        UINT numFormats = 0;

        context_.pixelFormatsMS.resize(GLPlatformContext::maxNumPixelFormatsMS);

        int result = wglChoosePixelFormatARB(
            context_.hDC,
            attribsInt,
            attribsFlt,
            GLPlatformContext::maxNumPixelFormatsMS,
            context_.pixelFormatsMS.data(),
            &numFormats
        );

        context_.pixelFormatsMS.resize(numFormats);

        if (!result || numFormats < 1)
        {
            if (desc_.multiSampling.samples <= 0)
            {
                /* Lowest count of multi-samples reached -> return with error */
                return false;
            }

            /* Choose next lower count of multi-samples */
            --desc_.multiSampling.samples;
        }
        else
        {
            /* Found suitable pixel formats */
            break;
        }
    }

    /* Check if multi-sample count was reduced */
    if (desc_.multiSampling.samples < queriedMultiSamples)
    {
        Log::StdOut()
            << "reduced multi-samples for anti-aliasing from "
            << std::to_string(queriedMultiSamples) << " to "
            << std::to_string(desc_.multiSampling.samples) << std::endl;
    }

    /* Enable anti-aliasing */
    if (desc_.multiSampling.enabled)
        glEnable(GL_MULTISAMPLE);
    else
        glDisable(GL_MULTISAMPLE);

    return true;
}

void Win32GLContext::CopyPixelFormat(Win32GLContext& sourceContext)
{
    context_.pixelFormat    = sourceContext.context_.pixelFormat;
    context_.pixelFormatsMS = sourceContext.context_.pixelFormatsMS;
}

void Win32GLContext::RecreateWindow()
{
    /* Recreate window with current descriptor, then update device context and pixel format */
    window_.Recreate(window_.QueryDesc());
    SetupDeviceContextAndPixelFormat();
}


} // /namespace LLGL



// ================================================================================
