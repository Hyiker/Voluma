#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcmetinf.h>
#include <dcmtk/dcmdata/dctk.h>
#include <fmt/core.h>

#include <string>

#include "Patient.h"
#include "core/enum.h"
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
    };

    VolData() = default;
    VolData(DcmFileFormat& dcmFile);
    VolData(const VolData& d) = delete;

    static std::shared_ptr<VolData> loadFromDisk(const std::string& filename);

    // Member getter
    const auto& getPatientData() { return mPatientData; }

   private:
    void loadPatientData(DcmDataset* pDataset);

    void loadScanMeta(DcmDataset* pDataset);

    // File metadata
    PatientData mPatientData;  ///< Patient data
    ScanMeta mMetaData;        ///< Scanning metadata
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