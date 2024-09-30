#pragma once

#include <filesystem>
#include <vector>

#include "Core/Enum.h"

namespace Voluma {

enum class ColorSpace {
    Linear,  ///< linear color space
    sRGB,    ///< sRGB color space, gamma corrected
};

VL_ENUM_INFO(ColorSpace,
             {{ColorSpace::Linear, "Linear"}, {ColorSpace::sRGB, "sRGB"}});
VL_ENUM_REGISTER(ColorSpace);

/**
 * @brief abstract image file interface
 */
class Image {
   public:
    Image(size_t width, size_t height, int channels,
          ColorSpace colorSpace = ColorSpace::Linear)
        : mWidth(int(width)),
          mHeight(int(height)),
          mChannels(channels),
          mColorSpace(colorSpace),
          mRawData(channels, std::vector<float>(width * height, 0.f)) {}
    Image(const Image& other) = delete;
    Image(Image&& other) noexcept
        : mWidth(other.mWidth),
          mHeight(other.mHeight),
          mChannels(other.mChannels),
          mRawData(std::move(other.mRawData)) {}
    void resize(size_t width, size_t height);
    /** Resize the number of channels of the image, keep the original data and
     * fill the new channels with 0.
     */
    void resizeChannels(int channels) {
        this->mChannels = channels;
        mRawData.resize(channels,
                        std::vector<float>(size_t(mWidth * mHeight), 0.f));
    }

    // Image properties
    [[nodiscard]] auto getWidth() const { return mWidth; }
    [[nodiscard]] auto getHeight() const { return mHeight; }
    [[nodiscard]] auto getChannels() const { return mChannels; }
    [[nodiscard]] auto getArea() const { return mWidth * mHeight; }
    [[nodiscard]] const auto& getRawData() const { return mRawData; }

    // Pixel accessing
    [[nodiscard]] float getPixel(int x, int y, int channels) const {
        return mRawData[channels][x + y * mWidth];
    }
    [[nodiscard]] float getPixel(int index, int channels) const {
        return mRawData[channels][index];
    }
    float& getPixel(int x, int y, int channels) {
        return mRawData[channels][x + y * mWidth];
    }
    float& getPixel(int index, int channels) {
        return mRawData[channels][index];
    }

    void setPixel(int x, int y, int channels, float value);
    void setPixel(int index, int channels, float value);

    void writeEXR(const std::filesystem::path& filename) const;
    void readEXR(const std::filesystem::path& filename);

    void writePNG(const std::filesystem::path& filename,
                  bool toSrgb = true) const;
    void readPNG(const std::filesystem::path& filename);

    /** Construct an image object from a file.
     */
    static Image load(const std::filesystem::path& filename);

   private:
    int mWidth;
    int mHeight;
    int mChannels;

    ColorSpace mColorSpace = ColorSpace::Linear;  ///< color space of the image
    std::vector<std::vector<float>>
        mRawData;  // raw data layers splitted by channels, width x height x
                   // channels, m_rawData[channel][x + y * width]
};

}  // namespace Voluma