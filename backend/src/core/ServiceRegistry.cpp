#include "core/ServiceRegistry.hpp"
#include "config/ConfigManager.hpp"
#include "services/OpenClawGateway.hpp"
#include "services/GPTSoVITSService.hpp"
#include "services/Live2DAnimationService.hpp"
#include "services/AvatarResponseService.hpp"
#include "services/AuthService.hpp"
#include "services/AuthServiceImpl.hpp"
#include "services/MessageServiceImpl.hpp"
#include "controllers/WebSocketController.hpp"
#include "utils/DatabaseUtil.hpp"
#include "utils/HashUtil.hpp"
#include "utils/JwtUtil.hpp"
#include "spdlog/spdlog.h"

#include <fstream>
#include <sstream>

extern std::shared_ptr<Yachiyo::Utils::DatabaseUtil> g_databaseUtil;

namespace yachiyo::core {

ServiceRegistry::ServiceRegistry(
    std::shared_ptr<config::ConfigManager> configMgr,
    std::shared_ptr<spdlog::logger> logger)
    : config_(std::move(configMgr)), log_(std::move(logger)) {
}

PipelineContext ServiceRegistry::buildPipeline() {
    PipelineContext ctx;

    initOpenClawGateway(ctx);
    initTTSService(ctx);
    initAnimationService(ctx);

    // AvatarService (精简版: 审查和翻译由 OpenClaw 统一处理)
    ctx.avatarService = std::make_shared<yachiyo::services::AvatarResponseService>(
        ctx.openClawGateway,
        ctx.ttsService,
        ctx.animationService
    );

    std::string avatarLang = config_->getString("avatar.language", "zh-CN");
    ctx.avatarService->initialize(avatarLang);
    log_->info("AvatarResponseService 初始化完成");

    ctx.wsController = std::make_shared<yachiyo::controllers::WebSocketController>(ctx.avatarService);
    ctx.wsController->initialize();
    log_->info("WebSocketController 初始化完成");

    log_->info("Avatar 响应管线构建完成 (审查+翻译由 OpenClaw 统一处理)");
    return ctx;
}

void ServiceRegistry::createSharedServices(PipelineContext& ctx) {
    ctx.dbUtil = std::make_shared<Yachiyo::Utils::DatabaseUtil>();

    // 连接数据库
    Yachiyo::Utils::DatabaseConfig dbCfg;
    dbCfg.host     = config_->getString("database.host", "localhost");
    dbCfg.port     = config_->getInt("database.port", 5432);
    dbCfg.database = config_->getString("database.name", "yachiyo");
    dbCfg.username = config_->getString("database.username", "postgres");
    dbCfg.password = config_->getString("database.password", "");
    if (!ctx.dbUtil->connect(dbCfg)) {
        log_->warn("DatabaseUtil 连接失败，认证功能可能不可用");
    }

    // 设置全局 DatabaseUtil 供 SystemLogger 使用
    ::g_databaseUtil = ctx.dbUtil;
    ctx.hashUtil = std::make_shared<Yachiyo::Utils::HashUtil>();

    std::string jwtSecret = config_->getString("jwt.secret", "");
    if (jwtSecret.empty()) {
        log_->warn("JWT secret 未配置！生产环境请务必设置");
        jwtSecret = "yachiyo-cpp-unsafe-default-change-me";
    }
    ctx.jwtUtil = std::make_shared<Yachiyo::Utils::JwtUtil>(
        jwtSecret,
        config_->getInt("jwt.expiration_hours", 24) * 3600
    );

    ctx.authService = std::make_shared<yachiyo::services::AuthServiceImpl>(
        ctx.dbUtil, ctx.jwtUtil, ctx.hashUtil
    );

    // MessageService 不再注入 DeepSeekModerationService
    ctx.messageService = std::make_shared<yachiyo::services::MessageServiceImpl>(
        ctx.dbUtil, ctx.authService, nullptr
    );
    log_->info("共享服务创建完成 (Auth + Message)");
}

void ServiceRegistry::initOpenClawGateway(PipelineContext& ctx) {
    ctx.openClawGateway = std::make_shared<yachiyo::services::OpenClawGateway>();

    std::string endpoint = config_->getString("openclaw.api_endpoint", "http://host.docker.internal:8100");
    std::string token = config_->getString("openclaw.auth_token", "");
    std::string model = config_->getString("openclaw.model", "deepseek/deepseek-v4-flash");
    int timeout = config_->getInt("openclaw.request_timeout", 30);

    if (!ctx.openClawGateway->initialize(endpoint, token, model, timeout)) {
        log_->warn("OpenClaw 网关初始化失败");
        return;
    }

    std::string promptPath = config_->getString(
        "openclaw.system_prompt_path", "/app/config/yachiyo_system_prompt.txt");
    std::ifstream promptFile(promptPath);
    if (promptFile.is_open()) {
        std::stringstream buffer;
        buffer << promptFile.rdbuf();
        std::string prompt = buffer.str();
        if (!prompt.empty()) {
            ctx.openClawGateway->setSystemPrompt(prompt);
            log_->info("System prompt 已加载 ({} chars)", prompt.size());
        }
    }

    log_->info("OpenClaw 网关初始化完成: endpoint={}, model={}", endpoint, model);
}

void ServiceRegistry::initTTSService(PipelineContext& ctx) {
    bool ttsEnabled = config_->getBool("gpt_sovits.enabled", true);
    if (!ttsEnabled) {
        log_->info("GPT-SoVITS TTS 已禁用 (gpt_sovits.enabled=false), 使用 mock 模式");
        ctx.ttsService = std::make_shared<yachiyo::services::GPTSoVITSService>();
        ctx.ttsService->initialize("", yachiyo::services::GPTSoVITSService::InferenceMode::CPU);
        return;
    }

    ctx.ttsService = std::make_shared<yachiyo::services::GPTSoVITSService>();

    std::string endpoint = config_->getString("gpt_sovits.api_endpoint", "http://localhost:5000");
    std::string modeStr = config_->getString("gpt_sovits.mode", "cpu");
    auto mode = (modeStr == "gpu")
        ? yachiyo::services::GPTSoVITSService::InferenceMode::GPU
        : yachiyo::services::GPTSoVITSService::InferenceMode::CPU;

    ctx.ttsService->initialize(endpoint, mode);
    log_->info("GPT-SoVITS TTS 初始化: endpoint={}, mode={}", endpoint, modeStr);
}

void ServiceRegistry::initAnimationService(PipelineContext& ctx) {
    ctx.animationService = std::make_shared<yachiyo::services::Live2DAnimationService>();
    ctx.animationService->initialize();
    log_->info("Live2D 动画服务初始化完成");
}

} // namespace yachiyo::core
