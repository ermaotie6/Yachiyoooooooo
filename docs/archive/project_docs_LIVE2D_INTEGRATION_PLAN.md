# 🎨 Live2D 虚拟形象集成方案

> **目的**: 将八千代辉夜姬 Live2D 形象集成到 Yachiyo 虚拟主播系统  
> **难度**: ⭐⭐⭐ 中等  
> **工作量**: 40-60 小时  
> **创建日期**: 2026年4月2日

---

## 📋 概述

### 集成目标

将 Live2D 3D 虚拟形象与 OpenClaw 自主代理、消息系统、表情系统完整对接，实现：

✅ **实时虚拟主播形象显示**  
✅ **情感识别 → 表情自动切换**  
✅ **参数驱动 → 虚拟形象实时反应**  
✅ **多平台适配** (Web, Discord, Telegram 等)  
✅ **自动视频生成** (用于发送到社交媒体)

---

## 🏗️ 架构设计

### 系统组件关系

```
┌──────────────────────────────────────────────────────────────┐
│                   OpenClaw 自主代理                           │
│              (执行虚拟主播任务、生成内容)                    │
└──────────────────┬───────────────────────────────────────────┘
                   │ 命令
                   ▼
┌──────────────────────────────────────────────────────────────┐
│         Yachiyo 后端 API 服务                                │
├──────────────────────────────────────────────────────────────┤
│ ┌────────────────┐  ┌─────────────────┐  ┌───────────────┐ │
│ │ 消息处理服务   │  │ 表情识别引擎    │  │ 虚拟形象渲染  │ │
│ │ MessageService │  │ EmotionAnalyzer │  │ Live2DRenderer│ │
│ └────────┬───────┘  └────────┬────────┘  └───────┬───────┘ │
│          │                   │                    │         │
│          └───────────────────┼────────────────────┘         │
│                              │                              │
│          ┌───────────────────▼──────────────────┐          │
│          │   Live2D 模型管理器                  │          │
│          │   Live2DModelManager                 │          │
│          ├────────────────────────────────────┤          │
│          │ - 模型加载 (model3.json)            │          │
│          │ - 参数驱动 (cdi3.json)              │          │
│          │ - 表情应用 (exp3.json)              │          │
│          │ - 物理模拟 (physics3.json)          │          │
│          │ - 渲染输出 (PNG/MP4)                │          │
│          └────────────────────────────────────┘          │
│                                                             │
└──────────────────────────────────────────────────────────────┘
                   │
    ┌──────────────┼──────────────┐
    │              │              │
    ▼              ▼              ▼
┌─────────┐  ┌──────────┐  ┌──────────┐
│ Web UI  │  │ Discord  │  │ Telegram │
│ 实时显示 │  │ Bot 发送 │  │ 发送图片 │
└─────────┘  └──────────┘  └──────────┘
```

### 核心模块列表

| 模块 | 职责 | 技术栈 |
|------|------|--------|
| **Live2DModelManager** | 管理模型生命周期 | C++20 + Live2D SDK |
| **EmotionAnalyzer** | 分析消息情感 → 表情 | NLP + 规则引擎 |
| **ParameterDriver** | 驱动模型参数变化 | 时序控制 |
| **Live2DRenderer** | 渲染模型 → 图片 | OpenGL / GPU |
| **VideoEncoder** | 生成视频 | FFmpeg |
| **Live2DWebComponent** | Web 前端显示 | Pixi.js + WebGL |

---

## 🛠️ 实现方案

### 阶段 1: Live2D SDK 集成 (5-10 小时)

#### 1.1 添加 Live2D SDK 依赖

**修改**: `YachiyoCPP/CMakeLists.txt`

```cmake
# Live2D Cubism SDK
set(LIVE2D_SDK_PATH "${CMAKE_SOURCE_DIR}/third_party/CubismSdkForNative")

if(EXISTS ${LIVE2D_SDK_PATH})
    add_subdirectory(${LIVE2D_SDK_PATH})
    include_directories(${LIVE2D_SDK_PATH}/Framework/src)
    link_directories(${LIVE2D_SDK_PATH}/Framework/out/cmake/linux/x86_64/Release)
endif()

target_link_libraries(yachiyo_server 
    cubism
    glfw
    glew
)
```

#### 1.2 创建 Live2D 模型管理器

**新建**: `include/models/Live2DModel.hpp`

