#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * Live2D 动画命令 DTO
 */
struct Live2DCommand {
    std::string type;  // "expression", "motion", "parameter", "sequence"
    json params;
    
    json toJson() const {
        json j;
        j["type"] = type;
        j["params"] = params;
        return j;
    }
    
    static Live2DCommand fromJson(const json& j) {
        Live2DCommand cmd;
        if (j.contains("type")) cmd.type = j["type"];
        if (j.contains("params")) cmd.params = j["params"];
        return cmd;
    }
};

/**
 * 表情命令 DTO
 */
struct ExpressionCommand : Live2DCommand {
    std::string expressionName;
    int durationMs = 2000;
    
    ExpressionCommand() {
        type = "expression";
    }
    
    static ExpressionCommand fromJson(const json& j) {
        ExpressionCommand cmd;
        if (j.contains("type")) cmd.type = j["type"];
        if (j.contains("expression_name")) cmd.expressionName = j["expression_name"];
        if (j.contains("duration_ms")) cmd.durationMs = j["duration_ms"];
        cmd.params = j;
        return cmd;
    }
};

/**
 * 动作命令 DTO
 */
struct MotionCommand : Live2DCommand {
    std::string group;
    int index = 0;
    bool loop = false;
    int priority = 1;
    
    MotionCommand() {
        type = "motion";
    }
    
    static MotionCommand fromJson(const json& j) {
        MotionCommand cmd;
        if (j.contains("type")) cmd.type = j["type"];
        if (j.contains("group")) cmd.group = j["group"];
        if (j.contains("index")) cmd.index = j["index"];
        if (j.contains("loop")) cmd.loop = j["loop"];
        if (j.contains("priority")) cmd.priority = j["priority"];
        cmd.params = j;
        return cmd;
    }
};

/**
 * 参数命令 DTO (直接设置模型参数)
 */
struct ParameterCommand : Live2DCommand {
    std::string paramName;
    float value = 0.0;
    int transitionMs = 0;
    
    ParameterCommand() {
        type = "parameter";
    }
    
    static ParameterCommand fromJson(const json& j) {
        ParameterCommand cmd;
        if (j.contains("type")) cmd.type = j["type"];
        if (j.contains("param_name")) cmd.paramName = j["param_name"];
        if (j.contains("value")) cmd.value = j["value"];
        if (j.contains("transition_ms")) cmd.transitionMs = j["transition_ms"];
        cmd.params = j;
        return cmd;
    }
};

/**
 * Live2D 动作序列请求 DTO
 */
struct Live2DSequenceRequest {
    std::string requestId;
    std::vector<Live2DCommand> commands;
    bool autoPlay = true;
    
    json toJson() const {
        json j;
        j["request_id"] = requestId;
        json cmdArray = json::array();
        for (const auto& cmd : commands) {
            cmdArray.push_back(cmd.toJson());
        }
        j["commands"] = cmdArray;
        j["auto_play"] = autoPlay;
        return j;
    }
    
    static Live2DSequenceRequest fromJson(const json& j) {
        Live2DSequenceRequest req;
        if (j.contains("request_id")) req.requestId = j["request_id"];
        if (j.contains("auto_play")) req.autoPlay = j["auto_play"];
        
        if (j.contains("commands")) {
            for (const auto& cmdJson : j["commands"]) {
                Live2DCommand cmd;
                if (cmdJson.contains("type")) {
                    std::string type = cmdJson["type"];
                    cmd.type = type;
                }
                cmd.params = cmdJson;
                req.commands.push_back(cmd);
            }
        }
        
        return req;
    }
};

/**
 * Live2D 动作序列响应 DTO
 */
struct Live2DSequenceResponse {
    std::string requestId;
    bool success = false;
    int commandCount = 0;
    int totalDurationMs = 0;
    std::string status;  // idle / playing / completed
    std::string errorMessage;
    
    json toJson() const {
        json j;
        j["request_id"] = requestId;
        j["success"] = success;
        j["command_count"] = commandCount;
        j["total_duration_ms"] = totalDurationMs;
        j["status"] = status;
        if (!errorMessage.empty()) {
            j["error"] = errorMessage;
        }
        return j;
    }
    
    static Live2DSequenceResponse fromJson(const json& j) {
        Live2DSequenceResponse resp;
        if (j.contains("request_id")) resp.requestId = j["request_id"];
        if (j.contains("success")) resp.success = j["success"];
        if (j.contains("command_count")) resp.commandCount = j["command_count"];
        if (j.contains("total_duration_ms")) resp.totalDurationMs = j["total_duration_ms"];
        if (j.contains("status")) resp.status = j["status"];
        if (j.contains("error")) resp.errorMessage = j["error"];
        return resp;
    }
};
