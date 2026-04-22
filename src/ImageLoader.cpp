#include "ImageLoader.hpp"

#include <stdexcept>

const Image Image::readImage(std::span<const uint8_t> imgFileData) {
    Image image;
    uint8_t* data = stbi_load_from_memory(
        imgFileData.data(),
        imgFileData.size_bytes(),
        &image.width,
        &image.height,
        &image.channels,
        4
    );
    if (data == nullptr) {
        throw std::runtime_error("Can not read image file.");
    }
    image.channels = 4;
    image.data = ImageDataWrapper(data);
    return image;
}

std::span<const uint8_t> Image::rawData() const {
    return std::span<const uint8_t>(data.get(), width * height * channels);
}
