#include "Program.h"

#include <slang-com-ptr.h>
#include <slang-gfx.h>
#include <slang.h>

#include <vector>

#include "Core/Error.h"
#include "Utils/Logger.h"
namespace Voluma {

void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob) {
    if (diagnosticsBlob != nullptr) {
        logError("Program: {}",
                 (const char*)diagnosticsBlob->getBufferPointer());
    }
}

std::vector<std::string> getShaderSearchDirectories() {
    return {std::filesystem::current_path().string()};
}

ProgramManager::ProgramManager(std::shared_ptr<Device> device)
    : mpDevice(device) {}

SlangCompileTarget getSlangCompileTarget() {
#if VL_WINDOWS
    return SlangCompileTarget::SLANG_DXIL;
#elif VL_MACOSX
    return SlangCompileTarget::SLANG_METAL;
#else
    return SlangCompileTarget::SLANG_SPIRV;
#endif
}

Slang::ComPtr<gfx::IShaderProgram> ProgramManager::createProgram(
    std::string_view filePath,
    std::vector<ProgramEntryPoint> entryPoints) const {
    // Create session
    slang::SessionDesc sessionDesc;

    std::vector<std::string> searchPaths;
    std::vector<const char*> slangSearchPaths;

    for (auto& path : getShaderSearchDirectories()) {
        searchPaths.push_back(path);
        slangSearchPaths.push_back(searchPaths.back().data());
    }
    sessionDesc.searchPaths = slangSearchPaths.data();
    sessionDesc.searchPathCount = (SlangInt)slangSearchPaths.size();

    slang::TargetDesc targetDesc;
    targetDesc.format = getSlangCompileTarget();
    targetDesc.profile = mpDevice->getGlobalSession()->findProfile("sm_6_5");
    logInfo("Profile id: {}", int(targetDesc.profile));
    targetDesc.forceGLSLScalarBufferLayout = true;
    sessionDesc.targetCount = 1;
    sessionDesc.targets = &targetDesc;
    targetDesc.flags |= SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

    Slang::ComPtr<slang::ISession> pSlangSession;
    mpDevice->getGlobalSession()->createSession(sessionDesc,
                                                pSlangSession.writeRef());

    Slang::ComPtr<slang::IBlob> diagnosticsBlob;
    slang::IModule* module =
        pSlangSession->loadModule(filePath.data(), diagnosticsBlob.writeRef());
    diagnoseIfNeeded(diagnosticsBlob);
    VL_ASSERT(module != nullptr);

    std::vector<slang::IComponentType*> componentTypes;
    componentTypes.push_back(module);

    for (auto& ep : entryPoints) {
        Slang::ComPtr<slang::IEntryPoint> stageEntryPoint;
        module->findEntryPointByName(ep.first.c_str(),
                                     stageEntryPoint.writeRef());
        componentTypes.push_back(stageEntryPoint);
    }

    Slang::ComPtr<slang::IComponentType> linkedProgram;
    pSlangSession->createCompositeComponentType(
        componentTypes.data(), componentTypes.size(), linkedProgram.writeRef(),
        diagnosticsBlob.writeRef());
    VL_ASSERT(linkedProgram != nullptr);
    diagnoseIfNeeded(diagnosticsBlob);

    gfx::IShaderProgram::Desc programDesc = {};
    programDesc.slangGlobalScope = linkedProgram;

    return mpDevice->getGfxDevice()->createProgram(programDesc);
}
} // namespace Voluma