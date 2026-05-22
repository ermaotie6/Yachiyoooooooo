#include "Application.hpp"
#include "config/ConfigManager.hpp"
#include "utils/HttpServer.hpp"
#include "utils/LogUtils.hpp"
#include "controllers/BaseController.hpp"
#include "controllers/HealthController.hpp"
#include "controllers/AuthController.hpp"
#include "controllers/AIController.hpp"
#include "controllers/UserController.hpp"
#include "controllers/MessageController.hpp"
#include "services/DatabaseService.hpp"
#include "services/WebSocketService.hpp"
#include "services/AuthServiceImpl.hpp"
#include "services/MessageServiceImpl.hpp"
#include "core/ServiceRegistry.hpp"
#include "handlers/WebSocketMessageHandler.hpp"
#include "controllers/WebSocketController.hpp"
#include "utils/JwtUtil.hpp"
#include "utils/HashUtil.hpp"
#include "utils/DatabaseUtil.hpp"
#include "utils/RedisUtil.hpp"
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <fstream>
#include <sstream>

// 全局服务实例 (供所有 .cpp extern 引用)
std::shared_ptr<Yachiyo::Services::DatabaseService> g_databaseService = nullptr;
std::shared_ptr<Yachiyo::Services::WebSocketService> g_webSocketService = nullptr;

namespace yachiyo {

using utils::LogUtils;
namespace Services = Yachiyo::Services;
namespace Utils = Yachiyo::Utils;

// 全局应用程序实例
std::shared_ptr<Application> Application::instance = nullptr;
std::once_flag Application::initFlag;

Application::Application(PrivateTag)
    : configManager(nullptr),
      httpServer(nullptr),
      running(false),
      startTime(std::chrono::system_clock::now()) {
    logger = LogUtils::getLogger("Application");
}

Application::~Application() { stop(); }

std::shared_ptr<Application> Application::getInstance() {
    std::call_once(initFlag, []() {
        instance = std::make_shared<Application>(PrivateTag{});
    });
    return instance;
}

// ==================== 初始化 ====================

bool Application::initialize(int argc, char* argv[]) {
    try {
        logger->info("正在初始化 Yachiyo 应用程序...");

        parseArguments(argc, argv);

        configManager = config::ConfigManager::getInstance();
        configManager->initialize(arguments.configDir, arguments.environment);

        logger->info("配置环境: {}", configManager->getEnvironment());
        logger->info("配置路径: {}", configManager->getConfigPath());

        auto appConfig = configManager->loadConfig("config");

        LogUtils::initialize(
            configManager->getString("logging.level", "info"),
            configManager->getString("logging.file", "logs/yachiyo.log"));

        // 严格按依赖顺序初始化
        initializeHttpServer(appConfig);   // 1. HTTP 服务器
        initializeDatabase();              // 2. 数据库
        initializeAIServices();            // 3. AI 检测
        initializeServices();              // 4. WebSocket + Redis + Avatar 管线
        initializeControllers();           // 5. 路由注册（最后，依赖所有服务就绪）

        logger->info("应用程序初始化完成");
        return true;

    } catch (const std::exception& e) {
        logger->error("应用程序初始化失败: {}", e.what());
        return false;
    }
}

// ==================== 命令行 ====================

void Application::parseArguments(int argc, char* argv[]) {
    arguments.configDir = "config";
    arguments.environment = "dev";
    arguments.port = 8080;
    arguments.host = "0.0.0.0";
    arguments.workers = std::thread::hardware_concurrency();

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config-dir" && i + 1 < argc)      arguments.configDir = argv[++i];
        else if (arg == "--env" && i + 1 < argc)         arguments.environment = argv[++i];
        else if (arg == "--port" && i + 1 < argc)        arguments.port = std::stoi(argv[++i]);
        else if (arg == "--host" && i + 1 < argc)        arguments.host = argv[++i];
        else if (arg == "--workers" && i + 1 < argc)     arguments.workers = std::stoi(argv[++i]);
        else if (arg == "--help") { showHelp(); exit(0); }
    }
}