```cpp
#pragma once

#include <string>
#include <memory>
#include <map>
#include <json/json.h>
#include "CubismModelJson.hpp"

namespace Models {

class Expression {
public:
    Expression() = default;
    bool load(const std::string& filePath);
    void apply(class Live2DModel* model);
    
private:
    Json::Value expressionData;
};

class Live2DModel {
public:
    Live2DModel();
    ~Live2DModel();
    
    // 模型生命周期
    bool loadModel(const std::string& modelPath);
    void update(float deltaTime);
    void draw();
    
    // 参数控制 (来自 cdi3.json)
    bool setParameter(const std::string& paramId, float value);
    float getParameter(const std::string& paramId) const;
    
    // 表情控制 (来自 *.exp3.json)
    bool loadExpression(const std::string& name, 
                       const std::string& expressionPath);
    bool applyExpression(const std::string& name, float blend = 1.0f);
    void clearExpressions();
    
    // 物理模拟
    void updatePhysics(float deltaTime);
    
    // 渲染输出
    struct RenderOutput {
        int width;
        int height;
        unsigned char* rgbaData;  // RGBA 图像数据
        size_t dataSize;
    };
    
    RenderOutput renderToImage();
    std::string renderToBase64();
    
    // 调试
    void logModelInfo() const;
    
private:
    std::unique_ptr<Csm::CubismModel> model;
    std::map<std::string, Expression> expressions;
    std::string modelPath;
    
    bool loadFileAsString(const std::string& filePath, 
                         std::string& outData) const;
};

}  // namespace Models
```

#### 1.3 创建 Live2D 模型管理器实现

**新建**: `src/models/Live2DModel.cpp`

```cpp
#include "models/Live2DModel.hpp"
#include "utils/Logger.hpp"
#include "utils/JsonUtils.hpp"
#include <fstream>
#include <glm/glm.hpp>

namespace Models {

Live2DModel::Live2DModel() {}

Live2DModel::~Live2DModel() {
    // Cubism SDK 清理
}

bool Live2DModel::loadModel(const std::string& modelPath) {
    this->modelPath = modelPath;
    
    LOG_INFO("加载 Live2D 模型: {}", modelPath);
    
    // 读取 model3.json
    std::string modelJson;
    if (!loadFileAsString(modelPath, modelJson)) {
        LOG_ERROR("无法读取模型文件: {}", modelPath);
        return false;
    }
    
    try {
        // 解析 model3.json
        Json::Value root = Utils::JsonUtils::parse(modelJson);
        
        // 加载 MOC3 文件
        std::string mocPath = root["FileReferences"]["Moc"].asString();
        LOG_INFO("加载 MOC3: {}", mocPath);
        
        // 加载纹理
        auto textures = root["FileReferences"]["Textures"];
        for (const auto& tex : textures) {
            LOG_INFO("加载纹理: {}", tex.asString());
        }
        
        // 加载物理配置
        std::string physicsPath = 
            root["FileReferences"]["Physics"].asString();
        LOG_INFO("加载物理配置: {}", physicsPath);
        
        // 加载参数定义
        std::string cdiPath = 
            root["FileReferences"]["DisplayInfo"].asString();
        LOG_INFO("加载参数定义: {}", cdiPath);
        
        LOG_INFO("Live2D 模型加载成功");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("解析模型文件失败: {}", e.what());
        return false;
    }
}

bool Live2DModel::setParameter(const std::string& paramId, 
                               float value) {
    if (!model) {
        LOG_ERROR("模型未加载");
        return false;
    }
    
    // 设置参数值 (0.0 - 1.0)
    float clampedValue = std::max(0.0f, std::min(1.0f, value));
    
    // 调用 Cubism SDK 设置参数
    // model->SetParameterValueById(paramId, clampedValue);
    
    LOG_DEBUG("设置参数 {}: {}", paramId, clampedValue);
    return true;
}

bool Live2DModel::applyExpression(const std::string& name, 
                                 float blend) {
    auto it = expressions.find(name);
    if (it == expressions.end()) {
        LOG_WARN("表情不存在: {}", name);
        return false;
    }
    
    LOG_INFO("应用表情: {} (blend: {})", name, blend);
    it->second.apply(this);
    return true;
}

void Live2DModel::update(float deltaTime) {
    if (!model) return;
    
    // 更新模型参数
    // model->Update();
    
    // 更新物理模拟
    updatePhysics(deltaTime);
}

void Live2DModel::updatePhysics(float deltaTime) {
    if (!model) return;
    
    // 调用 Cubism SDK 物理引擎
    // physics->Evaluate(model.get(), deltaTime);
}

Live2DModel::RenderOutput Live2DModel::renderToImage() {
    RenderOutput output;
    output.width = 1920;
    output.height = 1080;
    
    // 使用 OpenGL 渲染到纹理
    // TODO: 实现 FBO 渲染
    
    return output;
}

std::string Live2DModel::renderToBase64() {
    auto output = renderToImage();
    // TODO: 转换为 Base64 编码
    return "";
}

bool Live2DModel::loadFileAsString(const std::string& filePath, 
                                   std::string& outData) const {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    outData.resize(size);
    file.seekg(0, std::ios::beg);
    file.read(&outData[0], size);
    
    return true;
}

void Live2DModel::logModelInfo() const {
    LOG_INFO("=== Live2D 模型信息 ===");
    LOG_INFO("模型路径: {}", modelPath);
    LOG_INFO("参数数量: {}", 150);  // 从 cdi3.json 获取
    LOG_INFO("表情数量: {}", expressions.size());
    LOG_INFO("物理规则数: {}", 183);
}

}  // namespace Models
```

