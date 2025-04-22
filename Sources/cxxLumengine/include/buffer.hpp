#ifndef LE_BUFFER_HPP
#define LE_BUFFER_HPP

#include <cstddef>
#include <string>

class Buffer;
using BufferPtr = std::shared_ptr<Buffer>;
class Buffer final {
    std::unique_ptr<char[]> m_ptr;
    std::size_t m_size;
    std::size_t m_pos { 0 };  // Current position in buffer for read/write operations

public:
    // An empty buffer declaration
    Buffer() : m_ptr(nullptr), m_size(0) {}

    // Constructor with max size
    explicit Buffer(const std::size_t max_size)
        : m_ptr(std::make_unique<char[]>(max_size)), m_size(max_size) {}

    // Constructor that consumes a std::string
    explicit Buffer(std::string&& str)
        : m_ptr(std::make_unique<char[]>(str.size())), m_size(str.size()) {
        std::ranges::move(str, m_ptr.get());
    }

    // Disable copy constructor and copy assignment operator
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // Move constructor
    Buffer(Buffer&& other) noexcept
        : m_ptr(std::move(other.m_ptr)), m_size(other.m_size) {
        other.m_size = 0;
    }

    // Move assignment operator
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            m_ptr = std::move(other.m_ptr);
            m_size = other.m_size;
            other.m_size = 0;
        }
        return *this;
    }

    // Getters
    [[nodiscard]] char* pointer() const noexcept { return m_ptr.get(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_size; }
    [[nodiscard]] bool empty() const noexcept { return m_size == 0; }
    [[nodiscard]] std::string to_string() const {
        return m_size ? std::string(m_ptr.get(), m_size) : std::string();
    }

    // Helper pointer constructors
    BufferPtr static shared() {
        return std::make_shared<Buffer>();
    }

    BufferPtr static shared(const std::size_t max_size) {
        return std::make_shared<Buffer>(max_size);
    }

    BufferPtr static shared(std::string&& str) {
        return std::make_shared<Buffer>(std::move(str));
    }

    // Reset position to beginning of buffer
    void reset() noexcept { m_pos = 0; }

    // Get remaining bytes available to read/write
    [[nodiscard]] std::size_t remaining() const noexcept { return m_size - m_pos; }

    // Get current position
    [[nodiscard]] std::size_t position() const noexcept { return m_pos; }

    // Write data to buffer at current position
    std::size_t write(const char* data, const std::size_t length) noexcept {
        const std::size_t available = remaining();
        const std::size_t to_write = std::min(available, length);
        if (to_write > 0) {
            std::memcpy(m_ptr.get() + m_pos, data, to_write);
            m_pos += to_write;
        }
        return to_write;
    }

    // Read data from buffer at current position
    std::size_t read(char* data, const std::size_t length) noexcept {
        const std::size_t available = remaining();
        const std::size_t to_read = std::min(available, length);
        if (to_read > 0) {
            std::memcpy(data, m_ptr.get() + m_pos, to_read);
            m_pos += to_read;
        }
        return to_read;
    }

    // Peek at data without advancing position
    std::size_t peek(char* data, const std::size_t length) const noexcept {
        const std::size_t available = remaining();
        const std::size_t to_read = std::min(available, length);
        if (to_read > 0) {
            std::memcpy(data, m_ptr.get() + m_pos, to_read);
        }
        return to_read;
    }

    // Set position (with bounds checking)
    bool seek(const std::size_t pos) noexcept {
        if (pos <= m_size) {
            m_pos = pos;
            return true;
        }
        return false;
    }
};

#endif // LE_BUFFER_HPP