void Application::showHelp() {
    std::cout << "Yachiyo C++ 服务器使用说明:\n";
    std::cout << "  --config-dir <dir>    配置文件目录 (默认: config)\n";
    std::cout << "  --env <env>           运行环境 (默认: dev)\n";
    std::cout << "  --port <port>         监听端口 (默认: 8080)\n";
    std::cout << "  --host <host>         监听地址 (默认: 0.0.0.0)\n";
    std::cout << "  --workers <num>       工作线程数 (默认: CPU核心数)\n";
    std::cout << "  --help                显示此帮助信息\n";
}

// ==================== HTTP 服务器 ====================

void Application::initializeHttpServer(const json& config) {
    int port = configManager->getInt("server.port", arguments.port);
    std::string host = configManager->getString("server.host", arguments.host);
    int workers = configManager->getInt("server.workers", arguments.workers);

    logger->info("初始化 HTTP 服务器: {}:{} (workers: {})", host, port, workers);

    httpServer = std::make_shared<utils::CrowHttpServer>();
    httpServer->setHost(host);
    httpServer->setPort(port);
    httpServer->setWorkers(workers);

    if (configManager->getBool("server.ssl.enabled", false)) {
        httpServer->enableSSL(
            configManager->getString("server.ssl.cert", ""),
            configManager->getString("server.ssl.key", ""));
    }

    if (configManager->getBool("server.cors.enabled", true)) {
        httpServer->enableCORS(
            configManager->getString("server.cors.origin", "*"),
            configManager->getString("server.cors.methods", "GET,POST,PUT,DELETE,OPTIONS"),
            configManager->getString("server.cors.headers", "Content-Type,Authorization"),
            configManager->getBool("server.cors.credentials", true));
    }

    if (configManager->getBool("server.compression.enabled", true)) {
        httpServer->enableCompression(
            configManager->getInt("server.compression.level", 6));
    }

    if (configManager->getBool("server.rateLimit.enabled", true)) {
        httpServer->enableRateLimit(
            configManager->getInt("server.rateLimit.maxRequests", 100),
            configManager->getInt("server.rateLimit.windowSeconds", 60));
    }
}

// ==================== 数据库 ====================

void Application::initializeDatabase() {
    logger->info("正在初始化数据库连接...");

    std::string dbHost = configManager->getString("database.host", "localhost");
    int dbPort = configManager->getInt("database.port", 5432);
    std::string dbName = configManager->getString("database.name", "yachiyo");
    std::string dbUser = configManager->getString("database.username", "postgres");
    std::string dbPassword = configManager->getString("database.password", "");
    int dbPoolSize = configManager->getInt("database.poolSize", 20);

    try {
        std::string connStr = "host=" + dbHost + " port=" + std::to_string(dbPort)
            + " dbname=" + dbName + " user=" + dbUser;
        if (!dbPassword.empty()) connStr += " password=" + dbPassword;

        auto& dbService = Yachiyo::Services::DatabaseService::getInstance();
        if (!dbService.initialize(connStr, static_cast<size_t>(dbPoolSize))) {
            logger->error("数据库连接池初始化失败");
            return;
        }

        g_databaseService = std::shared_ptr<Yachiyo::Services::DatabaseService>(
            &dbService, [](Yachiyo::Services::DatabaseService*){});

        logger->info("数据库连接初始化完成: {}@{}:{}/{} (pool={})",
                    dbUser, dbHost, dbPort, dbName, dbPoolSize);
    } catch (const std::exception& e) {
        logger->error("数据库初始化异常: {}", e.what());
    }
}

// ==================== AI 服务检测 ====================