---

### 阶段 2: 情感识别与表情映射 (8-12 小时)

#### 2.1 创建情感分析器

**新建**: `include/models/EmotionAnalyzer.hpp`

```cpp
#pragma once

#include <string>
#include <vector>
#include <map>

namespace Models {

enum class Emotion {
    HAPPY,      // 开心 → 笑咪咪.exp3.json
    SHY,        // 害羞 → 眯眯眼.exp3.json
    SAD,        // 难过 → 眼泪.exp3.json
    CRYING,     // 哭泣 → 泪珠.exp3.json
    NEUTRAL     // 中立 → 无表情
};

struct EmotionScore {
    Emotion emotion;
    float confidence;  // 0.0 - 1.0
    float intensity;   // 情感强度
};

class EmotionAnalyzer {
public:
    EmotionAnalyzer();
    
    // 分析文本情感
    EmotionScore analyzeText(const std::string& text);
    
    // 获取表情文件路径
    std::string getExpressionFile(Emotion emotion) const;
    
    // 获取参数调整
    struct ParameterAdjustment {
        std::string paramId;
        float value;
    };
    std::vector<ParameterAdjustment> getParameterAdjustments(
        Emotion emotion, float intensity);
    
private:
    // 情感关键词映射
    std::map<std::string, Emotion> emotionKeywords;
    
    void initializeKeywords();
    Emotion detectEmotion(const std::string& text);
    float calculateIntensity(const std::string& text);
};

}  // namespace Models
```

#### 2.2 创建情感分析器实现

**新建**: `src/models/EmotionAnalyzer.cpp`

