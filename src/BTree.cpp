#include "BTree.h"
#include <algorithm>
#include <stdexcept>

BTree::BTree(DiskManager& disk) : disk_(disk) {
    if (disk_.pageCount() == 0) {
        root_page_id_ = allocateNode(true);
    } else {
        root_page_id_ = 0;
    }
}

uint32_t BTree::allocateNode(bool is_leaf) {
    uint32_t pid = disk_.allocatePage();
    BTreeNode node;
    node.is_leaf  = is_leaf;
    node.page_id  = pid;
    node.parent_id = INVALID_PAGE;
    writeNode(node);
    return pid;
}

void BTree::serializeNode(const BTreeNode& node, Page& page) {
    page.clear();
    uint32_t off = 0;
    page.write<uint8_t>(off,  node.is_leaf ? 1 : 0); off += 1;
    page.write<uint32_t>(off, static_cast<uint32_t>(node.keys.size())); off += 4;
    page.write<uint32_t>(off, node.parent_id); off += 4;

    for (size_t i = 0; i < node.keys.size(); i++) {
        uint16_t klen = static_cast<uint16_t>(node.keys[i].size());
        page.write<uint16_t>(off, klen); off += 2;
        std::memcpy(page.raw() + off, node.keys[i].data(), klen);
        off += klen;

        if (node.is_leaf) {
            uint16_t vlen = static_cast<uint16_t>(node.values[i].size());
            page.write<uint16_t>(off, vlen); off += 2;
            std::memcpy(page.raw() + off, node.values[i].data(), vlen);
            off += vlen;
        } else {
            page.write<uint32_t>(off, node.children[i]); off += 4;
        }
    }
    if (!node.is_leaf && !node.children.empty()) {
        page.write<uint32_t>(off, node.children.back()); off += 4;
    }
}

BTreeNode BTree::deserializeNode(const Page& page, uint32_t page_id) {
    BTreeNode node;
    node.page_id = page_id;
    uint32_t off = 0;

    node.is_leaf   = page.read<uint8_t>(off) == 1; off += 1;
    uint32_t nkeys = page.read<uint32_t>(off);     off += 4;
    node.parent_id = page.read<uint32_t>(off);     off += 4;

    for (uint32_t i = 0; i < nkeys; i++) {
        uint16_t klen = page.read<uint16_t>(off); off += 2;
        std::string key(reinterpret_cast<const char*>(page.raw() + off), klen);
        off += klen;
        node.keys.push_back(key);

        if (node.is_leaf) {
            uint16_t vlen = page.read<uint16_t>(off); off += 2;
            std::string val(reinterpret_cast<const char*>(page.raw() + off), vlen);
            off += vlen;
            node.values.push_back(val);
        } else {
            uint32_t child = page.read<uint32_t>(off); off += 4;
            node.children.push_back(child);
        }
    }
    if (!node.is_leaf && nkeys > 0) {
        uint32_t last_child = page.read<uint32_t>(off);
        node.children.push_back(last_child);
    }
    return node;
}

void BTree::writeNode(const BTreeNode& node) {
    Page page;
    serializeNode(node, page);
    disk_.writePage(node.page_id, page);
}

BTreeNode BTree::readNode(uint32_t page_id) {
    Page page;
    disk_.readPage(page_id, page);
    return deserializeNode(page, page_id);
}

uint32_t BTree::findLeaf(const std::string& key) {
    uint32_t cur = root_page_id_;
    while (true) {
        BTreeNode node = readNode(cur);
        if (node.is_leaf) return cur;
        // Find which child to descend into
        size_t i = 0;
        while (i < node.keys.size() && key >= node.keys[i]) i++;
        cur = node.children[i];
    }
}

std::optional<std::string> BTree::search(const std::string& key) {
    uint32_t leaf_id = findLeaf(key);
    BTreeNode leaf   = readNode(leaf_id);
    for (size_t i = 0; i < leaf.keys.size(); i++) {
        if (leaf.keys[i] == key) return leaf.values[i];
    }
    return std::nullopt;
}

void BTree::insertIntoLeaf(BTreeNode& leaf,
                            const std::string& key,
                            const std::string& value) {
    auto it = std::lower_bound(leaf.keys.begin(), leaf.keys.end(), key);
    size_t idx = it - leaf.keys.begin();

    // Update existing key
    if (it != leaf.keys.end() && *it == key) {
        leaf.values[idx] = value;
        return;
    }
    leaf.keys.insert(it, key);
    leaf.values.insert(leaf.values.begin() + idx, value);
}

