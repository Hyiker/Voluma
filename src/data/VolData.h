
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dctk.h>
#include <fmt/core.h>

#include <cstdint>
#include <limits>
#include <string>

#include "Patient.h"
#include "core/enum.h"
#include "data/DcmParser.h"

namespace Voluma {

class VolData {
   public:
    struct ScanMeta {
        uint32_t rowCount, colCount;  ///< Image size

        // Pixel metadata
        float pixelSpaceV,
            pixelSpaceH;  ///< Vertical and Horizontal pixel spacing

        // Slice metadata
        float sliceThickness;  ///< 3D slice image thickness
        float
            sliceLocation;  ///< Depth location of the slice, could be negative

        // Rescale
        float rescaleIntercept;
        float rescaleSlope;
    };

    VolData() = default;
    VolData(DcmFileFormat& dcmFile);
    VolData(const VolData& d) = delete;

    static std::shared_ptr<VolData> loadFromDisk(const std::string& filename);

    // Member getter
    const auto& getPatientData() { return mPatientData; }

    auto getRowWidth() const { return mMetaData.rowCount; }

    auto getColWidth() const { return mMetaData.colCount; }

    void save(const std::filesystem::path& filename) const;

   private:
    void loadPatientData(const DcmParser& parser);

    void loadScanMeta(const DcmParser& parser);

    void loadImage(const DcmParser& parser);

    // File metadata
    PatientData mPatientData;  ///< Patient data
    ScanMeta mMetaData;        ///< Scanning metadata

    // Volume data
    std::vector<uint16_t> mVolumeSliceData;
    uint16_t mMaxPixelValue = 0u;
    uint16_t mMinPixelValue = std::numeric_limits<uint16_t>::max();
};

}  // namespace Voluma

template <>
class fmt::formatter<Voluma::PatientData> : formatter<std::string> {
   public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Voluma::PatientData& d, format_context& ctx) const {
        return formatter<std::string>::format(d.toString(), ctx);
    }
};