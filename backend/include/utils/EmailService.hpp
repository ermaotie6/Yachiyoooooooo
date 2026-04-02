#pragma once

#include <string>
#include <vector>
#include <memory>

namespace Yachiyo {
namespace Utils {

/**
 * @brief 邮件配置
 */
struct EmailConfig {
    std::string host = "localhost";
    int port = 25;
    std::string username;
    std::string password;
    std::string fromEmail = "noreply@yachiyo.com";
    std::string fromName = "Yachiyo";
    bool useTls = false;
    bool useSsl = false;
    int timeoutSeconds = 10;
};

/**
 * @brief 邮件消息
 */
struct EmailMessage {
    std::string to;
    std::string cc;
    std::string bcc;
    std::string subject;
    std::string body;
    bool isHtml = false;
    std::vector<std::pair<std::string, std::string>> attachments; // (filename, filepath)

    EmailMessage() = default;
    EmailMessage(const std::string& to, const std::string& subject, const std::string& body)
        : to(to), subject(subject), body(body) {
    }
};

/**
 * @brief 邮件服务类
 */
class EmailService {
public:
    /**
     * @brief 构造函数
     * @param config 邮件配置
     */
    explicit EmailService(const EmailConfig& config);
    ~EmailService();

    /**
     * @brief 发送邮件
     * @param message 邮件消息
     * @return 是否成功
     */
    bool sendEmail(const EmailMessage& message);

    /**
     * @brief 发送验证码邮件
     * @param to 收件人邮箱
     * @param code 验证码
     * @return 是否成功
     */
    bool sendVerificationCodeEmail(const std::string& to, const std::string& code);

    /**
     * @brief 发送密码重置邮件
     * @param to 收件人邮箱
     * @param resetLink 重置链接
     * @return 是否成功
     */
    bool sendPasswordResetEmail(const std::string& to, const std::string& resetLink);

    /**
     * @brief 发送欢迎邮件
     * @param to 收件人邮箱
     * @param username 用户名
     * @return 是否成功
     */
    bool sendWelcomeEmail(const std::string& to, const std::string& username);

    /**
     * @brief 验证邮箱配置
     * @return 配置是否有效
     */
    bool validateConfig();

    /**
     * @brief 获取错误信息
     * @return 错误信息
     */
    std::string getError() const;

private:
    EmailConfig config;
    std::string lastError;

    /**
     * @brief 构建SMTP命令
     * @param message 邮件消息
     * @return SMTP命令
     */
    std::string buildSmtpMessage(const EmailMessage& message);

    /**
     * @brief 通过SMTP发送邮件
     * @param message SMTP格式的邮件消息
     * @return 是否成功
     */
    bool sendViaSmtp(const std::string& message);
};

} // namespace Utils
} // namespace Yachiyo
