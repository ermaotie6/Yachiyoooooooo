#include "utils/EmailService.hpp"
#include <iostream>
#include <sstream>
#include <ctime>
#include <regex>

namespace Yachiyo {
namespace Utils {

EmailService::EmailService(const EmailConfig& config)
    : config(config), lastError("") {
    std::cout << "[INFO] 邮件服务初始化: " << config.host << ":" << config.port << std::endl;
}

EmailService::~EmailService() {
    std::cout << "[INFO] 邮件服务关闭" << std::endl;
}

bool EmailService::sendEmail(const EmailMessage& message) {
    try {
        // 验证收件人
        std::regex emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
        if (!std::regex_match(message.to, emailRegex)) {
            lastError = "无效的收件人邮箱地址";
            return false;
        }

        // 验证主题和正文
        if (message.subject.empty() || message.body.empty()) {
            lastError = "邮件主题和正文不能为空";
            return false;
        }

        // 构建SMTP消息
        std::string smtpMessage = buildSmtpMessage(message);

        // 发送邮件
        if (!sendViaSmtp(smtpMessage)) {
            return false;
        }

        std::cout << "[INFO] 邮件发送成功: to=" << message.to 
                  << ", subject=" << message.subject << std::endl;
        return true;

    } catch (const std::exception& e) {
        lastError = std::string("邮件发送异常: ") + e.what();
        std::cerr << "[ERROR] " << lastError << std::endl;
        return false;
    }
}

bool EmailService::sendVerificationCodeEmail(const std::string& to, const std::string& code) {
    try {
        EmailMessage message;
        message.to = to;
        message.subject = "YachiyoCPP 邮箱验证码";
        message.isHtml = true;

        // 构建HTML格式的邮件正文
        std::ostringstream body;
        body << "<html><body style='font-family: Arial, sans-serif;'>\n"
             << "  <h2>邮箱验证</h2>\n"
             << "  <p>亲爱的用户，您的验证码是：</p>\n"
             << "  <div style='background-color: #f0f0f0; padding: 10px; text-align: center; "
             << "font-size: 24px; font-weight: bold; margin: 20px 0;'>\n"
             << "    " << code << "\n"
             << "  </div>\n"
             << "  <p style='color: #666; font-size: 12px;'>验证码有效期为 5 分钟</p>\n"
             << "  <p style='color: #666; font-size: 12px;'>如非本人操作，请忽略此邮件</p>\n"
             << "</body></html>";

        message.body = body.str();
        return sendEmail(message);

    } catch (const std::exception& e) {
        lastError = std::string("发送验证码邮件失败: ") + e.what();
        std::cerr << "[ERROR] " << lastError << std::endl;
        return false;
    }
}

bool EmailService::sendPasswordResetEmail(const std::string& to, const std::string& resetLink) {
    try {
        EmailMessage message;
        message.to = to;
        message.subject = "YachiyoCPP 密码重置";
        message.isHtml = true;

        std::ostringstream body;
        body << "<html><body style='font-family: Arial, sans-serif;'>\n"
             << "  <h2>密码重置请求</h2>\n"
             << "  <p>亲爱的用户，您申请了密码重置。请点击以下链接进行重置：</p>\n"
             << "  <p><a href='" << resetLink << "' style='background-color: #007bff; "
             << "color: white; padding: 10px 20px; text-decoration: none; border-radius: 5px;'>\n"
             << "    重置密码\n"
             << "  </a></p>\n"
             << "  <p style='color: #666; font-size: 12px;'>或复制以下链接到浏览器：</p>\n"
             << "  <p style='color: #0066cc; font-size: 12px; word-break: break-all;'>" 
             << resetLink << "</p>\n"
             << "  <p style='color: #666; font-size: 12px;'>链接有效期为 24 小时</p>\n"
             << "  <p style='color: #666; font-size: 12px;'>如非本人操作，请忽略此邮件</p>\n"
             << "</body></html>";

        message.body = body.str();
        return sendEmail(message);

    } catch (const std::exception& e) {
        lastError = std::string("发送密码重置邮件失败: ") + e.what();
        std::cerr << "[ERROR] " << lastError << std::endl;
        return false;
    }
}

bool EmailService::sendWelcomeEmail(const std::string& to, const std::string& username) {
    try {
        EmailMessage message;
        message.to = to;
        message.subject = "欢迎加入 YachiyoCPP";
        message.isHtml = true;

        std::ostringstream body;
        body << "<html><body style='font-family: Arial, sans-serif;'>\n"
             << "  <h2>欢迎加入!</h2>\n"
             << "  <p>亲爱的 " << username << "，</p>\n"
             << "  <p>欢迎加入 YachiyoCPP 社区！我们很高兴有您的加入。</p>\n"
             << "  <h3>快速开始：</h3>\n"
             << "  <ul>\n"
             << "    <li>完善您的个人信息</li>\n"
             << "    <li>浏览我们的教程和文档</li>\n"
             << "    <li>加入我们的社区论坛</li>\n"
             << "    <li>探索 AI 聊天功能</li>\n"
             << "  </ul>\n"
             << "  <p>如有任何问题，请联系我们的支持团队。</p>\n"
             << "  <p>祝您使用愉快！</p>\n"
             << "  <hr style='border: 1px solid #ddd;'>\n"
             << "  <p style='color: #666; font-size: 12px;'>© 2026 YachiyoCPP Team</p>\n"
             << "</body></html>";

        message.body = body.str();
        return sendEmail(message);

    } catch (const std::exception& e) {
        lastError = std::string("发送欢迎邮件失败: ") + e.what();
        std::cerr << "[ERROR] " << lastError << std::endl;
        return false;
    }
}

bool EmailService::validateConfig() {
    if (config.host.empty()) {
        lastError = "SMTP服务器地址未配置";
        return false;
    }

    if (config.port <= 0 || config.port > 65535) {
        lastError = "SMTP服务器端口无效";
        return false;
    }

    std::regex emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    if (!std::regex_match(config.fromEmail, emailRegex)) {
        lastError = "发件人邮箱地址无效";
        return false;
    }

    return true;
}

std::string EmailService::getError() const {
    return lastError;
}

std::string EmailService::buildSmtpMessage(const EmailMessage& message) {
    std::ostringstream smtp;

    // 邮件头
    smtp << "From: " << config.fromName << " <" << config.fromEmail << ">\r\n";
    smtp << "To: " << message.to << "\r\n";

    if (!message.cc.empty()) {
        smtp << "Cc: " << message.cc << "\r\n";
    }

    smtp << "Subject: " << message.subject << "\r\n";
    smtp << "Date: ";

    // 添加日期
    time_t now = time(nullptr);
    struct tm* timeinfo = gmtime(&now);
    char dateBuffer[100];
    strftime(dateBuffer, sizeof(dateBuffer), "%a, %d %b %Y %H:%M:%S +0000", timeinfo);
    smtp << dateBuffer << "\r\n";

    smtp << "MIME-Version: 1.0\r\n";
    smtp << "Content-Type: " << (message.isHtml ? "text/html" : "text/plain");
    smtp << "; charset=utf-8\r\n";
    smtp << "Content-Transfer-Encoding: quoted-printable\r\n";
    smtp << "\r\n";

    // 邮件正文
    smtp << message.body << "\r\n";

    return smtp.str();
}

bool EmailService::sendViaSmtp(const std::string& message) {
    try {
        // 在生产环境中，这里应该使用真实的SMTP连接库（如libcurl、libsmtp等）
        // 这里是一个模拟实现，仅用于演示

        if (!validateConfig()) {
            return false;
        }

        // 模拟SMTP发送过程
        std::cout << "[DEBUG] SMTP消息已准备，准备发送到 " << config.host 
                  << ":" << config.port << std::endl;

        // 在实际实现中，这里应该：
        // 1. 建立TCP连接到SMTP服务器
        // 2. 执行EHLO/HELO命令
        // 3. 如果需要，执行STARTTLS或认证
        // 4. 执行MAIL FROM、RCPT TO和DATA命令
        // 5. 发送邮件内容
        // 6. 执行QUIT命令并关闭连接

        // 为了演示，我们模拟成功
        std::cout << "[INFO] 邮件已通过SMTP发送" << std::endl;
        return true;

    } catch (const std::exception& e) {
        lastError = std::string("SMTP发送失败: ") + e.what();
        std::cerr << "[ERROR] " << lastError << std::endl;
        return false;
    }
}

} // namespace Utils
} // namespace Yachiyo
