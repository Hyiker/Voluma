#include "VolSlice.h"

namespace Voluma {
VolSlice::VolSlice(const DcmParser& parser) {
    mRawData = parser.getU16Array(DCM_PixelData);

    for (auto v : mRawData) {
        mMaxPixelValue = std::max(v, mMaxPixelValue);
        mMinPixelValue = std::min(v, mMinPixelValue);
    }

    mThickness = (float)parser.getF64(DCM_SliceThickness);
    mLocation = (float)parser.getF64(DCM_SliceLocation);
}

VolSlice::VolSlice(VolSlice&& other) noexcept
    : mRawData(other.mRawData),
      mThickness(other.mThickness),
      mLocation(other.mLocation),
      mMaxPixelValue(other.mMaxPixelValue),
      mMinPixelValue(other.mMinPixelValue) {}
} // namespace Voluma