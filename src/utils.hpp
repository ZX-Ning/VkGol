#ifndef UTILS_HPP
#define UTILS_HPP

#include <GLFW/glfw3.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <vector>

constexpr bool IS_DEBUG =
#ifdef _DEBUG
    true;
#else
    false;
#endif

#define DISABLE_COPY(ClassName)                      \
public:                                              \
    ClassName(const ClassName&) = delete;            \
    ClassName& operator=(const ClassName&) = delete;

inline std::vector<char> readFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    size_t fileSize = std::filesystem::file_size(filePath);
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

template <class T = int>
struct Size2D {
    static_assert(std::is_integral_v<T>);
    T width;
    T height;
    template <class U>
    operator Size2D<U>() {
        return {
            static_cast<U>(this->width),
            static_cast<U>(this->height)
        };
    }
};

struct RGBAColor : public std::array<float, 4> {
    float* getRaw() {
        return this->data();
    }

    RGBAColor(float r, float g, float b, float a)
        : std::array<float, 4>{r, g, b, a} {}

    RGBAColor() : RGBAColor(0.f, 0.f, 0.f, 1.f) {}

    static float srgbToLinear(float color) {
        if (color <= 0.04045f) {
            return color / 12.92f;
        }
        else {
            return std::pow((color + 0.055f) / 1.055f, 2.4f);
        }
    }

    RGBAColor srgbToLinear() {
        return {
            srgbToLinear((*this)[0]),
            srgbToLinear((*this)[1]),
            srgbToLinear((*this)[2]),
            (*this)[3]
        };
    }
};

class GLFWwindow;
struct GLFWwindowDeleter {
    void operator()(GLFWwindow* window) {
        if (window != nullptr) {
            glfwDestroyWindow(window);
        }
    }
};
typedef std::unique_ptr<GLFWwindow, GLFWwindowDeleter> GLFWwindowWrapper;

inline uint64_t getTimestampMs() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = duration_cast<milliseconds>(now.time_since_epoch());
    return duration.count();
}

template <class T>
inline size_t getVectorSize(const std::vector<T>& vec) {
    return vec.size() * sizeof(T);
}

#endif  // UTILS_HPP
