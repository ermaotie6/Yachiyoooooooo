#pragma once

#include <string>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <memory>
#include "Logger.hpp"

namespace Yachiyo {
namespace Utils {

/**
 * @brief 自定义异常基类
 */
class YachiyoException : public std::runtime_error {
protected:
    int errorCode;
    std::string errorContext;
    std::shared_ptr<Logger> logger;

public:
    YachiyoException(const std::string& message, int code = -1)
        : std::runtime_error(message), errorCode(code) {
    }

    virtual ~YachiyoException() = default;

    int getErrorCode() const { return errorCode; }
    const std::string& getErrorContext() const { return errorContext; }

    void setErrorContext(const std::string& context) {
        errorContext = context;
    }

    virtual std::string getDetailedMessage() const {
        std::ostringstream oss;
        oss << "Error [" << errorCode << "]: " << what();
        if (!errorContext.empty()) {
            oss << " | Context: " << errorContext;
        }
        return oss.str();
    }
};

/**
 * @brief 数据库异常
 */
class DatabaseException : public YachiyoException {
public:
    DatabaseException(const std::string& message, int code = 1001)
        : YachiyoException(message, code) {
    }
};

/**
 * @brief 认证异常
 */
class AuthenticationException : public YachiyoException {
public:
    AuthenticationException(const std::string& message, int code = 2001)
        : YachiyoException(message, code) {
    }
};

/**
 * @brief 授权异常
 */
class AuthorizationException : public YachiyoException {
public:
    AuthorizationException(const std::string& message, int code = 2002)
        : YachiyoException(message, code) {
    }
};

/**
 * @brief 验证异常
 */
class ValidationException : public YachiyoException {
public:
    ValidationException(const std::string& message, int code = 4001)
        : YachiyoException(message, code) {
    }
};

/**
 * @brief Redis 异常
 */
class RedisException : public YachiyoException {
public:
    RedisException(const std::string& message, int code = 3001)
        : YachiyoException(message, code) {
    }
};

/**
 * @brief 邮件异常
 */
class EmailException : public YachiyoException {
public:
    EmailException(const std::string& message, int code = 3002)
        : YachiyoException(message, code) {
    }
};

/**
 * @brief 业务逻辑异常
 */
class BusinessException : public YachiyoException {
public:
    BusinessException(const std::string& message, int code = 5001)
        : YachiyoException(message, code) {
    }
};

/**
 * @brief 异常处理工具类
 */
class ExceptionHandler {
public:
    /**
     * @brief 处理异常并返回 HTTP 状态码
     */
    static int handleException(const std::exception& e, std::string& errorMessage) {
        // 尝试转换为自定义异常
        const auto* yException = dynamic_cast<const YachiyoException*>(&e);
        if (yException) {
            errorMessage = yException->getDetailedMessage();
            return getHTTPStatusCode(yException->getErrorCode());
        }

        // 处理标准异常
        errorMessage = std::string("Internal Error: ") + e.what();
        return 500;
    }

    /**
     * @brief 根据错误代码获取 HTTP 状态码
     */
    static int getHTTPStatusCode(int errorCode) {
        if (errorCode >= 2001 && errorCode <= 2999) {
            return 401; // Unauthorized (认证)
        }
        if (errorCode >= 2002 && errorCode <= 2999) {
            return 403; // Forbidden (授权)
        }
        if (errorCode >= 4001 && errorCode <= 4999) {
            return 400; // Bad Request (验证)
        }
        if (errorCode >= 1001 && errorCode <= 1999) {
            return 500; // Internal Server Error (数据库)
        }
        if (errorCode >= 3001 && errorCode <= 3999) {
            return 503; // Service Unavailable (Redis/外部服务)
        }
        return 500; // 默认内部错误
    }

    /**
     * @brief 安全地执行代码块
     */
    template<typename Func>
    static auto safeExecute(Func&& func, const std::string& operationName = "Operation") {
        try {
            return func();
        } catch (const DatabaseException& e) {
            auto logger = LogUtils::getLogger("ExceptionHandler");
            logger->error(operationName + " 数据库异常: " + e.what());
            throw;
        } catch (const AuthenticationException& e) {
            auto logger = LogUtils::getLogger("ExceptionHandler");
            logger->error(operationName + " 认证异常: " + e.what());
            throw;
        } catch (const AuthorizationException& e) {
            auto logger = LogUtils::getLogger("ExceptionHandler");
            logger->error(operationName + " 授权异常: " + e.what());
            throw;
        } catch (const ValidationException& e) {
            auto logger = LogUtils::getLogger("ExceptionHandler");
            logger->error(operationName + " 验证异常: " + e.what());
            throw;
        } catch (const RedisException& e) {
            auto logger = LogUtils::getLogger("ExceptionHandler");
            logger->error(operationName + " Redis异常: " + e.what());
            throw;
        } catch (const EmailException& e) {
            auto logger = LogUtils::getLogger("ExceptionHandler");
            logger->error(operationName + " 邮件异常: " + e.what());
            throw;
        } catch (const BusinessException& e) {
            auto logger = LogUtils::getLogger("ExceptionHandler");
            logger->error(operationName + " 业务异常: " + e.what());
            throw;
        } catch (const std::exception& e) {
            auto logger = LogUtils::getLogger("ExceptionHandler");
            logger->error(operationName + " 未知异常: " + std::string(e.what()));
            throw;
        }
    }

    /**
     * @brief 记录异常堆栈信息
     */
    static void logException(const std::exception& e, const std::string& context = "") {
        auto logger = LogUtils::getLogger("ExceptionHandler");
        
        const auto* yException = dynamic_cast<const YachiyoException*>(&e);
        if (yException) {
            logger->error("异常 [" + std::to_string(yException->getErrorCode()) + "]: " +
                         std::string(e.what()) + " | " + context);
        } else {
            logger->error("未知异常: " + std::string(e.what()) + " | " + context);
        }
    }
};

} // namespace Utils
} // namespace Yachiyo
