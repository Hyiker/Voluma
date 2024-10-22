#include "VolData.h"

#include <dcmtk/dcmdata/dcpixel.h>
#include <dcmtk/dcmimgle/dcmImage.h>
#include <dcmtk/dcmimgle/dipixel.h>
#include <dcmtk/dcmimgle/diutils.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <execution>
#include <filesystem>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Core/Error.h"
#include "Data/DcmParser.h"
#include "Utils/Image.h"
#include "Utils/Logger.h"
#include "fmt/format.h"
namespace Voluma {
using ScanMeta = VolData::ScanMeta;
bool ScanMeta::operator==(const ScanMeta& other) {
    return rowCount == other.rowCount && colCount == other.colCount &&
           //? Should we verify float here
           pixelSpaceV == other.pixelSpaceV &&
           pixelSpaceH == other.pixelSpaceH &&
           rescaleIntercept == other.rescaleIntercept &&
           rescaleSlope == other.rescaleSlope;
}

std::string ScanMeta::toString() const {
    return fmt::format("ScanMeta(res = ({}, {}), pixelSpace = ({}, {}))",
                       rowCount, colCount, pixelSpaceH, pixelSpaceV);
}

std::shared_ptr<VolData> VolData::loadFromDisk(
    const std::filesystem::path& folder) {
    auto pVolData = std::make_shared<VolData>();

    std::vector<std::string> filePaths;
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        auto fn = entry.path();
        if (entry.is_regular_file() && fn.extension() == ".dcm") {
            filePaths.push_back((folder / fn.filename()).string());
        }
    }

    std::mutex addSliceMutex;

    auto loadDcmData = [&](const std::string& fn, bool isFirst = false) {
        DcmFileFormat dfile;
        OFCondition result = dfile.loadFile(fn.c_str());

        if (result.bad()) logError("Failed to load slice file {}.", fn);

        auto* pDataset = dfile.getDataset();
        DcmParser parser(pDataset);

        if (isFirst) {
            // Read meta data from the first .dcm file
            pVolData->mPatientData = pVolData->loadPatientData(parser);
            pVolData->mMetaData = pVolData->loadScanMeta(parser);
        } else {
            // Otherwise, verify the metadata and patient data
            if (!pVolData->verify(parser))
                logError(
                    "Slice file {} has divergent meta data with previous "
                    "ones.",
                    fn);
        }
        {
            std::unique_lock<std::mutex> lck(addSliceMutex);
            pVolData->addSlice(VolSlice(parser));
        }
    };

    loadDcmData(filePaths[0], true);
    std::for_each(std::execution::par_unseq, filePaths.begin() + 1,
                  filePaths.end(), loadDcmData);

    pVolData->finalize();

    return pVolData;
}

void VolData::saveSlice(const std::filesystem::path& filename,
                        int index) const {
    if (filename.extension() != ".exr") {
        logFatal("VolData::save only support Exr format.");
    }

    VL_ASSERT(index < mVolumeSliceData.size());
    const VolSlice& slice = mVolumeSliceData[index];

    Image image(getColWidth(), getRowWidth(), 1, ColorSpace::Linear);
    for (int i = 0; i < image.getArea(); i++) {
        float normalizedVal =
            float(slice.mRawData[i]) /
            float(slice.mMaxPixelValue - slice.mMinPixelValue);
        image.setPixel(i, 0, normalizedVal);
    }
    image.writeEXR(filename);
}

PatientData VolData::loadPatientData(const DcmParser& parser) const {
    // Initialize patient data
    std::string id, name, birthDate;

    id = parser.getString(DCM_PatientID);
    name = parser.getString(DCM_PatientName);
    birthDate = parser.getString(DCM_PatientBirthDate);

    std::string genderStr = parser.getString(DCM_PatientSex);
    auto gender = (genderStr == "M")   ? Gender::Male
                  : (genderStr == "F") ? Gender::Female
                                       : Gender::Others;

    PatientData pd;
    pd.id = id;
    pd.name = name;
    pd.birthDate = birthDate;
    pd.gender = gender;
    return pd;
}

ScanMeta VolData::loadScanMeta(const DcmParser& parser) const {
    uint32_t rows, cols;
    float pixelSpaceV, pixelSpaceH;
    float rescaleIntercept, rescaleSlope;

    rows = parser.getU16(DCM_Rows);
    cols = parser.getU16(DCM_Columns);

    std::string pixelSpace = parser.getString(DCM_PixelSpacing);
    auto segEnd = pixelSpace.find('\\');
    pixelSpaceH = std::stof(pixelSpace.substr(0, segEnd));
    pixelSpaceV = std::stof(pixelSpace.substr(segEnd + 1));

    rescaleIntercept = (float)parser.getF64(DCM_RescaleIntercept);
    rescaleSlope = (float)parser.getF64(DCM_RescaleSlope);

    ScanMeta meta;
    meta.rowCount = rows;
    meta.colCount = cols;
    meta.pixelSpaceV = pixelSpaceV;
    meta.pixelSpaceH = pixelSpaceH;
    meta.rescaleIntercept = rescaleIntercept;
    meta.rescaleSlope = rescaleSlope;
    return meta;
}

void VolData::addSlice(VolSlice&& slice) {
    mVolumeSliceData.emplace_back(std::move(slice));
}

bool VolData::verify(const DcmParser& parser) const {
    bool isSame = true;
    // Verify patient id
    isSame |= mPatientData.id == parser.getString(DCM_PatientID);

    auto meta = loadScanMeta(parser);
    // Verify slice meta data
    isSame |= mMetaData == meta;

    return isSame;
}

void VolData::finalize() {
    // Reorder slices by their location
    std::sort(mVolumeSliceData.begin(), mVolumeSliceData.end(),
              [](const VolSlice& s1, const VolSlice& s2) {
                  return s1.mLocation <= s2.mLocation;
              });

    mBufferData.reserve(getVolumeSize());

    for (const auto& slice : mVolumeSliceData) {
        std::transform(slice.mRawData.begin(), slice.mRawData.end(),
                       std::back_inserter(mBufferData),
                       [](uint16_t v) { return float(v); });

        mMinValue = std::min(slice.mMinPixelValue, mMinValue);
        mMaxValue = std::max(slice.mMaxPixelValue, mMaxValue);
    }

    logInfo("Min val: {}, max val: {}", mMinValue, mMaxValue);
    if (mBufferData.size() != getVolumeSize()) {
        logError("Bad buffer data size!");
    }
    mBufferData.shrink_to_fit();
}

} // namespace Voluma