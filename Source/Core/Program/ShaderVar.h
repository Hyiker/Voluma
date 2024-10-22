#pragma once
#include <slang-gfx.h>

#include <string_view>

#include "Core/Buffer.h"
#include "Core/Macros.h"
#include "Core/Texture.h"
namespace Voluma {
class VL_API ShaderVar {
   public:
    /** Construct root shader variable.
     */
    ShaderVar(gfx::IShaderObject* pShader);

    ShaderVar() = default;

    ShaderVar operator[](std::string_view name) const;

    ShaderVar operator[](size_t index) const;

    void setBlob(void const* data, size_t size) const;

    template <typename T>
    void setBlob(const T& val) const {
        setBlob(&val, sizeof(val));
    }

    template <typename T>
    void operator=(const T& val) const {
        setImpl(val);
    }

    bool isValid() const { return mpShader != nullptr; }

   private:
    void setImpl(const Texture& texture) const;

    void setImpl(const Buffer& buffer) const;
    
    void setImpl(gfx::ISamplerState* sampler) const;

    template <typename T>
    void setImpl(const T& val) const;

    gfx::IShaderObject* mpShader;
    slang::TypeLayoutReflection* mpTypeLayout = nullptr;

    gfx::ShaderOffset mOffset;
};

template <typename T>
void ShaderVar::setImpl(const T& val) const {
    setBlob(val);
}

} // namespace Voluma