```cpp
#include "models/EmotionAnalyzer.hpp"
#include "utils/Logger.hpp"
#include <algorithm>
#include <cctype>

namespace Models {

EmotionAnalyzer::EmotionAnalyzer() {
    initializeKeywords();
}

void EmotionAnalyzer::initializeKeywords() {
    // 开心关键词
    emotionKeywords["开心"] = Emotion::HAPPY;
    emotionKeywords["高兴"] = Emotion::HAPPY;
    emotionKeywords["开玩笑"] = Emotion::HAPPY;
    emotionKeywords["哈哈"] = Emotion::HAPPY;
    emotionKeywords["棒"] = Emotion::HAPPY;
    emotionKeywords["赞"] = Emotion::HAPPY;
    emotionKeywords["太好了"] = Emotion::HAPPY;
    
    // 害羞关键词
    emotionKeywords["害羞"] = Emotion::SHY;
    emotionKeywords["脸红"] = Emotion::SHY;
    emotionKeywords["不好意思"] = Emotion::SHY;
    emotionKeywords["呃"] = Emotion::SHY;
    emotionKeywords["嘛"] = Emotion::SHY;
    
    // 难过关键词
    emotionKeywords["难过"] = Emotion::SAD;
    emotionKeywords["伤心"] = Emotion::SAD;
    emotionKeywords["失望"] = Emotion::SAD;
    emotionKeywords["糟糕"] = Emotion::SAD;
    emotionKeywords["讨厌"] = Emotion::SAD;
    
    // 哭泣关键词
    emotionKeywords["哭"] = Emotion::CRYING;
    emotionKeywords["眼泪"] = Emotion::CRYING;
    emotionKeywords["泣"] = Emotion::CRYING;
    emotionKeywords["抽泣"] = Emotion::CRYING;
}

EmotionScore EmotionAnalyzer::analyzeText(const std::string& text) {
    EmotionScore score;
    score.emotion = detectEmotion(text);
    score.intensity = calculateIntensity(text);
    score.confidence = 0.7f + (score.intensity * 0.3f);  // 0.7 - 1.0
    
    LOG_DEBUG("情感分析: 情感={}, 强度={}, 置信度={}",
              static_cast<int>(score.emotion),
              score.intensity,
              score.confidence);
    
    return score;
}

Emotion EmotionAnalyzer::detectEmotion(const std::string& text) {
    // 简单的关键词匹配
    for (const auto& [keyword, emotion] : emotionKeywords) {
        if (text.find(keyword) != std::string::npos) {
            return emotion;
        }
    }
    
    return Emotion::NEUTRAL;
}

float EmotionAnalyzer::calculateIntensity(const std::string& text) {
    float intensity = 0.5f;  // 默认中等强度
    
    // 统计大写字母数量 (表示强调)
    int upperCount = 0;
    for (char c : text) {
        if (std::isupper(c)) upperCount++;
    }
    
    if (upperCount > text.length() * 0.3f) {
        intensity = 0.9f;  // 高强度
    }
    
    // 统计感叹号数量
    int exclamationCount = std::count(text.begin(), text.end(), '!');
    if (exclamationCount > 2) {
        intensity = std::max(intensity, 0.8f);
    }
    
    return std::min(intensity, 1.0f);
}

std::string EmotionAnalyzer::getExpressionFile(Emotion emotion) const {
    switch (emotion) {
        case Emotion::HAPPY:
            return "yachiyo_live2d/笑咪咪.exp3.json";
        case Emotion::SHY:
            return "yachiyo_live2d/眯眯眼.exp3.json";
        case Emotion::SAD:
            return "yachiyo_live2d/眼泪.exp3.json";
        case Emotion::CRYING:
            return "yachiyo_live2d/泪珠.exp3.json";
        case Emotion::NEUTRAL:
        default:
            return "";
    }
}

std::vector<EmotionAnalyzer::ParameterAdjustment>
EmotionAnalyzer::getParameterAdjustments(Emotion emotion, 
                                         float intensity) {
    std::vector<ParameterAdjustment> adjustments;
    
    switch (emotion) {
        case Emotion::HAPPY:
            // 开心: 嘴巴上扬，眼睛眯细
            adjustments.push_back({"ParamMouthSmile", intensity});
            adjustments.push_back({"ParamEyeLOpen", 0.5f});
            adjustments.push_back({"ParamEyeROpen", 0.5f});
            break;
            
        case Emotion::SHY:
            // 害羞: 眼睛眯细，低头
            adjustments.push_back({"ParamEyeLOpen", 0.3f});
            adjustments.push_back({"ParamEyeROpen", 0.3f});
            adjustments.push_back({"ParamHeadAngleY", 15.0f});
            break;
            
        case Emotion::SAD:
            // 难过: 眉毛下压，嘴巴下扬
            adjustments.push_back({"ParamBrowLY", -intensity});
            adjustments.push_back({"ParamBrowRY", -intensity});
            adjustments.push_back({"ParamMouthOpenY", -intensity * 0.5f});
            break;
            
        case Emotion::CRYING:
            // 哭泣: 完全闭眼，嘴巴开大
            adjustments.push_back({"ParamEyeLOpen", 0.0f});
            adjustments.push_back({"ParamEyeROpen", 0.0f});
            adjustments.push_back({"ParamMouthOpenY", 1.0f});
            break;
            
        case Emotion::NEUTRAL:
        default:
            break;
    }
    
    return adjustments;
}

}  // namespace Models
```

---

### 阶段 3: 虚拟形象渲染服务 (10-15 小时)

#### 3.1 创建虚拟形象渲染服务

**新建**: `include/services/Live2DRenderService.hpp`

