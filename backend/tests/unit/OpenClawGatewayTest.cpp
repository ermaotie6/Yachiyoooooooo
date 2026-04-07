#include <gtest/gtest.h>
#include "services/OpenClawGateway.hpp"

using yachiyo::services::OpenClawGateway;

class OpenClawGatewayTest : public ::testing::Test {
protected:
    void SetUp() override {
        gateway = std::make_unique<OpenClawGateway>();
    }
    
    void TearDown() override {
        if (gateway) gateway->shutdown();
    }
    
    std::unique_ptr<OpenClawGateway> gateway;
};

// ==================== 初始化测试 ====================

TEST_F(OpenClawGatewayTest, Initialize_Default) {
    // 桥接服务未运行时，仍然应该初始化成功（只是发出警告）
    bool result = gateway->initialize("http://localhost:8765", 30);
    EXPECT_TRUE(result);
}

TEST_F(OpenClawGatewayTest, Initialize_CustomEndpoint) {
    bool result = gateway->initialize("http://192.168.1.100:9999", 10);
    EXPECT_TRUE(result);
}

// ==================== 健康检查测试 ====================

TEST_F(OpenClawGatewayTest, IsHealthy_NoService) {
    gateway->initialize("http://localhost:99999", 2);
    // 桥接服务不可用时，健康检查应返回 false
    EXPECT_FALSE(gateway->isHealthy());
}

// ==================== 缓存测试 ====================

TEST_F(OpenClawGatewayTest, ClearCache_Empty) {
    gateway->initialize();
    gateway->clearCache();
    // 清空空缓存不应崩溃
}

TEST_F(OpenClawGatewayTest, GetCachedResponse_Miss) {
    gateway->initialize();
    auto cached = gateway->getCachedResponse("nonexistent_key");
    EXPECT_EQ(cached, nullptr);
}

// ==================== 情感提示测试 ====================

TEST_F(OpenClawGatewayTest, SetEmotionHints) {
    gateway->initialize();
    std::vector<std::string> emotions = {"happy", "excited", "calm"};
    gateway->setEmotionHints(emotions);
    // 验证不崩溃，情感提示将在下次 processMessage 中使用
}

TEST_F(OpenClawGatewayTest, SetEmotionHints_Empty) {
    gateway->initialize();
    gateway->setEmotionHints({});
    // 空列表不应崩溃
}

// ==================== 消息处理测试 (离线模式) ====================

TEST_F(OpenClawGatewayTest, ProcessMessage_BridgeUnavailable) {
    gateway->initialize("http://localhost:99999", 2);
    
    auto result = gateway->processMessage("user_1", "你好");
    
    // 桥接服务不可用时应返回错误
    EXPECT_FALSE(result.isSuccess());
    EXPECT_FALSE(result.getError().message.empty());
}

// ==================== 关闭测试 ====================

TEST_F(OpenClawGatewayTest, Shutdown) {
    gateway->initialize();
    gateway->shutdown();
    // 关闭后再次调用不应崩溃
    gateway->shutdown();
}

TEST_F(OpenClawGatewayTest, Shutdown_BeforeInitialize) {
    // 未初始化时关闭不应崩溃
    gateway->shutdown();
}
