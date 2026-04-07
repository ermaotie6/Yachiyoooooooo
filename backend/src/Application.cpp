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
#include "services/MessageService.hpp"
#include "services/AuthService.hpp"
#include "services/AuthServiceImpl.hpp"
#include "services/MessageServiceImpl.hpp"
#include "services/OpenClawGateway.hpp"
#include "services/TranslationService.hpp"
#include "services/GPTSoVITSService.hpp"
#include "services/Live2DAnimationService.hpp"
#include "services/DeepSeekModerationService.hpp"
#include "services/AvatarResponseService.hpp"
#include "controllers/WebSocketController.hpp"
#include "utils/JwtUtil.hpp"
#include "utils/HashUtil.hpp"
#include "utils/DatabaseUtil.hpp"
#include "utils/RedisUtil.hpp"
#include <memory>
#include <thread>
#include <csignal>

// 全局服务实例 (放在全局命名空间，供所有 .cpp extern 引用)
std::shared_ptr<Yachiyo::Services::DatabaseService> g_databaseService = nullptr;
std::shared_ptr<Yachiyo::Services::WebSocketService> g_webSocketService = nullptr;

namespace yachiyo {

using utils::LogUtils;
using Services = Yachiyo::Services;
using Utils = Yachiyo::Utils;

// 全局应用程序实例
std::shared_ptr<Application> Application::instance = nullptr;

Application::Application() 
    : configManager(nullptr), 
      httpServer(nullptr), 
      running(false),
      startTime(std::chrono::system_clock::now()) {
    logger = LogUtils::getLogger("Application");
}

Application::~Application() {
    stop();
}

std::shared_ptr<Application> Application::getInstance() {
    if (!instance) {
        instance = std::make_shared<Application>();
    }
    return instance;
}

bool Application::initialize(int argc, char* argv[]) {
    try {
        logger->info("正在初始化Yachiyo应用程序...");
        
        // 解析命令行参数
        parseArguments(argc, argv);
        
        // 初始化配置管理器
        configManager = config::ConfigManager::getInstance();
        configManager->initialize(arguments.configDir, arguments.environment);
        
        logger->info("配置环境: {}", configManager->getEnvironment());
        logger->info("配置路径: {}", configManager->getConfigPath());
        
        // 加载应用程序配置
        auto appConfig = configManager->loadConfig("application");
        
        // 初始化日志系统
        LogUtils::initialize(configManager->getString("logging.level", "info"),
                           configManager->getString("logging.file", "logs/yachiyo.log"));
        
        // 初始化HTTP服务器
        initializeHttpServer(appConfig);
        
        // 初始化数据库连接池 (必须在控制器之前，因为消息控制器依赖数据库服务)
        initializeDatabase();
        
        // 初始化AI服务
        initializeAIServices();
        
        // 初始化其他服务 (WebSocket等，必须在控制器之前)
        initializeServices();
        
        // 初始化控制器 (最后初始化，确保所有依赖的服务已就绪)
        initializeControllers();
        
        logger->info("应用程序初始化完成");
        return true;
        
    } catch (const std::exception& e) {
        logger->error("应用程序初始化失败: {}", e.what());
        return false;
    }
}

void Application::parseArguments(int argc, char* argv[]) {
    arguments.configDir = "config";
    arguments.environment = "dev";
    arguments.port = 8080;
    arguments.host = "0.0.0.0";
    arguments.workers = std::thread::hardware_concurrency();
    
    // 简单的命令行参数解析
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--config-dir" && i + 1 < argc) {
            arguments.configDir = argv[++i];
        } else if (arg == "--env" && i + 1 < argc) {
            arguments.environment = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            arguments.port = std::stoi(argv[++i]);
        } else if (arg == "--host" && i + 1 < argc) {
            arguments.host = argv[++i];
        } else if (arg == "--workers" && i + 1 < argc) {
            arguments.workers = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            showHelp();
            exit(0);
        }
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

void Application::initializeHttpServer(const json& config) {
    // 从配置获取服务器设置
    int port = configManager->getInt("server.port", arguments.port);
    std::string host = configManager->getString("server.host", arguments.host);
    int workers = configManager->getInt("server.workers", arguments.workers);
    
    logger->info("初始化HTTP服务器: {}:{} (workers: {})", host, port, workers);
    
    // 创建HTTP服务器实例
    httpServer = std::make_shared<utils::CrowHttpServer>();
    
    // 配置服务器
    httpServer->setHost(host);
    httpServer->setPort(port);
    httpServer->setWorkers(workers);
    
    // 配置SSL（如果启用）
    if (configManager->getBool("server.ssl.enabled", false)) {
        httpServer->enableSSL(
            configManager->getString("server.ssl.cert", ""),
            configManager->getString("server.ssl.key", "")
        );
    }
    
    // 配置CORS
    if (configManager->getBool("server.cors.enabled", true)) {
        httpServer->enableCORS(
            configManager->getString("server.cors.origin", "*"),
            configManager->getString("server.cors.methods", "GET,POST,PUT,DELETE,OPTIONS"),
            configManager->getString("server.cors.headers", "Content-Type,Authorization"),
            configManager->getBool("server.cors.credentials", true)
        );
    }
    
    // 配置压缩
    if (configManager->getBool("server.compression.enabled", true)) {
        httpServer->enableCompression(
            configManager->getInt("server.compression.level", 6)
        );
    }
    
    // 配置请求限制
    if (configManager->getBool("server.rateLimit.enabled", true)) {
        httpServer->enableRateLimit(
            configManager->getInt("server.rateLimit.maxRequests", 100),
            configManager->getInt("server.rateLimit.windowSeconds", 60)
        );
    }
}

void Application::initializeControllers() {
    logger->info("正在初始化控制器...");
    
    // 注册健康检查控制器
    auto healthController = std::make_shared<controllers::HealthController>();
    httpServer->registerController("/api/v1/health", healthController);
    
    // 注册认证控制器
    auto dbUtil = std::make_shared<Utils::DatabaseUtil>();
    auto hashUtil = std::make_shared<Utils::HashUtil>();
    auto jwtUtil = std::make_shared<Utils::JwtUtil>(
        configManager->getString("jwt.secret", "yachiyo-default-secret-change-in-production"),
        configManager->getInt("jwt.expiresIn", 86400)
    );
    auto authService = std::make_shared<yachiyo::services::AuthServiceImpl>(dbUtil, jwtUtil, hashUtil);
    auto authController = std::make_shared<controllers::AuthController>(authService, jwtUtil);
    httpServer->registerController("/api/v1/auth", authController);
    
    // 注册AI控制器
    auto aiController = std::make_shared<controllers::AIController>();
    httpServer->registerController("/api/v2/ai", aiController);
    
    // 注册用户控制器
    auto userController = std::make_shared<controllers::UserController>();
    httpServer->registerController("/api/v1/users", userController);
    
    // 注册消息控制器（需要数据库和WebSocket服务）
    if (g_databaseService && g_webSocketService) {
        // 创建 MessageServiceImpl
        auto messageService = std::make_shared<yachiyo::services::MessageServiceImpl>(
            dbUtil,
            std::static_pointer_cast<yachiyo::services::IAuthService>(authService)
        );
        
        auto messageController = std::make_shared<controllers::MessageController>(
            messageService,
            std::static_pointer_cast<yachiyo::services::IAuthService>(authService),
            g_databaseService, g_webSocketService, jwtUtil,
            Yachiyo::Utils::Logger::getLogger("MessageController")
        );
        httpServer->registerController("/api/v1/messages", messageController);
        
        logger->info("消息控制器已注册");
    } else {
        logger->warn("数据库或WebSocket服务未初始化，跳过消息控制器注册");
    }
    
    logger->info("控制器初始化完成");
}

void Application::initializeDatabase() {
    logger->info("正在初始化数据库连接...");
    
    // 获取数据库配置
    std::string dbHost = configManager->getString("database.host", "localhost");
    int dbPort = configManager->getInt("database.port", 5432);  // PostgreSQL默认端口
    std::string dbName = configManager->getString("database.name", "yachiyo");
    std::string dbUser = configManager->getString("database.user", "postgres");
    std::string dbPassword = configManager->getString("database.password", "");
    int dbPoolSize = configManager->getInt("database.poolSize", 10);
    
    try {
        // 构建连接字符串
        std::string connectionString = "host=" + dbHost + 
                                      " port=" + std::to_string(dbPort) +
                                      " dbname=" + dbName +
                                      " user=" + dbUser;
        
        if (!dbPassword.empty()) {
            connectionString += " password=" + dbPassword;
        }
        
        // 获取数据库服务单例
        auto& dbService = Yachiyo::Services::DatabaseService::getInstance();
        
        // 初始化连接
        if (!dbService.initialize(connectionString)) {
            logger->error("数据库连接池初始化失败");
            return;
        }
        
        // 将单例地址包装为 shared_ptr（不释放，因为是 static 对象）
        g_databaseService = std::shared_ptr<Yachiyo::Services::DatabaseService>(
            &dbService, [](Yachiyo::Services::DatabaseService*){} // no-op deleter
        );
        
        logger->info("数据库连接初始化完成: {}@{}:{}/{} (pool size: {})", 
                    dbUser, dbHost, dbPort, dbName, dbPoolSize);
        
    } catch (const std::exception& e) {
        logger->error("数据库初始化异常: {}", e.what());
    }
}

void Application::initializeAIServices() {
    logger->info("正在初始化AI服务...");
    
    // 获取AI配置
    std::string aiProvider = configManager->getString("ai.provider", "openai");
    std::string apiKey = configManager->getString("ai.apiKey", "");
    std::string baseUrl = configManager->getString("ai.baseUrl", "https://api.openai.com/v1");
    std::string model = configManager->getString("ai.model", "gpt-3.5-turbo");
    
    // 初始化AI服务
    // aiService = std::make_shared<AIService>(...);
    
    logger->info("AI服务初始化完成: provider={}, model={}", aiProvider, model);
}

void Application::initializeServices() {
    logger->info("正在初始化其他服务...");
    
    // 初始化WebSocket服务
    try {
        g_webSocketService = std::make_shared<Yachiyo::Services::WebSocketService>();
        
        int wsPort = configManager->getInt("websocket.port", 9001);
        std::string wsHost = configManager->getString("websocket.host", "0.0.0.0");
        
        // 启动WebSocket服务（在后台线程中，start() 是阻塞调用）
        std::thread wsThread([this, wsHost, wsPort]() {
            try {
                logger->info("WebSocket服务启动: {}:{}", wsHost, wsPort);
                g_webSocketService->start(wsHost, wsPort, "/");
            } catch (const std::exception& e) {
                logger->error("WebSocket服务异常: {}", e.what());
            }
        });
        wsThread.detach();  // 在后台运行
        
        // 等待 Crow WebSocket 服务启动
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        logger->info("WebSocket服务初始化完成: {}:{}", wsHost, wsPort);
        
    } catch (const std::exception& e) {
        logger->error("WebSocket服务初始化失败: {}", e.what());
    }
    
    // 初始化Redis连接池
    if (configManager->getBool("redis.enabled", false)) {
        std::string redisHost = configManager->getString("redis.host", "localhost");
        int redisPort = configManager->getInt("redis.port", 6379);
        std::string redisPassword = configManager->getString("redis.password", "");
        int redisPoolSize = configManager->getInt("redis.poolSize", 5);
        
        // redisPool = std::make_shared<RedisPool>(...);
        logger->info("Redis连接初始化完成: {}:{}", redisHost, redisPort);
    }
    
    // ==================================================================
    // 初始化 Avatar 响应管线 (OpenClaw → 翻译 → TTS → Live2D 动画)
    // ==================================================================
    try {
        logger->info("正在初始化 Avatar 响应管线...");

        // 1. OpenClaw 网关 — 通过 Node.js 桥接服务与 OpenClaw 通信
        auto openClawGateway = std::make_shared<yachiyo::services::OpenClawGateway>();
        std::string bridgeEndpoint = configManager->getString(
            "openclaw.bridge_endpoint", "http://localhost:8765");
        int bridgeTimeout = configManager->getInt("openclaw.timeout", 30);
        if (openClawGateway->initialize(bridgeEndpoint, bridgeTimeout)) {
            logger->info("OpenClaw 网关初始化完成: {}", bridgeEndpoint);
        } else {
            logger->warn("OpenClaw 网关初始化失败 (桥接服务可能未启动), Avatar 管线将降级");
        }

        // 2. 翻译服务
        auto translationService = std::make_shared<yachiyo::services::TranslationService>();
        translationService->initialize();
        logger->info("翻译服务初始化完成");

        // 3. GPT-SoVITS TTS 服务
        auto ttsService = std::make_shared<yachiyo::services::GPTSoVITSService>();
        std::string ttsEndpoint = configManager->getString("tts.endpoint", "");
        ttsService->initialize(ttsEndpoint);
        logger->info("GPT-SoVITS TTS 服务初始化完成");

        // 4. Live2D 动画服务
        auto animationService = std::make_shared<yachiyo::services::Live2DAnimationService>();
        animationService->initialize();
        logger->info("Live2D 动画服务初始化完成");

        // 5. 内容审查服务 (可选)
        std::shared_ptr<yachiyo::services::DeepSeekModerationService> moderationService = nullptr;
        if (configManager->getBool("moderation.enabled", false)) {
            moderationService = std::make_shared<yachiyo::services::DeepSeekModerationService>();
            std::string moderationApiKey = configManager->getString("moderation.apiKey", "");
            std::string moderationEndpoint = configManager->getString("moderation.endpoint", "");
            moderationService->initialize(moderationApiKey, moderationEndpoint);
            logger->info("DeepSeek 内容审查服务初始化完成");
        }

        // 6. 组装 AvatarResponseService
        auto avatarService = std::make_shared<yachiyo::services::AvatarResponseService>(
            openClawGateway, translationService, ttsService, animationService, moderationService);
        std::string avatarLanguage = configManager->getString("avatar.language", "zh-CN");
        avatarService->initialize(avatarLanguage);
        logger->info("AvatarResponseService 初始化完成");

        // 7. 创建 WebSocketController 并注册回调
        auto wsController = std::make_shared<yachiyo::controllers::WebSocketController>(avatarService);
        wsController->initialize();

        // 8. 将 WebSocket 消息回调连接到 WebSocketController
        //    当 WebSocketService 收到 user_message 时，转发给 WebSocketController 处理，
        //    WebSocketController 调用 AvatarResponseService，然后通过 WebSocketService 推送响应。
        if (g_webSocketService) {
            g_webSocketService->onMessageReceived(
                [wsController, this](int64_t client_id, const json& message) {
                    try {
                        std::string clientIdStr = std::to_string(client_id);
                        std::string userId;

                        // 从消息中提取 user_id
                        if (message.contains("data") && message["data"].contains("user_id")) {
                            userId = message["data"]["user_id"].get<std::string>();
                        }
                        if (message.contains("data") && message["data"].contains("content")) {
                            std::string text = message["data"]["content"].get<std::string>();
                            std::string language = "zh-CN";
                            if (message["data"].contains("language")) {
                                language = message["data"]["language"].get<std::string>();
                            }

                            // 调用 WebSocketController 处理用户消息
                            auto result = wsController->processUserMessage(
                                clientIdStr, userId, text, language);

                            if (!result.isSuccess()) {
                                // 推送错误到客户端
                                json errorMsg = {
                                    {"type", "error"},
                                    {"data", {
                                        {"message", "处理消息失败"},
                                        {"code", result.getError().code}
                                    }}
                                };
                                g_webSocketService->sendToClient(client_id, errorMsg);
                            }
                            // 注：成功时 WebSocketController::processUserMessage 内部已通过
                            //     broadcastResponse 构建响应 JSON，但 broadcastResponse 使用的是
                            //     控制器内部的 session 路由，实际推送仍需通过 WebSocketService。
                            //     这里额外通过 WebSocketService 推送 avatar_response。
                            else {
                                // result 已包含 avatar 响应 JSON
                                g_webSocketService->sendToClient(client_id, result.getValue());
                            }
                        }
                    } catch (const std::exception& e) {
                        logger->error("WebSocket 消息处理异常: {}", e.what());
                        json errorMsg = {
                            {"type", "error"},
                            {"data", {{"message", std::string("内部错误: ") + e.what()}}}
                        };
                        g_webSocketService->sendToClient(client_id, errorMsg);
                    }
                });
            logger->info("WebSocket 消息回调已连接到 Avatar 管线");
        }

        logger->info("Avatar 响应管线初始化完成");
    } catch (const std::exception& e) {
        logger->error("Avatar 响应管线初始化失败: {} (WebSocket 仍可接受连接，但无法处理 AI 消息)", e.what());
    }
    
    logger->info("其他服务初始化完成");
}

bool Application::start() {
    if (running) {
        logger->warn("应用程序已经在运行");
        return false;
    }
    
    try {
        logger->info("正在启动Yachiyo应用程序...");
        
        // 启动HTTP服务器
        if (!httpServer->start()) {
            logger->error("HTTP服务器启动失败");
            return false;
        }
        
        running = true;
        
        // 注册信号处理器
        registerSignalHandlers();
        
        logger->info("Yachiyo应用程序启动成功，监听端口: {}", httpServer->getPort());
        logger->info("API文档: http://{}:{}/docs", httpServer->getHost(), httpServer->getPort());
        logger->info("健康检查: http://{}:{}/api/v1/health", httpServer->getHost(), httpServer->getPort());
        
        return true;
        
    } catch (const std::exception& e) {
        logger->error("应用程序启动失败: {}", e.what());
        return false;
    }
}

void Application::stop() {
    if (!running) {
        return;
    }
    
    logger->info("正在停止Yachiyo应用程序...");
    
    // 停止HTTP服务器
    if (httpServer) {
        httpServer->stop();
    }
    
    // 关闭数据库连接池
    // if (databasePool) {
    //     databasePool->close();
    // }
    
    // 关闭Redis连接池
    // if (redisPool) {
    //     redisPool->close();
    // }
    
    running = false;
    logger->info("Yachiyo应用程序已停止");
}

void Application::wait() {
    if (httpServer && running) {
        httpServer->wait();
    }
}

void Application::registerSignalHandlers() {
    // 信号处理器已在 main.cpp globalInit() 中通过 setupSignalHandlers() 注册。
    // 这里不再重复注册，避免覆盖全局设置。
    // 注意：C++ 标准不允许 lambda 或有状态函数作为 std::signal 的处理器，
    //       只允许 extern "C" 函数或无捕获的函数指针。
    logger->info("信号处理器已在全局初始化阶段注册 (SIGINT, SIGTERM, SIGSEGV, SIGABRT)");
}

} // namespace yachiyo