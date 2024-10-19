#pragma once
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dctk.h>
#include <fmt/core.h>

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>

#include "Core/Enum.h"
#include "Data/DcmParser.h"
#include "Patient.h"
#include "VolSlice.h"

namespace Voluma {

class VL_API VolData {
   public:
    struct ScanMeta {
        uint32_t rowCount, colCount; ///< Image size

        // Pixel metadata
        float pixelSpaceV,
            pixelSpaceH; ///< Vertical and Horizontal pixel spacing

        // Rescale
        float rescaleIntercept;
        float rescaleSlope;

        bool operator==(const ScanMeta& other);

        std::string toString() const;
    };

    VolData() = default;

    VolData(const VolData& d) = delete;

    static std::shared_ptr<VolData> loadFromDisk(
        const std::filesystem::path& folder);

    // Member getter
    const auto& getPatientData() { return mPatientData; }

    const auto& getScanMetaData() { return mMetaData; }

    auto getRowWidth() const { return mMetaData.rowCount; }

    auto getColWidth() const { return mMetaData.colCount; }

    uint32_t getVolumeSize() const {
        return getRowWidth() * getColWidth() * getSliceCount();
    }

    /** Save slice image to disk.
     */
    void saveSlice(const std::filesystem::path& filename, int index) const;
    int getSliceCount() const { return mVolumeSliceData.size(); }

    // TODO: Move otherwhere
    PatientData loadPatientData(const DcmParser& parser) const;
    ScanMeta loadScanMeta(const DcmParser& parser) const;

    bool verify(const DcmParser& parser) const;
    void addSlice(VolSlice&& slice);

    const std::vector<float> getBufferData() const { return mBufferData; }

    void finalize();

   private:
    // File metadata
    PatientData mPatientData; ///< Patient data
    ScanMeta mMetaData;       ///< Scanning metadata

    // CT Slices
    std::vector<VolSlice> mVolumeSliceData;

    std::vector<float> mBufferData;
};

} // namespace Voluma

VL_FMT(Voluma::VolData::ScanMeta)