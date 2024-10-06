#include "ShaderVar.h"

#include "Utils/Logger.h"
namespace Voluma {
ShaderVar::ShaderVar(gfx::IShaderObject* pShader)
    : mpShader(pShader), mTypeLayout(pShader->getElementTypeLayout()) {}

ShaderVar ShaderVar::operator[](std::string_view name) const {
    if (!isValid()) logFatal("");
    ShaderVar var;
    switch (mTypeLayout->getKind()) {
        case slang::TypeReflection::Kind::Struct: {
            // We start by looking up the index of a field matching `name`.
            //
            // If there is no such field, we have an error.
            //
            SlangInt fieldIndex =
                mTypeLayout->findFieldIndexByName(name.data());
            if (fieldIndex == -1) break;

            // Once we know the index of the field being referenced,
            // we create a cursor to point at the field, based on
            // the offset information already in this cursor, plus
            // offsets derived from the field's layout.
            //
            slang::VariableLayoutReflection* fieldLayout =
                mTypeLayout->getFieldByIndex((unsigned int)fieldIndex);

            var = *this;

            var.mTypeLayout = fieldLayout->getTypeLayout();

            var.mOffset.uniformOffset =
                mOffset.uniformOffset + fieldLayout->getOffset();
            var.mOffset.bindingRangeIndex =
                mOffset.bindingRangeIndex +
                mTypeLayout->getFieldBindingRangeOffset(fieldIndex);

            var.mOffset.bindingArrayIndex = mOffset.bindingArrayIndex;
        } break;

        // In some cases the user might be trying to acess a field by name
        // from a cursor that references a constant buffer or parameter block,
        // and in these cases we want the access to Just Work.
        //
        case slang::TypeReflection::Kind::ConstantBuffer:
        case slang::TypeReflection::Kind::ParameterBlock: {
            // We basically need to "dereference" the current cursor
            // to go from a pointer to a constant buffer to a pointer
            // to the *contents* of the constant buffer.
            //
            ShaderVar d(mpShader->getObject(mOffset));
            var = d[name];
        } break;
        case slang::TypeReflection::Kind::Scalar:
        case slang::TypeReflection::Kind::Matrix:
        case slang::TypeReflection::Kind::Vector: {
            logFatal("Reach Slang basic types");
        } break;
        default:
            logFatal("Unexpected Slang type");
            break;
    }
    return var;
}

ShaderVar ShaderVar::operator[](size_t index) const {
    if (!isValid()) logFatal("");
    ShaderVar var;
    switch (mTypeLayout->getKind()) {
        case slang::TypeReflection::Kind::Array: {
            slang::VariableLayoutReflection* fieldLayout =
                mTypeLayout->getFieldByIndex(index);

            var = *this;

            var.mTypeLayout = fieldLayout->getTypeLayout();

            var.mOffset.uniformOffset =
                mOffset.uniformOffset + fieldLayout->getOffset();
            var.mOffset.bindingRangeIndex =
                mOffset.bindingRangeIndex +
                mTypeLayout->getFieldBindingRangeOffset(index);

            var.mOffset.bindingArrayIndex = mOffset.bindingArrayIndex;
        } break;
        default:
            logFatal("Unexpected Slang type");
            break;
    }
    return var;
}

void ShaderVar::setBlob(void const* data, size_t size) const {
    mpShader->setData(mOffset, data, size);
}
} // namespace Voluma