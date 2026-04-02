#include "../include/Application.hpp"
#include "../include/config/ConfigManager.hpp"
#include "../include/utils/HttpServer.hpp"
#include "../include/utils/LogUtils.hpp"
#include "../include/controllers/BaseController.hpp"
#include <memory>
#include <thread>
#include <csignal>

namespace yachiyo {

// 全局应用程序实例
std::shared_ptr<Application> Application::instance = nullptr;

Application::Application() 
    : configManager(nullptr), 
      httpServer(nullptr), 
      running(false) {
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
                           configManager->getString("logging.file", "logs/yachiyo.log"),
                           configManager->getBool("logging.console", true));
        
        // 初始化HTTP服务器
        initializeHttpServer(appConfig);
        
        // 初始化控制器
        initializeControllers();
        
        // 初始化数据库连接池
        initializeDatabase();
        
        // 初始化AI服务
        initializeAIServices();
        
        // 初始化其他服务
        initializeServices();
        
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
    auto authController = std::make_shared<controllers::AuthController>();
    httpServer->registerController("/api/v1/auth", authController);
    
    // 注册AI控制器
    auto aiController = std::make_shared<controllers::AIController>();
    httpServer->registerController("/api/v2/ai", aiController);
    
    // 注册用户控制器
    auto userController = std::make_shared<controllers::UserController>();
    httpServer->registerController("/api/v1/users", userController);
    
    // 注册帖子控制器
    auto postController = std::make_shared<controllers::PostController>();
    httpServer->registerController("/api/v1/posts", postController);
    
    logger->info("控制器初始化完成");
}

void Application::initializeDatabase() {
    logger->info("正在初始化数据库连接...");
    
    // 获取数据库配置
    std::string dbHost = configManager->getString("database.host", "localhost");
    int dbPort = configManager->getInt("database.port", 3306);
    std::string dbName = configManager->getString("database.name", "yachiyo");
    std::string dbUser = configManager->getString("database.user", "root");
    std::string dbPassword = configManager->getString("database.password", "");
    int dbPoolSize = configManager->getInt("database.poolSize", 10);
    
    // 初始化数据库连接池
    // 这里需要实现具体的数据库连接池初始化
    // databasePool = std::make_shared<DatabasePool>(...);
    
    logger->info("数据库连接初始化完成: {}@{}:{}/{}", dbUser, dbHost, dbPort, dbName);
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
    
    // 初始化Redis连接池
    if (configManager->getBool("redis.enabled", false)) {
        std::string redisHost = configManager->getString("redis.host", "localhost");
        int redisPort = configManager->getInt("redis.port", 6379);
        std::string redisPassword = configManager->getString("redis.password", "");
        int redisPoolSize = configManager->getInt("redis.poolSize", 5);
        
        // redisPool = std::make_shared<RedisPool>(...);
        logger->info("Redis连接初始化完成: {}:{}", redisHost, redisPort);
    }
    
    // 初始化邮件服务
    if (configManager->getBool("email.enabled", false)) {
        // emailService = std::make_shared<EmailService>(...);
        logger->info("邮件服务初始化完成");
    }
    
    // 初始化文件存储服务
    // fileStorageService = std::make_shared<FileStorageService>(...);
    
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
    std::signal(SIGINT, [](int) {
        auto app = Application::getInstance();
        app->stop();
        exit(0);
    });
    
    std::signal(SIGTERM, [](int) {
        auto app = Application::getInstance();
        app->stop();
        exit(0);
    });
    
    logger->info("信号处理器已注册 (SIGINT, SIGTERM)");
}

} // namespace yachiyo