```cpp
#pragma once

#include <string>
#include <memory>
#include "models/Live2DModel.hpp"
#include "models/EmotionAnalyzer.hpp"

namespace Services {

class Live2DRenderService {
public:
    Live2DRenderService();
    ~Live2DRenderService();
    
    // 初始化
    bool initialize(const std::string& modelPath);
    
    // 渲染虚拟形象 (基于消息文本)
    struct RenderResult {
        std::string imageBase64;  // PNG 图片 Base64
        std::string videoUrl;     // MP4 视频 URL (可选)
        Models::Emotion emotion;
        float emotionIntensity;
    };
    
    RenderResult renderByMessage(const std::string& messageText);
    
    // 渲染虚拟形象 (基于指定表情)
    RenderResult renderByExpression(const std::string& expressionName);
    
    // 渲染虚拟形象 (基于参数)
    struct ParameterFrame {
        std::map<std::string, float> parameters;
        float duration;  // 帧持续时间
    };
    RenderResult renderByParameters(
        const std::vector<ParameterFrame>& frames);
    
    // 生成动画视频
    std::string generateAnimationVideo(
        const std::vector<ParameterFrame>& frames,
        const std::string& outputPath);
    
    // 获取模型信息
    void logModelInfo() const;
    
private:
    std::unique_ptr<Models::Live2DModel> model;
    std::unique_ptr<Models::EmotionAnalyzer> emotionAnalyzer;
    
    // 参数驱动过程
    void animateParameters(const std::vector<ParameterFrame>& frames);
};

}  // namespace Services
```

#### 3.2 创建虚拟形象渲染服务实现

**新建**: `src/services/Live2DRenderService.cpp`

```cpp
#include "services/Live2DRenderService.hpp"
#include "utils/Logger.hpp"
#include <thread>
#include <chrono>

namespace Services {

Live2DRenderService::Live2DRenderService() 
    : model(std::make_unique<Models::Live2DModel>()),
      emotionAnalyzer(std::make_unique<Models::EmotionAnalyzer>()) {
}

Live2DRenderService::~Live2DRenderService() = default;

bool Live2DRenderService::initialize(const std::string& modelPath) {
    LOG_INFO("初始化 Live2D 渲染服务");
    
    if (!model->loadModel(modelPath)) {
        LOG_ERROR("无法加载 Live2D 模型");
        return false;
    }
    
    // 预加载所有表情
    std::vector<std::string> expressions = {
        "笑咪咪", "眯眯眼", "眼泪", "泪珠"
    };
    
    for (const auto& expr : expressions) {
        std::string exprPath = "yachiyo_live2d/" + expr + ".exp3.json";
        if (!model->loadExpression(expr, exprPath)) {
            LOG_WARN("无法加载表情: {}", expr);
        }
    }
    
    model->logModelInfo();
    return true;
}

Live2DRenderService::RenderResult 
Live2DRenderService::renderByMessage(const std::string& messageText) {
    LOG_INFO("根据消息渲染虚拟形象: {}", messageText);
    
    // 分析情感
    auto emotionScore = emotionAnalyzer->analyzeText(messageText);
    
    // 获取对应的表情文件
    std::string expressionFile = 
        emotionAnalyzer->getExpressionFile(emotionScore.emotion);
    
    // 获取参数调整
    auto paramAdjustments = 
        emotionAnalyzer->getParameterAdjustments(
            emotionScore.emotion, emotionScore.intensity);
    
    // 应用表情
    if (!expressionFile.empty()) {
        std::string exprName;
        if (emotionScore.emotion == Models::Emotion::HAPPY) {
            exprName = "笑咪咪";
        } else if (emotionScore.emotion == Models::Emotion::SHY) {
            exprName = "眯眯眼";
        } else if (emotionScore.emotion == Models::Emotion::SAD) {
            exprName = "眼泪";
        } else if (emotionScore.emotion == Models::Emotion::CRYING) {
            exprName = "泪珠";
        }
        
        model->applyExpression(exprName, emotionScore.confidence);
    }
    
    // 应用参数调整
    for (const auto& adjustment : paramAdjustments) {
        model->setParameter(adjustment.paramId, adjustment.value);
    }
    
    // 更新模型
    model->update(0.016f);  // 16ms (60fps)
    
    // 渲染输出
    RenderResult result;
    result.imageBase64 = model->renderToBase64();
    result.emotion = emotionScore.emotion;
    result.emotionIntensity = emotionScore.intensity;
    
    LOG_INFO("渲染完成");
    return result;
}

Live2DRenderService::RenderResult 
Live2DRenderService::renderByExpression(
    const std::string& expressionName) {
    LOG_INFO("根据表情渲染: {}", expressionName);
    
    model->applyExpression(expressionName);
    model->update(0.016f);
    
    RenderResult result;
    result.imageBase64 = model->renderToBase64();
    
    return result;
}

std::string Live2DRenderService::generateAnimationVideo(
    const std::vector<ParameterFrame>& frames,
    const std::string& outputPath) {
    LOG_INFO("生成动画视频: {}", outputPath);
    
    // 逐帧渲染
    std::vector<std::string> frameImages;
    for (size_t i = 0; i < frames.size(); ++i) {
        // 应用参数
        for (const auto& [paramId, value] : frames[i].parameters) {
            model->setParameter(paramId, value);
        }
        
        // 更新模型
        float deltaTime = frames[i].duration / 1000.0f;
        model->update(deltaTime);
        
        // 渲染
        frameImages.push_back(model->renderToBase64());
        
        LOG_DEBUG("渲染帧 {}/{}", i + 1, frames.size());
    }
    
    // 使用 FFmpeg 生成视频
    // ffmpeg -framerate 60 -i frame_%04d.png -c:v libx264 output.mp4
    
    LOG_INFO("视频生成完成: {}", outputPath);
    return outputPath;
}

void Live2DRenderService::logModelInfo() const {
    model->logModelInfo();
}

}  // namespace Services
```

