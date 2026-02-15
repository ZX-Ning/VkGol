#ifndef IMAGELOADER_HPP
#define IMAGELOADER_HPP

#include <stb_image.h>

#include <cstdint>
#include <memory>
#include <span>

struct Image {
private:
    struct StbImageDeleter {
        void operator()(uint8_t* data) const noexcept {
            if (data) {
                stbi_image_free(data);
            }
        }
    };
    typedef std::unique_ptr<uint8_t, StbImageDeleter> ImageDataWrapper;
    Image() = default;
    ImageDataWrapper data;

public:
    int width;
    int height;
    int channels;
    static const Image readImage(std::span<const uint8_t> imgFileData);
    std::span<const uint8_t> rawData() const;
};

#endif  // IMAGELOADER_HPP
