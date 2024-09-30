#pragma once
#include <cstdint>
#include <vector>

#include "DcmParser.h"
namespace Voluma {
class VolSlice {
   public:
    VolSlice() = default;
    VolSlice(const VolSlice&) = delete;
    VolSlice(const DcmParser& parser);
    VolSlice(VolSlice&& other) noexcept;

    VolSlice& operator=(const VolSlice&) = delete;
    VolSlice& operator=(VolSlice&&) = default;

   private:
    std::vector<uint16_t> mRawData;  ///< CT data stored in Hounsfield Unit(HU)
    // Slice metadata
    float mThickness;  ///< 3D slice image thickness
    float mLocation;   ///< Depth location of the slice, could be negative

    uint16_t mMaxPixelValue = 0u;
    uint16_t mMinPixelValue = std::numeric_limits<uint16_t>::max();

    friend class VolData;
};
}  // namespace Voluma