---

### 阶段 4: API 端点开发 (8-12 小时)

#### 4.1 创建虚拟形象 API 路由

**修改**: `include/controllers/routes.hpp`

```cpp
// 在现有路由中添加

// 虚拟形象 API
router->post("/api/avatar/render-by-message",
    handlers::avatar::renderByMessage);

router->post("/api/avatar/render-by-expression",
    handlers::avatar::renderByExpression);

router->post("/api/avatar/generate-video",
    handlers::avatar::generateVideo);

router->get("/api/avatar/model-info",
    handlers::avatar::getModelInfo);
```

#### 4.2 创建虚拟形象控制器

**新建**: `src/controllers/AvatarController.cpp`

```cpp
#include <controllers/AvatarController.hpp>
#include <services/Live2DRenderService.hpp>
#include <utils/JsonUtils.hpp>
#include <utils/Logger.hpp>

namespace Controllers {

namespace {
    std::unique_ptr<Services::Live2DRenderService> g_renderService;
}

class AvatarController : public BaseController {
public:
    static void initialize() {
        g_renderService = std::make_unique<Services::Live2DRenderService>();
        g_renderService->initialize("yachiyo_live2d/八千代辉夜姬.model3.json");
    }
    
    // POST /api/avatar/render-by-message
    static crow::response renderByMessage(
        const crow::request& req,
        const std::string& token) {
        
        try {
            auto body = crow::json::load(req.body);
            std::string message = body["message"].s();
            
            auto result = g_renderService->renderByMessage(message);
            
            return crow::response(200, crow::json::wvalue{
                {"status", "success"},
                {"emotion", static_cast<int>(result.emotion)},
                {"intensity", result.emotionIntensity},
                {"image_base64", result.imageBase64}
            }.dump());
            
        } catch (const std::exception& e) {
            LOG_ERROR("渲染失败: {}", e.what());
            return crow::response(500, errorResponse("渲染失败"));
        }
    }
    
    // POST /api/avatar/render-by-expression
    static crow::response renderByExpression(
        const crow::request& req,
        const std::string& token) {
        
        try {
            auto body = crow::json::load(req.body);
            std::string expression = body["expression"].s();
            
            auto result = g_renderService->renderByExpression(expression);
            
            return crow::response(200, crow::json::wvalue{
                {"status", "success"},
                {"image_base64", result.imageBase64}
            }.dump());
            
        } catch (const std::exception& e) {
            LOG_ERROR("渲染失败: {}", e.what());
            return crow::response(500, errorResponse("渲染失败"));
        }
    }
    
    // GET /api/avatar/model-info
    static crow::response getModelInfo(const crow::request& req) {
        try {
            g_renderService->logModelInfo();
            
            return crow::response(200, crow::json::wvalue{
                {"status", "success"},
                {"model", "八千代辉夜姬"},
                {"expressions", 4},
                {"parameters", 150},
                {"physics_rules", 183}
            }.dump());
            
        } catch (const std::exception& e) {
            return crow::response(500, errorResponse("获取信息失败"));
        }
    }
};

}  // namespace Controllers
```

---

### 阶段 5: Web 前端显示 (10-15 小时)