void Application::initializeAIServices() {
    logger->info("正在检测可用的 AI 提供商...");

    std::string provider = "none";
    std::string apiKey;

    apiKey = configManager->getString("ai.deepseek.api_key", "");
    if (!apiKey.empty()) {
        provider = "deepseek";
    } else {
        apiKey = configManager->getString("ai.openai.api_key", "");
        if (!apiKey.empty()) provider = "openai";
        else {
            apiKey = configManager->getString("ai.qianwen.api_key", "");
            if (!apiKey.empty()) provider = "qianwen";
            else {
                std::string ollama = configManager->getString("ai.ollama.base_url", "");
                if (!ollama.empty()) provider = "ollama";
            }
        }
    }

    if (provider == "none") {
        logger->warn("未配置任何 AI 服务，AI 聊天功能将不可用");
    }
    logger->info("AI 提供商检测完成: provider={}", provider);
}

// ==================== 服务初始化（重构后 — 使用 ServiceRegistry + WebSocketMessageHandler） ====================

void Application::initializeServices() {
    logger->info("正在初始化服务...");

    // === 1. WebSocket 服务 ===
    try {
        g_webSocketService = std::make_shared<Yachiyo::Services::WebSocketService>();
        int wsPort = configManager->getInt("websocket.port", 9001);
        std::string wsHost = configManager->getString("websocket.host", "0.0.0.0");

        wsThread_ = std::thread([wsHost, wsPort]() {
            try {
                g_webSocketService->start(wsHost, wsPort, "/");
            } catch (const std::exception& e) {
                auto log = LogUtils::getLogger("WebSocket");
                log->error("WebSocket 服务异常: {}", e.what());
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        logger->info("WebSocket 服务已启动: {}:{}", wsHost, wsPort);
    } catch (const std::exception& e) {
        logger->error("WebSocket 初始化失败: {}", e.what());
    }

    // === 2. Redis ===
    if (configManager->getBool("redis.enabled", false)) {
        try {
            Yachiyo::Utils::RedisConfig cfg;
            cfg.host = configManager->getString("redis.host", "localhost");
            cfg.port = configManager->getInt("redis.port", 6379);
            cfg.password = configManager->getString("redis.password", "");
            cfg.database = configManager->getInt("redis.database", 0);
            cfg.maxConnections = configManager->getInt("redis.pool_size", 10);
            cfg.timeoutSeconds = configManager->getInt("redis.timeout_seconds", 5);

            if (Yachiyo::Utils::RedisUtil::initializePool(cfg)) {
                logger->info("Redis 连接池初始化完成: {}:{}", cfg.host, cfg.port);
            } else {
                logger->warn("Redis 连接失败，将以无缓存模式运行");
            }
        } catch (const std::exception& e) {
            logger->warn("Redis 初始化异常: {}", e.what());
        }
    }

    // === 3. Avatar 管线 (通过 ServiceRegistry) ===
    try {
        core::ServiceRegistry registry(configManager, logger);

        // 创建共享服务 (Auth + Message + JWT)
        pipeline_ = std::make_shared<core::PipelineContext>();
        registry.createSharedServices(*pipeline_);

        // 构建 Avatar 管线 (OpenClaw 一站式处理: AI对话 + 审查 + 翻译 → TTS → 动画 → AvatarService → WSController)
        auto pipeCtx = registry.buildPipeline();
        pipeline_->openClawGateway   = std::move(pipeCtx.openClawGateway);
        pipeline_->ttsService        = std::move(pipeCtx.ttsService);
        pipeline_->animationService  = std::move(pipeCtx.animationService);
        pipeline_->avatarService     = std::move(pipeCtx.avatarService);
        pipeline_->wsController      = std::move(pipeCtx.wsController);

        logger->info("Avatar 管线构建完成");
    } catch (const std::exception& e) {
        logger->error("管线构建失败: {} (WebSocket 仍可接受连接)", e.what());
    }

    // === 4. WebSocket 消息处理器 ===
    if (g_webSocketService && pipeline_ && pipeline_->wsController) {
        wsHandler_ = std::make_shared<handlers::WebSocketMessageHandler>(
            g_webSocketService, pipeline_, logger);
        wsHandler_->registerCallbacks();
        logger->info("WebSocket 消息处理器已注册");
    } else {
        logger->warn("WebSocket 消息处理器未注册（服务未就绪）");
    }

    logger->info("服务初始化完成");
}

// ==================== 控制器注册 ====================

void Application::initializeControllers() {
    logger->info("正在注册路由控制器...");

    // 从 pipeline_ 获取共享服务引用
    auto& dbUtil  = pipeline_->dbUtil;
    auto& hashUtil = pipeline_->hashUtil;
    auto& jwtUtil  = pipeline_->jwtUtil;
    auto& authSvc  = pipeline_->authService;
    auto& msgSvc   = pipeline_->messageService;

    // 健康检查
    auto healthCtrl = std::make_shared<controllers::HealthController>();
    httpServer->registerController("/api/v1/health", healthCtrl);

    // JWT 密钥验证
    if (!jwtUtil) {
        std::string jwtSecret = configManager->getString("jwt.secret", "");
        if (jwtSecret.empty()) {
            logger->warn("⚠️ JWT secret 未配置！生产环境请务必设置");
        }
        jwtUtil = std::make_shared<Utils::JwtUtil>(
            jwtSecret.empty() ? "yachiyo-cpp-unsafe-default-change-me" : jwtSecret,
            configManager->getInt("jwt.expiration_hours", 24) * 3600);
    }

    // Auth
    if (!authSvc) {
        if (!dbUtil) dbUtil = std::make_shared<Utils::DatabaseUtil>();
        if (!hashUtil) hashUtil = std::make_shared<Utils::HashUtil>();
        authSvc = std::make_shared<services::AuthServiceImpl>(dbUtil, jwtUtil, hashUtil);
    }
    auto authCtrl = std::make_shared<controllers::AuthController>(authSvc, jwtUtil);
    httpServer->registerController("/api/v1/auth", authCtrl);

    // AI
    auto aiCtrl = std::make_shared<controllers::AIController>();
    httpServer->registerController("/api/v2/ai", aiCtrl);

    // Users
    auto userCtrl = std::make_shared<controllers::UserController>();
    httpServer->registerController("/api/v1/users", userCtrl);

    // Messages
    if (g_databaseService && g_webSocketService) {
        if (!msgSvc) {
            if (!dbUtil) dbUtil = std::make_shared<Utils::DatabaseUtil>();
            msgSvc = std::make_shared<services::MessageServiceImpl>(
                dbUtil, authSvc);
        }
        auto msgCtrl = std::make_shared<controllers::MessageController>(
            msgSvc, authSvc, g_databaseService, g_webSocketService, jwtUtil,
            Yachiyo::Utils::Logger::getLogger("MessageController"));
        httpServer->registerController("/api/v1/messages", msgCtrl);
        logger->info("消息控制器已注册");
    } else {
        logger->warn("数据库或 WebSocket 未就绪，跳过消息控制器注册");
    }

    logger->info("控制器注册完成");
}

// ==================== 启动 / 停止 ====================

bool Application::start() {
    if (running) { logger->warn("应用程序已在运行"); return false; }

    try {
        logger->info("正在启动 Yachiyo 应用程序...");
        if (!httpServer->start()) {
            logger->error("HTTP 服务器启动失败");
            return false;
        }

        running = true;
        registerSignalHandlers();

        logger->info("Yachiyo 启动成功，监听端口: {}", httpServer->getPort());
        logger->info("健康检查: http://{}:{}/api/v1/health",
                     httpServer->getHost(), httpServer->getPort());
        return true;
    } catch (const std::exception& e) {
        logger->error("启动失败: {}", e.what());
        return false;
    }
}

void Application::stop() {
    if (!running) return;

    logger->info("正在停止 Yachiyo 应用程序...");

    if (httpServer) httpServer->stop();

    if (wsThread_.joinable()) wsThread_.join();

    running = false;
    logger->info("Yachiyo 已停止");
}

void Application::wait() {
    if (httpServer && running) httpServer->wait();
}

void Application::registerSignalHandlers() {
    logger->info("信号处理器已在全局初始化阶段注册 (SIGINT, SIGTERM, SIGSEGV, SIGABRT)");
}

} // namespace yachiyo
