#pragma once
#include "DiskManager.h"
#include <string>
#include <optional>
#include <vector>

// Layout inside a BTree page (all offsets in bytes):
//  [0]      uint8_t   is_leaf   (1 = leaf, 0 = internal)
//  [1..4]   uint32_t  num_keys
//  [5..8]   uint32_t  parent_page_id
//  Keys/values start at offset 9
//  Each slot: [key_len:2][key:key_len][val_len:2][val:val_len]  (leaf)
//             [key_len:2][key:key_len][child_page:4]            (internal)

static constexpr uint32_t BTREE_ORDER = 10;  // max keys per node

struct BTreeNode {
    bool                      is_leaf;
    uint32_t                  page_id;
    uint32_t                  parent_id;
    std::vector<std::string>  keys;
    std::vector<std::string>  values;       // leaf only
    std::vector<uint32_t>     children;     // internal only
};

class BTree {
public:
    explicit BTree(DiskManager& disk);

    void        insert(const std::string& key, const std::string& value);
    std::optional<std::string> search(const std::string& key);
    bool        remove(const std::string& key);
    std::vector<std::pair<std::string,std::string>>
                scan(const std::string& from, const std::string& to);

    uint32_t    rootPageId() const { return root_page_id_; }

private:
    DiskManager& disk_;
    uint32_t     root_page_id_;

    BTreeNode readNode(uint32_t page_id);
    void      writeNode(const BTreeNode& node);
    uint32_t  allocateNode(bool is_leaf);

    void      insertIntoLeaf(BTreeNode& leaf,
                              const std::string& key,
                              const std::string& value);
    void      splitLeaf(BTreeNode& leaf);
    void      splitInternal(BTreeNode& node);
    void      insertIntoParent(BTreeNode& left, BTreeNode& right,
                                const std::string& push_up_key);
    uint32_t  findLeaf(const std::string& key);

    // Page serialization
    void      serializeNode(const BTreeNode& node, Page& page);
    BTreeNode deserializeNode(const Page& page, uint32_t page_id);
};