#### 5.1 创建 Live2D WebGL 组件

**新建**: `YachiyoWeb/src/components/Live2DViewer.vue`

```vue
<template>
  <div class="live2d-viewer-container">
    <canvas 
      ref="canvas" 
      id="live2d-canvas"
      width="1920"
      height="1080"
      class="live2d-canvas"
    ></canvas>
    
    <div class="controls">
      <div class="expression-buttons">
        <button @click="applyExpression('笑咪咪')">😊 开心</button>
        <button @click="applyExpression('眯眯眼')">🥰 害羞</button>
        <button @click="applyExpression('眼泪')">😢 难过</button>
        <button @click="applyExpression('泪珠')">😭 哭泣</button>
      </div>
      
      <div class="parameter-sliders">
        <div class="slider">
          <label>头部 X 轴</label>
          <input 
            type="range" 
            min="-30" 
            max="30" 
            @input="setParameter('ParamHeadAngleX', $event)"
          />
        </div>
        <div class="slider">
          <label>嘴巴开度</label>
          <input 
            type="range" 
            min="0" 
            max="1" 
            step="0.1"
            @input="setParameter('ParamMouthOpenY', $event)"
          />
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import * as PIXI from 'pixi.js'
import Live2DFramework from '@pixi/live2d-framework'

const canvas = ref<HTMLCanvasElement>()
let app: PIXI.Application
let live2dModel: any

onMounted(async () => {
  // 初始化 Pixi.js
  app = new PIXI.Application({
    view: canvas.value,
    width: 1920,
    height: 1080,
    backgroundAlpha: 0
  })
  
  // 加载 Live2D 模型
  const model = await Live2DFramework.loadModel(
    '/yachiyo_live2d/八千代辉夜姬.model3.json'
  )
  
  app.stage.addChild(model.view)
  live2dModel = model
  
  // 动画循环
  app.ticker.add(() => {
    model.update(app.ticker.deltaMS)
    model.render()
  })
})

const applyExpression = async (expressionName: string) => {
  const response = await fetch('/api/avatar/render-by-expression', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ expression: expressionName })
  })
  
  const data = await response.json()
  // 更新虚拟形象...
}

const setParameter = (paramId: string, event: Event) => {
  const value = (event.target as HTMLInputElement).value
  live2dModel.setParameter(paramId, parseFloat(value))
}
</script>

<style scoped>
.live2d-viewer-container {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.live2d-canvas {
  width: 100%;
  height: auto;
  border: 2px solid #ddd;
  border-radius: 8px;
}

.controls {
  display: flex;
  gap: 30px;
}

.expression-buttons {
  display: flex;
  gap: 10px;
}

.expression-buttons button {
  padding: 10px 20px;
  border: none;
  border-radius: 5px;
  background: #007bff;
  color: white;
  cursor: pointer;
  font-size: 14px;
}

.expression-buttons button:hover {
  background: #0056b3;
}

.parameter-sliders {
  display: flex;
  gap: 30px;
}

.slider {
  display: flex;
  align-items: center;
  gap: 10px;
}

.slider label {
  min-width: 100px;
  font-weight: bold;
}

.slider input {
  width: 200px;
}
</style>
```

#### 5.2 创建虚拟直播页面

**新建**: `YachiyoWeb/src/pages/LiveStream.vue`