void BTree::insertIntoParent(BTreeNode& left, BTreeNode& right,
                              const std::string& push_up_key) {
    // If left was root, create new root
    if (left.parent_id == INVALID_PAGE) {
        uint32_t new_root_id = allocateNode(false);
        BTreeNode new_root   = readNode(new_root_id);
        new_root.keys.push_back(push_up_key);
        new_root.children.push_back(left.page_id);
        new_root.children.push_back(right.page_id);
        left.parent_id  = new_root_id;
        right.parent_id = new_root_id;
        writeNode(left);
        writeNode(right);
        writeNode(new_root);
        root_page_id_ = new_root_id;
        return;
    }

    BTreeNode parent = readNode(left.parent_id);
    auto it  = std::lower_bound(parent.keys.begin(), parent.keys.end(), push_up_key);
    size_t idx = it - parent.keys.begin();
    parent.keys.insert(it, push_up_key);
    parent.children.insert(parent.children.begin() + idx + 1, right.page_id);
    right.parent_id = parent.page_id;
    writeNode(right);

    if (parent.keys.size() >= BTREE_ORDER) {
        splitInternal(parent);
    } else {
        writeNode(parent);
    }
}

void BTree::splitLeaf(BTreeNode& leaf) {
    size_t mid = leaf.keys.size() / 2;

    uint32_t new_id  = allocateNode(true);
    BTreeNode sibling = readNode(new_id);
    sibling.parent_id = leaf.parent_id;

    sibling.keys.assign(leaf.keys.begin() + mid,   leaf.keys.end());
    sibling.values.assign(leaf.values.begin() + mid, leaf.values.end());
    leaf.keys.resize(mid);
    leaf.values.resize(mid);

    std::string push_up = sibling.keys[0];
    writeNode(leaf);
    writeNode(sibling);
    insertIntoParent(leaf, sibling, push_up);
}

void BTree::splitInternal(BTreeNode& node) {
    size_t mid = node.keys.size() / 2;
    std::string push_up = node.keys[mid];

    uint32_t new_id  = allocateNode(false);
    BTreeNode sibling = readNode(new_id);
    sibling.parent_id = node.parent_id;

    sibling.keys.assign(node.keys.begin() + mid + 1, node.keys.end());
    sibling.children.assign(node.children.begin() + mid + 1, node.children.end());
    node.keys.resize(mid);
    node.children.resize(mid + 1);

    // Update parent pointers of moved children
    for (uint32_t child_id : sibling.children) {
        BTreeNode child = readNode(child_id);
        child.parent_id = sibling.page_id;
        writeNode(child);
    }

    writeNode(node);
    writeNode(sibling);
    insertIntoParent(node, sibling, push_up);
}

void BTree::insert(const std::string& key, const std::string& value) {
    uint32_t leaf_id = findLeaf(key);
    BTreeNode leaf   = readNode(leaf_id);
    insertIntoLeaf(leaf, key, value);

    if (leaf.keys.size() >= BTREE_ORDER) {
        splitLeaf(leaf);
    } else {
        writeNode(leaf);
    }
}

bool BTree::remove(const std::string& key) {
    uint32_t leaf_id = findLeaf(key);
    BTreeNode leaf   = readNode(leaf_id);

    auto it = std::find(leaf.keys.begin(), leaf.keys.end(), key);
    if (it == leaf.keys.end()) return false;

    size_t idx = it - leaf.keys.begin();
    leaf.keys.erase(leaf.keys.begin() + idx);
    leaf.values.erase(leaf.values.begin() + idx);
    writeNode(leaf);
    return true;
}

std::vector<std::pair<std::string,std::string>>
BTree::scan(const std::string& from, const std::string& to) {
    std::vector<std::pair<std::string,std::string>> results;
    uint32_t leaf_id = findLeaf(from);

    while (leaf_id != INVALID_PAGE) {
        BTreeNode leaf = readNode(leaf_id);
        for (size_t i = 0; i < leaf.keys.size(); i++) {
            if (leaf.keys[i] > to) return results;
            if (leaf.keys[i] >= from)
                results.push_back({leaf.keys[i], leaf.values[i]});
        }
        // Move to sibling — stored as first child slot of parent (simplified)
        break;  // single-leaf scan for now; extend with sibling pointers later
    }
    return results;
}
