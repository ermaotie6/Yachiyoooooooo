# 🎨 Live2D 资源使用指南

**版本**: 1.0  
**更新日期**: 2026年4月3日  
**用途**: Yachiyo 虚拱形象直播平台的 Live2D 动画集成

---

## 📋 目录

1. [简介](#简介)
2. [资源结构](#资源结构)
3. [模型加载](#模型加载)
4. [动作命令](#动作命令)
5. [前端集成](#前端集成)
6. [后端 API](#后端api)
7. [性能优化](#性能优化)
8. [常见问题](#常见问题)

---

## 简介

### Live2D 是什么？

Live2D 是一种 2D 动画技术，用于创建看起来像 3D 但实际上是 2D 的角色动画。

**特点**:
- 🎭 轻量级 2D 模型 (100KB-1MB)
- ⚡ 实时动画合成
- 🎨 支持参数化形变 (眼睛、嘴、姿态等)
- 💰 成本低，质量高

### Yachiyo 中的 Live2D

在 Yachiyo 中，Live2D 用于：
1. **实时面部表情** - 根据 OpenClaw 情感显示表情
2. **嘴部同步** - 根据音频合成动作
3. **肢体动作** - 根据 OpenClaw 动作命令做出手势/姿态
4. **眼睛跟踪** - 跟随鼠标/用户位置

---

## 资源结构

### 📁 resources/live2d/ 目录组织

```
resources/live2d/
├── models/                              # Live2D 模型库
│   ├── yachiyou_default/
│   │   ├── model.json                   # 模型定义
│   │   ├── model.moc3                   # 编译的模型文件
│   │   ├── textures/
│   │   │   ├── texture_00.png           # 身体贴图
│   │   │   ├── texture_01.png           # 脸部贴图
│   │   │   └── texture_02.png           # 衣服贴图
│   │   ├── motions/
│   │   │   ├── idle.motion3.json        # 待机动作
│   │   │   ├── talk.motion3.json        # 说话动作
│   │   │   ├── happy.motion3.json       # 开心动作
│   │   │   ├── sad.motion3.json         # 伤心动作
│   │   │   ├── angry.motion3.json       # 生气动作
│   │   │   ├── wave.motion3.json        # 挥手动作
│   │   │   ├── nod.motion3.json         # 点头动作
│   │   │   └── shake.motion3.json       # 摇头动作
│   │   └── expressions/
│   │       ├── f_default.exp3.json      # 默认表情
│   │       ├── f_smile.exp3.json        # 微笑表情
│   │       ├── f_surprised.exp3.json    # 惊讶表情
│   │       ├── f_sad.exp3.json          # 伤心表情
│   │       └── f_angry.exp3.json        # 生气表情
│   ├── yachiyou_playful/                # 其他变体
│   │   └── (类似结构)
│   └── yachiyou_cool/
│       └── (类似结构)
├── physics/                             # 物理设置
│   ├── default_physics.json
│   └── dynamic_hair.json
├── config.json                          # 全局配置
└── README.md                            # 文档
```

### model.json 示例

```json
{
  "Version": 3,
  "FileReferences": {
    "Moc": "model.moc3",
    "Textures": [
      "textures/texture_00.png",
      "textures/texture_01.png",
      "textures/texture_02.png"
    ],
    "Physics": "../physics/default_physics.json",
    "Motions": {
      "Idle": [
        {
          "File": "motions/idle.motion3.json"
        }
      ],
      "Tap Body": [
        {
          "File": "motions/wave.motion3.json"
        }
      ],
      "Expressions": [
        {
          "Name": "f_default",
          "File": "expressions/f_default.exp3.json"
        },
        {
          "Name": "f_smile",
          "File": "expressions/f_smile.exp3.json"
        }
      ]
    }
  },
  "Groups": [
    {
      "Name": "GroupMouth",
      "Ids": ["ParamMouthOpenY"]
    },
    {
      "Name": "GroupEye",
      "Ids": ["ParamEyeLOpen", "ParamEyeROpen"]
    }
  ],
  "HitAreas": [
    {
      "Name": "HitAreaHead",
      "Id": "HitAreaHead"
    },
    {
      "Name": "HitAreaBody",
      "Id": "HitAreaBody"
    }
  ]
}
```

### 配置文件 (config.json)

```json
{
  "models": {
    "yachiyou_default": {
      "path": "models/yachiyou_default",
      "name": "Yachiyo (默认)",
      "description": "默认的 Yachiyo 虚拱形象",
      "scale": 1.0,
      "x": 0.5,
      "y": 0.5,
      "mouthSyncIntensity": 0.8,
      "eyeTrackingEnabled": true,
      "physicsSimilation": true
    },
    "yachiyou_playful": {
      "path": "models/yachiyou_playful",
      "name": "Yachiyo (调皮版)",
      "description": "更活泼的 Yachiyo 虚拱形象",
      "scale": 1.0,
      "x": 0.5,
      "y": 0.5,
      "mouthSyncIntensity": 0.9,
      "eyeTrackingEnabled": true,
      "physicsSimilation": true
    },
    "yachiyou_cool": {
      "path": "models/yachiyou_cool",
      "name": "Yachiyo (冷淡版)",
      "description": "更沉静的 Yachiyo 虚拱形象",
      "scale": 1.0,
      "x": 0.5,
      "y": 0.5,
      "mouthSyncIntensity": 0.6,
      "eyeTrackingEnabled": true,
      "physicsSimilation": true
    }
  },
  "expressions": {
    "default": "f_default",
    "happy": "f_smile",
    "surprised": "f_surprised",
    "sad": "f_sad",
    "angry": "f_angry"
  },
  "motions": {
    "idle": "motions/idle",
    "talk": "motions/talk",
    "happy": "motions/happy",
    "sad": "motions/sad",
    "angry": "motions/angry",
    "wave": "motions/wave",
    "nod": "motions/nod",
    "shake": "motions/shake"
  },
  "rendering": {
    "canvasWidth": 1920,
    "canvasHeight": 1080,
    "fps": 60,
    "antiAlias": true
  }
}
```

---

## 模型加载

### 📦 Live2D Web 集成库

**推荐使用**: Cubism Web SDK 4.0+

```html
<!-- frontend/public/index.html -->

<!-- Live2D Cubism SDK -->
<script src="https://cdn.jsdelivr.net/npm/live2d-cubismcore@4.2.0/live2dcubismcore.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/pixi.js@7.0.0/dist/pixi.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/@pixi/stats@7.0.0/dist/stats.min.js"></script>

<!-- Cubism Framework -->
<script src="https://cdn.jsdelivr.net/npm/live2d-cubism-framework@4.2.0/Core/live2dcubismframework.min.js"></script>
```

### Vue 3  组件 (前端加载)

```vue
<!-- frontend/src/components/Live2DAvatar.vue -->

<template>
  <div class="live2d-container">
    <canvas
      ref="canvas"
      id="live2d-canvas"
      class="live2d-canvas"
      @mousemove="onMouseMove"
      @click="onCanvasClick"
    ></canvas>
    
    <div v-if="isLoading" class="loading-indicator">
      加载中...
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import { Live2DManager } from '@/services/Live2DManager'
import type { Live2DModelConfig } from '@/types/live2d'

const canvas = ref<HTMLCanvasElement | null>(null)
const live2dManager = ref<Live2DManager | null>(null)
const isLoading = ref(true)
const currentModel = ref<string>('yachiyou_default')

onMounted(async () => {
  if (!canvas.value) return
  
  try {
    // 初始化 Live2D 管理器
    live2dManager.value = new Live2DManager(canvas.value)
    await live2dManager.value.initialize()
    
    // 加载默认模型
    await live2dManager.value.loadModel(currentModel.value)
    
    isLoading.value = false
    
    // 启动动画循环
    live2dManager.value.startAnimation()
    
  } catch (error) {
    console.error('Live2D 初始化失败:', error)
    isLoading.value = false
  }
})

onUnmounted(() => {
  if (live2dManager.value) {
    live2dManager.value.dispose()
  }
})

const onMouseMove = (event: MouseEvent) => {
  if (!live2dManager.value || !canvas.value) return
  
  const rect = canvas.value.getBoundingClientRect()
  const x = (event.clientX - rect.left) / rect.width
  const y = (event.clientY - rect.top) / rect.height
  
  live2dManager.value.setEyeTrackingTarget(x, y)
}

const onCanvasClick = (event: MouseEvent) => {
  if (!live2dManager.value || !canvas.value) return
  
  const rect = canvas.value.getBoundingClientRect()
  const x = (event.clientX - rect.left) / rect.width
  const y = (event.clientY - rect.top) / rect.height
  
  // 检测点击的区域
  live2dManager.value.handleCanvasClick(x, y)
}

// 暴露给外部的方法
defineExpose({
  playExpression: (name: string) => live2dManager.value?.playExpression(name),
  playMotion: (group: string, index: number) => live2dManager.value?.playMotion(group, index),
  setMouthOpening: (value: number) => live2dManager.value?.setParameter('ParamMouthOpenY', value),
  loadModel: (modelName: string) => live2dManager.value?.loadModel(modelName)
})
</script>

<style scoped>
.live2d-container {
  width: 100%;
  height: 100%;
  position: relative;
  background: transparent;
}

.live2d-canvas {
  width: 100%;
  height: 100%;
  display: block;
}

.loading-indicator {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  font-size: 18px;
  color: #666;
}
</style>
```

### Live2DManager 类实现

```typescript
// frontend/src/services/Live2DManager.ts

import { PIXI } from '@pixi/app'

export class Live2DManager {
  private app: PIXI.Application | null = null
  private model: any = null
  private animator: any = null
  private modelConfig: any = null
  private parameters: Map<string, number> = new Map()
  
  constructor(canvas: HTMLCanvasElement) {
    this.initializePixi(canvas)
  }
  
  private initializePixi(canvas: HTMLCanvasElement) {
    this.app = new PIXI.Application({
      view: canvas,
      transparent: true,
      resolution: window.devicePixelRatio,
      autoDensity: true
    })
    
    window.addEventListener('resize', () => this.onWindowResize())
  }
  
  async initialize(): Promise<void> {
    // 初始化 Cubism SDK
    if (!Live2DCubismCore.isAvailable()) {
      throw new Error('Live2D Cubism Core 不可用')
    }
  }
  
  async loadModel(modelName: string): Promise<void> {
    try {
      // 1. 加载配置
      const response = await fetch(`/resources/live2d/config.json`)
      const config = await response.json()
      
      this.modelConfig = config.models[modelName]
      if (!this.modelConfig) {
        throw new Error(`模型 ${modelName} 不存在`)
      }
      
      // 2. 加载 model.json
      const modelJsonPath = `${this.modelConfig.path}/model.json`
      const modelJsonResponse = await fetch(modelJsonPath)
      const modelJson = await modelJsonResponse.json()
      
      // 3. 使用 Cubism SDK 加载模型
      // 这部分取决于您使用的具体 SDK 版本
      // 简化示例：
      this.model = await this.loadModelData(modelJsonPath)
      
      // 4. 初始化动画器
      this.animator = new ModelAnimator(this.model)
      
      console.log(`已加载模型: ${modelName}`)
      
    } catch (error) {
      console.error('加载模型失败:', error)
      throw error
    }
  }
  
  private async loadModelData(modelJsonPath: string): Promise<any> {
    // 这是一个简化的加载过程
    // 实际实现取决于 Live2D SDK 的具体 API
    
    const modelDirectory = modelJsonPath.split('/').slice(0, -1).join('/')
    
    // 加载模型 (pseudo code)
    return {
      modelJson: await this.fetchJson(modelJsonPath),
      textures: await this.loadTextures(modelDirectory),
      physics: await this.loadPhysics(modelDirectory),
      motions: await this.loadMotions(modelDirectory),
      expressions: await this.loadExpressions(modelDirectory)
    }
  }
  
  playExpression(name: string): void {
    if (!this.model || !this.modelConfig) return
    
    const expressionId = this.modelConfig.expressions[name] || name
    // 播放表情动画
    this.animator?.playExpression(expressionId)
  }
  
  playMotion(group: string, index: number = 0): void {
    if (!this.model || !this.animator) return
    
    // 播放动作
    this.animator.playMotion(group, index, true)
  }
  
  setParameter(paramName: string, value: number): void {
    if (!this.model) return
    
    // 设置模型参数
    this.parameters.set(paramName, Math.max(0, Math.min(1, value)))
  }
  
  setEyeTrackingTarget(x: number, y: number): void {
    if (!this.model) return
    
    // 计算眼睛注视方向
    const eyeX = (x - 0.5) * 2
    const eyeY = (y - 0.5) * 2
    
    this.setParameter('ParamEyeLOpen', 1.0)
    this.setParameter('ParamEyeROpen', 1.0)
    this.setParameter('ParamEyeLX', eyeX)
    this.setParameter('ParamEyeRX', eyeX)
    this.setParameter('ParamEyeLY', eyeY)
    this.setParameter('ParamEyeRY', eyeY)
  }
  
  handleCanvasClick(x: number, y: number): void {
    if (!this.model) return
    
    // 检测点击的区域 (hitArea)
    // 如果点击在头部，播放点头动作
    // 如果点击在身体，播放挥手动作等
    
    const hitArea = this.getHitAreaAtPosition(x, y)
    
    switch (hitArea) {
      case 'HitAreaHead':
        this.playMotion('Tap Head', 0)
        break
      case 'HitAreaBody':
        this.playMotion('Tap Body', 0)
        break
    }
  }
  
  startAnimation(): void {
    if (!this.app) return
    
    this.app.ticker.add(() => {
      this.update()
      this.render()
    })
  }
  
  private update(): void {
    if (!this.model || !this.animator) return
    
    // 更新参数
    for (const [param, value] of this.parameters) {
      this.model.setParameterValueById(param, value)
    }
    
    // 更新动画器
    this.animator.update()
  }
  
  private render(): void {
    // 渲染模型到 canvas
  }
  
  dispose(): void {
    if (this.app) {
      this.app.destroy()
    }
  }
  
  private onWindowResize(): void {
    if (!this.app) return
    
    this.app.renderer.resize(
      window.innerWidth,
      window.innerHeight
    )
  }
  
  private async fetchJson(path: string): Promise<any> {
    const response = await fetch(path)
    return response.json()
  }
  
  private async loadTextures(dir: string): Promise<any[]> {
    // 加载贴图
    return []
  }
  
  private async loadPhysics(dir: string): Promise<any> {
    // 加载物理配置
    return {}
  }
  
  private async loadMotions(dir: string): Promise<any> {
    // 加载动作
    return {}
  }
  
  private async loadExpressions(dir: string): Promise<any> {
    // 加载表情
    return {}
  }
  
  private getHitAreaAtPosition(x: number, y: number): string | null {
    // 检测点击的区域
    return null
  }
}
```

---

## 动作命令

### 🎬 动作命令映射表

| OpenClaw 动作 | Live2D 动作 | 参数 | 说明 |
|--------------|-----------|------|------|
| `wave` | `Tap Body` | index: 0 | 挥手打招呼 |
| `nod` | `Nod` | - | 点头同意 |
| `shake` | `Shake` | - | 摇头拒绝 |
| `think` | `Think` | - | 思考姿态 |
| `surprised` | `Surprised` | - | 惊讶 |
| `happy_jump` | `Happy` | intensity: high | 高兴跳跃 |
| `sad_lower` | `Sad` | intensity: low | 伤心低头 |

### 🎭 表情映射表

| OpenClaw 情感 | Live2D 表情 | 参数 |
|--------------|-----------|------|
| `开心` | `f_smile` | joy: 0.8 |
| `伤心` | `f_sad` | sadness: 0.7 |
| `生气` | `f_angry` | anger: 0.9 |
| `惊讶` | `f_surprised` | surprise: 0.8 |
| `思考` | `f_default` | neutral: 0.5 |

### 动作命令 JSON

```json
{
  "request_id": "live2d_cmd_20260403_001",
  "timestamp": 1712153600000,
  "commands": [
    {
      "type": "expression",
      "value": "f_smile",
      "duration_ms": 2000,
      "blend_mode": "normal"
    },
    {
      "type": "motion",
      "group": "Tap Body",
      "index": 0,
      "priority": 1,
      "loop": false
    },
    {
      "type": "parameter",
      "name": "ParamMouthOpenY",
      "value": 0.8,
      "transition_ms": 100
    },
    {
      "type": "sequence",
      "commands": [
        {
          "type": "motion",
          "group": "Talk",
          "index": 0
        },
        {
          "type": "motion",
          "group": "Idle",
          "index": 0
        }
      ]
    }
  ]
}
```

---

## 前端集成

### 接收来自后端的动作命令

```typescript
// frontend/src/services/avatarResponseHandler.ts

import { Live2DManager } from './Live2DManager'

export class AvatarResponseHandler {
  constructor(private live2dManager: Live2DManager) {}
  
  async handleAvatarResponse(response: AvatarResponse): Promise<void> {
    // 1. 设置表情
    if (response.emotions && response.emotions.length > 0) {
      const expression = this.emotionToExpression(response.emotions[0])
      this.live2dManager.playExpression(expression)
    }
    
    // 2. 执行动作序列
    if (response.actions && response.actions.length > 0) {
      for (const action of response.actions) {
        await this.executeAction(action)
      }
    }
    
    // 3. 嘴部同步 (如果有音频)
    if (response.audio_url) {
      await this.syncMouthToAudio(response.audio_url)
    }
  }
  
  private async executeAction(action: any): Promise<void> {
    if (action.type === 'motion') {
      this.live2dManager.playMotion(action.group, action.index)
      
      // 等待动作完成
      if (!action.loop) {
        await new Promise(resolve => 
          setTimeout(resolve, action.duration_ms || 1000)
        )
      }
    }
  }
  
  private emotionToExpression(emotion: string): string {
    const mapping: Record<string, string> = {
      '开心': 'f_smile',
      '伤心': 'f_sad',
      '生气': 'f_angry',
      '惊讶': 'f_surprised'
    }
    return mapping[emotion] || 'f_default'
  }
  
  private async syncMouthToAudio(audioUrl: string): Promise<void> {
    // 使用 Web Audio API 分析音频并同步嘴部
    const audioContext = new (window.AudioContext || (window as any).webkitAudioContext)()
    // ... 音频分析逻辑
  }
}
```

---

## 后端 API

### C++ 后端 Live2D 服务

```cpp
// backend/include/services/Live2DAnimationService.hpp

#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;

class Live2DAnimationService {
public:
    struct AnimationCommand {
        std::string type;  // "expression", "motion", "parameter", "sequence"
        json params;
    };
    
    /**
     * 从 OpenClaw 响应生成 Live2D 命令
     */
    std::vector<AnimationCommand> generateCommandsFromOpenClaw(
        const OpenClawResponse& openClawResponse
    ) {
        std::vector<AnimationCommand> commands;
        
        // 1. 表情命令
        if (!openClawResponse.emotions.empty()) {
            auto emotion = openClawResponse.emotions[0];
            std::string expression = emotionToExpression(emotion);
            
            commands.push_back({
                .type = "expression",
                .params = json{
                    {"value", expression},
                    {"duration_ms", 2000}
                }
            });
        }
        
        // 2. 动作命令
        if (!openClawResponse.actions.empty()) {
            for (const auto& action : openClawResponse.actions) {
                auto motionGroup = actionToMotion(action);
                
                commands.push_back({
                    .type = "motion",
                    .params = json{
                        {"group", motionGroup},
                        {"index", 0},
                        {"priority", 1}
                    }
                });
            }
        }
        
        return commands;
    }
    
    /**
     * 将情感映射到表情
     */
    std::string emotionToExpression(const std::string& emotion) {
        static const std::unordered_map<std::string, std::string> mapping{
            {"开心", "f_smile"},
            {"伤心", "f_sad"},
            {"生气", "f_angry"},
            {"惊讶", "f_surprised"},
            {"思考", "f_default"}
        };
        
        auto it = mapping.find(emotion);
        return it != mapping.end() ? it->second : "f_default";
    }
    
    /**
     * 将动作映射到 Live2D 动作
     */
    std::string actionToMotion(const std::string& action) {
        static const std::unordered_map<std::string, std::string> mapping{
            {"wave", "Tap Body"},
            {"nod", "Nod"},
            {"shake", "Shake"},
            {"think", "Think"},
            {"surprised", "Surprised"},
            {"happy_jump", "Happy"},
            {"sad_lower", "Sad"}
        };
        
        auto it = mapping.find(action);
        return it != mapping.end() ? it->second : "Idle";
    }
};
```

### 在虚拱响应中使用

```cpp
// src/services/AvatarResponseService.cpp

void AvatarResponseService::generateAvatarResponse(
    const ChatMessage& userMessage,
    const std::string& userId
) {
    // ... (前面的代码)
    
    // 1. 调用 OpenClaw
    auto openClawResponse = openClawService->process(userMessage.content);
    
    // 2. 生成 Live2D 命令
    auto live2dCommands = live2dService->generateCommandsFromOpenClaw(openClawResponse);
    
    // 3. 组织最终响应
    AvatarResponse response{
        .responseId = generateResponseId(),
        .text = openClawResponse.text,
        .emotions = openClawResponse.emotions,
        .actions = openClawResponse.actions,
        .live2d_commands = live2dCommands,
        .audio_url = synthesisResult.audioUrl,
        .duration_ms = synthesisResult.durationMs
    };
    
    // 4. 发送给前端
    broadcastToClient(userId, response);
}
```

---

## 性能优化

### 资源预加载

```typescript
// 预加载所有模型
const preloadModels = async (live2dManager: Live2DManager) => {
  const modelNames = ['yachiyou_default', 'yachiyou_playful', 'yachiyou_cool']
  
  for (const modelName of modelNames) {
    try {
      await live2dManager.loadModel(modelName)
      console.log(`预加载完成: ${modelName}`)
    } catch (error) {
      console.error(`预加载失败: ${modelName}`, error)
    }
  }
}
```

### 动画缓存

```cpp
// 缓存已播放的动作
std::unordered_map<std::string, std::vector<AnimationCommand>> animationCache;

auto key = emotion + "_" + action;
if (animationCache.find(key) != animationCache.end()) {
  // 使用缓存的动画
  return animationCache[key];
}

// 否则生成新的动画
auto commands = generateCommandsFromOpenClaw(response);
animationCache[key] = commands;
```

---

## 常见问题

### Q: 如何添加新的 Live2D 模型?
**A**: 
1. 将模型文件放在 `resources/live2d/models/` 中
2. 在 `config.json` 中添加模型配置
3. 确保包含所需的表情和动作文件

### Q: 嘴部同步不准确怎么办?
**A**: 调整 `config.json` 中的 `mouthSyncIntensity` 参数 (0.0-1.0)

### Q: 支持实时摄像头捕捉吗?
**A**: 支持通过 Web API 获取摄像头数据，进行面部识别和表情映射

### Q: 可以加载自定义模型吗?
**A**: 可以，支持任何符合 Live2D Cubism 格式的模型

---

**参考链接**: https://www.live2d.com/en/  
**SDK 文档**: https://docs.live2d.com/cubism-sdk-tutorials/index/  
**更新日期**: 2026年4月3日
