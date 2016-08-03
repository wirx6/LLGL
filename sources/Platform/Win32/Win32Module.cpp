/*
 * Win32Module.cpp
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "Win32Module.h"


namespace LLGL
{


std::string Module::GetModuleFilename(std::string moduleName)
{
    /* Extend module name to Win32 dynamic link library name (DLL) */
    #ifdef LLGL_DEBUG
    moduleName += "D";
    #endif
    return "LLGL_" + moduleName + ".dll";
}

bool Module::IsAvailable(const std::string& moduleFilename)
{
    /* Check if Win32 dynamic link library can be loaded properly */
    auto handle = LoadLibraryA(moduleFilename.c_str());
    if (handle)
    {
        FreeLibrary(handle);
        return true;
    }
    return false;
}

std::unique_ptr<Module> Module::Load(const std::string& moduleFilename)
{
    return std::unique_ptr<Module>(new Win32Module(moduleFilename));
}

Win32Module::Win32Module(const std::string& moduleFilename)
{
    /* Open Win32 dynamic link library (DLL) */
    handle_ = LoadLibraryA(moduleFilename.c_str());

    /* Check if loading has failed */
    if (!handle_)
        throw std::runtime_error("failed to load dynamic link library (DLL) \"" + moduleFilename + "\"");
}

Win32Module::~Win32Module()
{
    FreeLibrary(handle_);
}

void* Win32Module::LoadProcedure(const std::string& procedureName)
{
    /* Get procedure address from library module and return it as raw-pointer */
    auto procAddr = GetProcAddress(handle_, procedureName.c_str());
    return reinterpret_cast<void*>(procAddr);
}


} // /namespace LLGL



// ================================================================================