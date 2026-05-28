#include "Database.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <vector>
#include <atomic>

const std::string TEST_PATH = "/tmp/minidb_test";

struct DBTest : ::testing::Test {
    void SetUp() override {
        std::remove((TEST_PATH + ".db").c_str());
        std::remove((TEST_PATH + ".wal").c_str());
    }
    void TearDown() override {
        std::remove((TEST_PATH + ".db").c_str());
        std::remove((TEST_PATH + ".wal").c_str());
    }
};

TEST_F(DBTest, SetAndGet) {
    Database db(TEST_PATH);
    db.set("hello", "world");
    auto v = db.get("hello");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "world");
}

TEST_F(DBTest, GetMissing) {
    Database db(TEST_PATH);
    auto v = db.get("ghost");
    EXPECT_FALSE(v.has_value());
}

TEST_F(DBTest, Overwrite) {
    Database db(TEST_PATH);
    db.set("k", "v1");
    db.set("k", "v2");
    EXPECT_EQ(*db.get("k"), "v2");
}

TEST_F(DBTest, Delete) {
    Database db(TEST_PATH);
    db.set("bye", "value");
    EXPECT_TRUE(db.del("bye"));
    EXPECT_FALSE(db.get("bye").has_value());
}

TEST_F(DBTest, DeleteMissing) {
    Database db(TEST_PATH);
    EXPECT_FALSE(db.del("nobody"));
}

TEST_F(DBTest, MultipleKeys) {
    Database db(TEST_PATH);
    for (int i = 0; i < 100; i++)
        db.set("key" + std::to_string(i), "val" + std::to_string(i));
    for (int i = 0; i < 100; i++)
        EXPECT_EQ(*db.get("key" + std::to_string(i)), "val" + std::to_string(i));
}

TEST_F(DBTest, WALRecovery) {
    {
        Database db(TEST_PATH);
        db.set("persist", "yes");
        db.set("also",    "keep");
        // Destructor flushes
    }
    // Reopen — should recover from WAL / disk
    Database db2(TEST_PATH);
    auto v = db2.get("persist");
    EXPECT_TRUE(v.has_value());
}

TEST_F(DBTest, ConcurrentWrites) {
    Database db(TEST_PATH);
    std::vector<std::thread> threads;
    std::atomic<int> errors{0};

    for (int t = 0; t < 8; t++) {
        threads.emplace_back([&, t]() {
            try {
                for (int i = 0; i < 50; i++)
                    db.set("t" + std::to_string(t) + "k" + std::to_string(i),
                           "val");
            } catch (...) { errors++; }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_EQ(errors.load(), 0);
}

TEST_F(DBTest, StatsNotEmpty) {
    Database db(TEST_PATH);
    db.set("a", "b");
    EXPECT_FALSE(db.stats().empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
