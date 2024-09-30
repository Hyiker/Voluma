#include "VolData.h"

#include <dcmtk/dcmdata/dcpixel.h>
#include <dcmtk/dcmimgle/dipixel.h>
#include <dcmtk/dcmimgle/diutils.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/error.h"
#include "data/DcmParser.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include "utils/image.h"
#include "utils/logger.h"
namespace Voluma {

std::shared_ptr<VolData> VolData::loadFromDisk(const std::string& filename) {
    DcmFileFormat dfile;

    OFCondition result = dfile.loadFile(filename.c_str());
    if (result.bad()) logError("Failed to load file.");

    return std::make_shared<VolData>(dfile);
}

void VolData::save(const std::filesystem::path& filename) const {
    if (filename.extension() != ".exr") {
        logFatal("VolData::save only support Exr format.");
    }

    Image image(getColWidth(), getRowWidth(), 1, ColorSpace::Linear);
    for (int i = 0; i < image.getArea(); i++) {
        float normalizedVal =
            float(mVolumeSliceData[i]) / float(mMaxPixelValue - mMinPixelValue);
        image.setPixel(i, 0, normalizedVal);
    }
    image.writeEXR(filename);
}

VolData::VolData(DcmFileFormat& dcmFile) {
    auto* pMeta = dcmFile.getMetaInfo();
    auto* pDataset = dcmFile.getDataset();

    VL_ASSERT(pDataset != nullptr);
    VL_ASSERT(pMeta != nullptr);

    DcmParser parser(pDataset);

    // Read and load metadata
    loadPatientData(parser);
    loadScanMeta(parser);

    // Read slice images
    loadImage(parser);
}

void VolData::loadPatientData(const DcmParser& parser) {
    // Initialize patient data
    std::string id, name, birthDate;

    id = parser.getString(DCM_PatientID);
    name = parser.getString(DCM_PatientName);
    birthDate = parser.getString(DCM_PatientBirthDate);

    std::string genderStr = parser.getString(DCM_PatientSex);
    auto gender = (genderStr == "M")   ? Gender::Male
                  : (genderStr == "F") ? Gender::Female
                                       : Gender::Others;

    mPatientData.id = id;
    mPatientData.name = name;
    mPatientData.birthDate = birthDate;
    mPatientData.gender = gender;
}

void VolData::loadScanMeta(const DcmParser& parser) {
    uint32_t rows, cols;
    float pixelSpaceV, pixelSpaceH;
    float sliceThickness, sliceLocation;
    float rescaleIntercept, rescaleSlope;

    rows = parser.getU16(DCM_Rows);
    cols = parser.getU16(DCM_Columns);

    std::string pixelSpace = parser.getString(DCM_PixelSpacing);
    auto segEnd = pixelSpace.find('\\');
    pixelSpaceH = std::stof(pixelSpace.substr(0, segEnd));
    pixelSpaceV = std::stof(pixelSpace.substr(segEnd + 1));

    sliceThickness = (float)parser.getF64(DCM_SliceThickness);
    sliceLocation = (float)parser.getF64(DCM_SliceLocation);

    rescaleIntercept = (float)parser.getF64(DCM_RescaleIntercept);
    rescaleSlope = (float)parser.getF64(DCM_RescaleSlope);

    mMetaData.rowCount = rows;
    mMetaData.colCount = cols;
    mMetaData.pixelSpaceV = pixelSpaceV;
    mMetaData.pixelSpaceH = pixelSpaceH;
    mMetaData.sliceThickness = sliceThickness;
    mMetaData.sliceLocation = sliceLocation;
    mMetaData.rescaleIntercept = rescaleIntercept;
    mMetaData.rescaleSlope = rescaleSlope;
}

void VolData::loadImage(const DcmParser& parser) {
    mVolumeSliceData = parser.getU16Array(DCM_PixelData);

    for (auto v : mVolumeSliceData) {
        mMaxPixelValue = std::max(v, mMaxPixelValue);
        mMinPixelValue = std::min(v, mMinPixelValue);
    }
}

}  // namespace Voluma