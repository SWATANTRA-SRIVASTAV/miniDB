#include "DiskManager.h"
#include <iostream>

DiskManager::DiskManager(const std::string& db_file) : filename_(db_file) {
    file_.open(db_file, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
        file_.open(db_file, std::ios::in | std::ios::out |
                            std::ios::binary | std::ios::trunc);
    }
    if (!file_.is_open())
        throw std::runtime_error("Cannot open DB file: " + db_file);

    file_.seekg(0, std::ios::end);
    auto size = file_.tellg();
    num_pages_ = (size > 0) ? static_cast<uint32_t>(size / PAGE_SIZE) : 0;
}

DiskManager::~DiskManager() {
    if (file_.is_open()) { file_.flush(); file_.close(); }
}

void DiskManager::writePage(uint32_t page_id, const Page& page) {
    if (page_id >= MAX_PAGES)
        throw std::runtime_error("Page ID exceeds maximum");
    file_.seekp(pageOffset(page_id));
    file_.write(reinterpret_cast<const char*>(page.raw()), PAGE_SIZE);
    if (!file_) throw std::runtime_error("Write failed for page " +
                                          std::to_string(page_id));
    if (page_id >= num_pages_) num_pages_ = page_id + 1;
}

void DiskManager::readPage(uint32_t page_id, Page& page) {
    if (page_id >= num_pages_)
        throw std::runtime_error("Page " + std::to_string(page_id) +
                                  " does not exist");
    file_.seekg(pageOffset(page_id));
    file_.read(reinterpret_cast<char*>(page.raw()), PAGE_SIZE);
    if (!file_) throw std::runtime_error("Read failed for page " +
                                          std::to_string(page_id));
}

uint32_t DiskManager::allocatePage() {
    uint32_t id = num_pages_;
    Page blank;
    blank.clear();
    writePage(id, blank);
    return id;
}

uint32_t DiskManager::pageCount() const { return num_pages_; }

void DiskManager::flush() { file_.flush(); }
