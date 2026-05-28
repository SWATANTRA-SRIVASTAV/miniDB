#pragma once
#include <cstdint>
#include <cstring>
#include <array>

static constexpr uint32_t PAGE_SIZE     = 4096;   // 4KB pages (matches OS page size)
static constexpr uint32_t MAX_PAGES     = 65536;  // max DB size = 256MB
static constexpr uint32_t INVALID_PAGE  = UINT32_MAX;

// Raw 4KB block — what gets written to / read from disk
struct Page {
    std::array<uint8_t, PAGE_SIZE> data{};

    void     clear()                              { data.fill(0); }
    uint8_t* raw()                                { return data.data(); }
    const uint8_t* raw() const                    { return data.data(); }

    // Typed read/write helpers (no alignment issues)
    template<typename T>
    T read(uint32_t offset) const {
        T val;
        std::memcpy(&val, data.data() + offset, sizeof(T));
        return val;
    }
    template<typename T>
    void write(uint32_t offset, const T& val) {
        std::memcpy(data.data() + offset, &val, sizeof(T));
    }
};
