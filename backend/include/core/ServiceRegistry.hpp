#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace yachiyo::config { class ConfigManager; }
namespace yachiyo::services {
    class OpenClawGateway;
    class GPTSoVITSService;
    class Live2DAnimationService;
    class AvatarResponseService;
    class IAuthService;
    class IMessageService;
}
namespace yachiyo::controllers { class WebSocketController; }
namespace Yachiyo::Utils { class DatabaseUtil; class HashUtil; class JwtUtil; }
namespace spdlog { class logger; }

namespace yachiyo::core {

/**
 * @brief 管线上下文 — 持有 Avatar 管线所需的所有服务实例
 *
 * 架构 (精简版):
 *   审查 → OpenClaw (内置)  翻译 → OpenClaw (内置)
 *   不再依赖独立的 DeepSeekModerationService 和 TranslationService
 */
struct PipelineContext {
    std::shared_ptr<yachiyo::services::OpenClawGateway> openClawGateway;
    std::shared_ptr<yachiyo::services::GPTSoVITSService> ttsService;
    std::shared_ptr<yachiyo::services::Live2DAnimationService> animationService;
    std::shared_ptr<yachiyo::services::AvatarResponseService> avatarService;
    std::shared_ptr<yachiyo::controllers::WebSocketController> wsController;

    std::shared_ptr<Yachiyo::Utils::DatabaseUtil> dbUtil;
    std::shared_ptr<Yachiyo::Utils::HashUtil> hashUtil;
    std::shared_ptr<Yachiyo::Utils::JwtUtil> jwtUtil;
    std::shared_ptr<yachiyo::services::IAuthService> authService;
    std::shared_ptr<yachiyo::services::IMessageService> messageService;
};

/**
 * @brief 服务注册器 — 创建和组装管线服务
 *
 * 管线: OpenClaw → TTS → 动画 → AvatarService → WSController
 */
class ServiceRegistry {
public:
    explicit ServiceRegistry(std::shared_ptr<config::ConfigManager> configMgr,
                             std::shared_ptr<spdlog::logger> logger);

    PipelineContext buildPipeline();
    void createSharedServices(PipelineContext& ctx);

private:
    std::shared_ptr<config::ConfigManager> config_;
    std::shared_ptr<spdlog::logger> log_;

    void initOpenClawGateway(PipelineContext& ctx);
    void initTTSService(PipelineContext& ctx);
    void initAnimationService(PipelineContext& ctx);
};

} // namespace yachiyo::core