```vue
<template>
  <div class="live-stream">
    <div class="video-area">
      <Live2DViewer />
    </div>
    
    <div class="chat-area">
      <div class="messages">
        <div 
          v-for="msg in messages" 
          :key="msg.id"
          class="message"
        >
          <div class="user">{{ msg.user }}</div>
          <div class="text">{{ msg.text }}</div>
          <div class="emotion" v-if="msg.emotion">
            [{{ emotionLabel(msg.emotion) }}]
          </div>
        </div>
      </div>
      
      <div class="input-area">
        <input 
          v-model="inputMessage"
          placeholder="输入消息..."
          @keyup.enter="sendMessage"
        />
        <button @click="sendMessage">发送</button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import Live2DViewer from '@/components/Live2DViewer.vue'

const messages = ref([])
const inputMessage = ref('')

const emotionLabel = (emotion: number) => {
  const labels = ['开心', '害羞', '难过', '哭泣', '中立']
  return labels[emotion] || '未知'
}

const sendMessage = async () => {
  if (!inputMessage.value) return
  
  // 发送消息到后端
  const response = await fetch('/api/messages', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      content: inputMessage.value,
      platform: 'web'
    })
  })
  
  const data = await response.json()
  
  // 触发虚拟形象反应
  await fetch('/api/avatar/render-by-message', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      message: inputMessage.value
    })
  })
  
  inputMessage.value = ''
}
</script>

<style scoped>
.live-stream {
  display: grid;
  grid-template-columns: 2fr 1fr;
  gap: 20px;
  height: 100vh;
}

.video-area {
  background: #000;
  border-radius: 8px;
  overflow: hidden;
}

.chat-area {
  display: flex;
  flex-direction: column;
  background: #f5f5f5;
  border-radius: 8px;
  padding: 20px;
}

.messages {
  flex: 1;
  overflow-y: auto;
  margin-bottom: 20px;
}

.message {
  background: white;
  padding: 12px;
  margin-bottom: 10px;
  border-radius: 5px;
  border-left: 3px solid #007bff;
}

.user {
  font-weight: bold;
  color: #007bff;
  margin-bottom: 5px;
}

.emotion {
  font-size: 12px;
  color: #999;
  margin-top: 5px;
}

.input-area {
  display: flex;
  gap: 10px;
}

.input-area input {
  flex: 1;
  padding: 10px;
  border: 1px solid #ddd;
  border-radius: 5px;
  font-size: 14px;
}

.input-area button {
  padding: 10px 20px;
  background: #007bff;
  color: white;
  border: none;
  border-radius: 5px;
  cursor: pointer;
}

.input-area button:hover {
  background: #0056b3;
}
</style>
```

---

## 📋 集成检查清单

### 代码集成

- [ ] 添加 Live2D SDK 依赖到 CMakeLists.txt
- [ ] 创建 Live2DModel 类 (加载、渲染、参数)
- [ ] 创建 EmotionAnalyzer 类 (情感识别)
- [ ] 创建 Live2DRenderService 类 (渲染服务)
- [ ] 创建 AvatarController 类 (API 端点)
- [ ] 添加 Live2D 相关路由

### API 端点

- [ ] POST /api/avatar/render-by-message
- [ ] POST /api/avatar/render-by-expression
- [ ] POST /api/avatar/generate-video
- [ ] GET /api/avatar/model-info

### 前端集成

- [ ] 安装 Pixi.js 和 Live2D 库
- [ ] 创建 Live2DViewer 组件
- [ ] 创建 LiveStream 页面
- [ ] 集成消息→表情链路
- [ ] 测试实时显示

### 测试

- [ ] 单元测试 (情感分析)
- [ ] 集成测试 (渲染服务)
- [ ] E2E 测试 (API 端点)
- [ ] 性能测试 (渲染帧率)

### 部署

- [ ] 打包 Live2D 资源
- [ ] 优化模型文件大小
- [ ] 配置 CDN 分发
- [ ] 性能监控

---

## 🚀 快速开始

### 1. 下载 Live2D SDK

```bash
# Linux
cd YachiyoCPP/third_party
wget https://github.com/Live2D/CubismNativeSamples/releases/download/3-x-x/CubismSdkForNative-3-x-x.zip
unzip CubismSdkForNative-3-x-x.zip

# macOS / Windows
# 从官网下载: https://www.live2d.com/download/cubism-sdk/
```

### 2. 编译项目

```bash
cd YachiyoCPP
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 3. 运行后端

```bash
./yachiyo_server
```

### 4. 启动前端

```bash
cd YachiyoWeb
npm install
npm run dev
```

### 5. 访问虚拟直播页面

```
http://localhost:3000/live-stream
```

---

## 📊 性能指标

| 指标 | 目标 | 实现方式 |
|------|------|--------|
| **渲染帧率** | 60 FPS | GPU 加速渲染 |
| **加载时间** | < 2s | 预加载 + 缓存 |
| **内存占用** | < 500 MB | 资源流式加载 |
| **响应时间** | < 500ms | 异步处理 |

---

## 🎓 参考文档

- [Live2D Cubism SDK 文档](https://docs.live2d.com/)
- [Pixi.js Live2D 插件](https://github.com/guansss/pixi-live2d-display)
- [VTubeStudio 模型格式](https://denchisoft.com/)

---

**完整集成预计耗时**: 40-60 小时  
**难度**: ⭐⭐⭐ (中等)  
**优先级**: 🔴 高 (核心功能)
