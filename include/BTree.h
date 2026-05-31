#pragma once
#include "DiskManager.h"
#include <string>
#include <optional>
#include <vector>

// Page layout (offsets in bytes):
//  [0]     uint8_t   is_leaf
//  [1..4]  uint32_t  num_keys
//  [5..8]  uint32_t  parent_page_id
//  [9..12] uint32_t  right_sibling_id  (leaf only; INVALID_PAGE for internal)
//  Keys/values start at offset 13
//  Leaf slot:     [key_len:2][key:N][val_len:2][val:M]
//  Internal slot: [key_len:2][key:N][child_page:4]

// Why BTREE_ORDER=8: each leaf holds up to 7 key-value pairs before split.
// With 4KB pages and variable-length keys, 8 gives headroom without
// wasting pages on near-empty nodes during initial load.
static constexpr uint32_t BTREE_ORDER = 8;

struct BTreeNode {
    bool                      is_leaf;
    uint32_t                  page_id;
    uint32_t                  parent_id;
    uint32_t                  right_sibling;  // INVALID_PAGE if none
    std::vector<std::string>  keys;
    std::vector<std::string>  values;         // leaf only
    std::vector<uint32_t>     children;       // internal only
};

class BTree {
public:
    explicit BTree(DiskManager& disk);

    void        insert(const std::string& key, const std::string& value);
    std::optional<std::string> search(const std::string& key);
    bool        remove(const std::string& key);

    // Returns all key-value pairs where from <= key <= to,
    // walking the leaf chain via right_sibling pointers.
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

    // Rebalance after deletion: borrow from sibling or merge
    void      fixUnderflow(BTreeNode& node);

    void      serializeNode(const BTreeNode& node, Page& page);
    BTreeNode deserializeNode(const Page& page, uint32_t page_id);
};
