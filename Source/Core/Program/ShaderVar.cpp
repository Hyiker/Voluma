#include "ShaderVar.h"

#include "Core/Buffer.h"
#include "Utils/Logger.h"
#include "slang-com-helper.h"
#include "slang-gfx.h"
#include "slang.h"
namespace Voluma {
ShaderVar::ShaderVar(gfx::IShaderObject* pShader)
    : mpShader(pShader), mpTypeLayout(pShader->getElementTypeLayout()) {}

using Kind = slang::TypeReflection::Kind;
ShaderVar ShaderVar::operator[](std::string_view name) const {
    if (!isValid()) logFatal("Shader var is not valid!");

    ShaderVar var;
    switch (mpTypeLayout->getKind()) {
        case Kind::None: {
            logFatal("ShaderVar::operator[]: No such field \"{}\"", name);
            break;
        }
        case Kind::Struct: {
            SlangInt fieldIndex =
                mpTypeLayout->findFieldIndexByName(name.data());
            if (fieldIndex == -1) {
                logFatal("ShaderVar::operator[]: No such field \"{}\"", name);
                break;
            }

            slang::VariableLayoutReflection* fieldLayout =
                mpTypeLayout->getFieldByIndex((unsigned int)fieldIndex);

            var = *this;

            var.mpTypeLayout = fieldLayout->getTypeLayout();

            var.mOffset.uniformOffset =
                mOffset.uniformOffset + fieldLayout->getOffset();
            var.mOffset.bindingRangeIndex =
                mOffset.bindingRangeIndex +
                mpTypeLayout->getFieldBindingRangeOffset(fieldIndex);

            var.mOffset.bindingArrayIndex = mOffset.bindingArrayIndex;
        } break;

        case Kind::ConstantBuffer:
        case Kind::ParameterBlock: {
            ShaderVar d(mpShader->getObject(mOffset));
            var = d[name];
        } break;
        case Kind::Scalar:
        case Kind::Matrix:
        case Kind::Vector: {
            logFatal("Reach Slang basic types");
        } break;
        case slang::TypeReflection::Kind::TextureBuffer: {
            logInfo("Binding TextureBuffer");
            break;
        }
        default:
            logFatal("Unexpected Slang type");
            break;
    }
    return var;
}

ShaderVar ShaderVar::operator[](size_t index) const {
    if (!isValid()) logFatal("");
    ShaderVar var;
    switch (mpTypeLayout->getKind()) {
        case slang::TypeReflection::Kind::Array: {
            slang::VariableLayoutReflection* fieldLayout =
                mpTypeLayout->getFieldByIndex(index);

            var = *this;

            var.mpTypeLayout = fieldLayout->getTypeLayout();

            var.mOffset.uniformOffset =
                mOffset.uniformOffset + fieldLayout->getOffset();
            var.mOffset.bindingRangeIndex =
                mOffset.bindingRangeIndex +
                mpTypeLayout->getFieldBindingRangeOffset(index);

            var.mOffset.bindingArrayIndex = mOffset.bindingArrayIndex;
        } break;
        default:
            logFatal("Unexpected Slang type");
            break;
    }
    return var;
}

void ShaderVar::setBlob(void const* data, size_t size) const {
    if (!SLANG_SUCCEEDED(mpShader->setData(mOffset, data, size))) {
        logError("ShaderVar: Error setBlob");
    }
}

void ShaderVar::setImpl(const Texture& texture) const {
    mpShader->setResource(mOffset, texture.getView());
}

void ShaderVar::setImpl(const Buffer& buffer) const {
    mpShader->setResource(mOffset, buffer.view);
}

void ShaderVar::setImpl(gfx::ISamplerState* sampler) const {
    mpShader->setSampler(mOffset, sampler);
}

} // namespace Voluma