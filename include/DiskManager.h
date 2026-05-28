#pragma once
#include "Page.h"
#include <string>
#include <fstream>
#include <stdexcept>

class DiskManager {
public:
    explicit DiskManager(const std::string& db_file);
    ~DiskManager();

    void     writePage(uint32_t page_id, const Page& page);
    void     readPage(uint32_t page_id, Page& page);
    uint32_t allocatePage();          // returns new page_id
    uint32_t pageCount() const;
    void     flush();

private:
    std::fstream  file_;
    std::string   filename_;
    uint32_t      num_pages_{0};

    uint64_t pageOffset(uint32_t page_id) const {
        return static_cast<uint64_t>(page_id) * PAGE_SIZE;
    }
};
