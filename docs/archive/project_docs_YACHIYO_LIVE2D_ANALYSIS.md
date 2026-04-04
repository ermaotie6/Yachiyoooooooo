# 🎨 八千代辉夜姬 Live2D 形象分析

> **项目**: Yachiyo 虚拟主播系统  
> **角色**: 八千代辉夜姬 (Yachiyo Kaguya Hime)  
> **形象类型**: Live2D 3.0 实时模型  
> **创建日期**: 2026年2月19日  
> **最后保存**: VTubeStudio 1.32.67  
> **分析日期**: 2026年4月2日

---

## 📊 文件结构总览

```
yachiyo_live2d/
├── 八千代辉夜姬.model3.json          ← 【核心】模型定义文件
├── 八千代辉夜姬.moc3                 ← 【核心】编译的模型文件
├── 八千代辉夜姬.cdi3.json            ← 【核心】参数定义
├── 八千代辉夜姬.physics3.json        ← 【核心】物理引擎配置
├── 八千代辉夜姬.vtube.json           ← VTubeStudio 配置
├── 八千代辉夜姬.xyplugin.json        ← 扩展插件配置
├── 八千代辉夜姬.8192/                ← 【核心】纹理资源目录
│   ├── texture_00.png               ← 高分辨率纹理 (8192x8192)
│   └── texture_01.png               ← 高分辨率纹理 (8192x8192)
├── 八千代辉夜姬头像1.png             ← 模型头像缩略图
├── 笑咪咪.exp3.json                  ← 表情: 开心
├── 眯眯眼.exp3.json                  ← 表情: 迷蒙
├── 眼泪.exp3.json                    ← 表情: 悲伤
├── 泪珠.exp3.json                    ← 表情: 哭泣
└── items_pinned_to_model.json        ← 模型配件管理
```

**文件总数**: 14 个文件  
**总体积**: ~200MB+ (高分辨率 8K 纹理)  
**技术标准**: Live2D Cubism 3.0

---

## 🎯 核心文件详解

### 1️⃣ 模型主文件 - `八千代辉夜姬.model3.json`

**作用**: Live2D 模型的总配置文件，告诉引擎如何加载和渲染模型

**关键配置**:
```json
{
  "Version": 3,                    // Live2D Cubism 3.0 标准
  "FileReferences": {
    "Moc": "八千代辉夜姬.moc3",      // 编译的模型文件
    "Textures": [                  // 高分辨率纹理
      "八千代辉夜姬.8192/texture_00.png",  // 纹理 00
      "八千代辉夜姬.8192/texture_01.png"   // 纹理 01
    ],
    "Physics": "八千代辉夜姬.physics3.json",      // 物理配置
    "DisplayInfo": "八千代辉夜姬.cdi3.json"      // 显示参数
  },
  "Groups": [
    {
      "Target": "Parameter",
      "Name": "EyeBlink",           // 眨眼组
      "Ids": ["ParamEyeLOpen", "ParamEyeROpen"]
    },
    {
      "Target": "Parameter",
      "Name": "LipSync",            // 口型同步组(暂无)
      "Ids": []
    }
  ]
}
```

**用途**:
- ✅ 模型初始化的入口点
- ✅ 资源文件的索引
- ✅ 参数组的定义

---

### 2️⃣ 参数定义 - `八千代辉夜姬.cdi3.json`

**作用**: 定义模型可控制的所有参数 (4,695 行)

**参数分类**:

| 参数类型 | 示例 | 数量 | 用途 |
|--------|------|------|------|
| **头部运动** | ParamHeadAngleX/Y/Z | 3+ | 头部旋转角度 |
| **眼睛** | ParamEyeLOpen, ParamEyeROpen | 5+ | 左右眼睛开合度 |
| **眉毛** | ParamBrowLY, ParamBrowRY | 4+ | 眉毛上下运动 |
| **嘴巴** | ParamMouthOpenY, ParamMouthSmile | 6+ | 嘴巴开度和表情 |
| **头发物理** | ParamShairPhysics_L1-7, R1-7 | 14 | 左右侧发摇晃 |
| **身体运动** | ParamBodyAngleX/Y/Z | 3+ | 身体倾斜 |
| **胸部物理** | ParamChest_Z | 1+ | 胸部自然摇晃 |
| **表情切换** | ParamExpression_1-4 | 4 | 预设表情组 |
| **其他** | ParamHide_Eyes, ParamHighLight | 10+ | 隐藏/显示部件 |

**示例参数**:
```json
{
  "Id": "ParamHeadAngleX",        // 参数 ID
  "GroupId": "ParamHeadAngle",    // 所属组
  "Name": "头部角度X",             // 显示名称
  "Min": -30.0,                   // 最小值
  "Max": 30.0,                    // 最大值
  "DefaultValue": 0.0             // 默认值
}
```

**参数总数**: ~150+ 个可控制参数

---

### 3️⃣ 物理引擎 - `八千代辉夜姬.physics3.json`

**作用**: 定义哪些部件受物理影响 (22,948 行 - 最大的配置文件!)

**物理配置**:

| 配置项 | 值 | 说明 |
|--------|-----|------|
| **PhysicsSettingCount** | 183 | 物理规则总数 |
| **TotalInputCount** | 449 | 输入参数数 |
| **TotalOutputCount** | 578 | 输出参数数 |
| **VertexCount** | 795 | 网格顶点数 |
| **Fps** | 60 | 物理计算帧率 |
| **Gravity.Y** | -1 | 重力 (向下) |
| **Wind** | (0,0) | 风力 (无风) |

**物理部件示例**:
```
PhysicsSetting1: OX (头部X轴)
PhysicsSetting2: OY (头部Y轴)  
PhysicsSetting3: OZ (头部Z轴)
PhysicsSetting5-7: O_Body_X/Y/Z (身体轴)
PhysicsSetting8+: O_Chest_Z (胸部Z轴)
...
共 183 个物理规则
```

**主要物理对象**:
- 🎀 **头发**: 8 个层级的发丝物理
- 👚 **衣服**: 不同部分的布料摇晃
- 💎 **装饰品**: 发夹、项链等物理效果
- 📿 **配件**: 自然摆动效果

---

### 4️⃣ VTubeStudio 配置 - `八千代辉夜姬.vtube.json`

**作用**: VTubeStudio 软件的模型设置 (1,034 行)

**模型信息**:
```json
{
  "Version": 1,
  "Name": "【雪熊企划】八千代辉夜姬",
  "ModelID": "17616156f74a469a9372bce87bd2525d",  // 唯一标识
  "FileReferences": {
    "Icon": "八千代辉夜姬头像1.png",
    "Model": "八千代辉夜姬.model3.json"
  }
}
```

**模型位置 (SavedModelPosition)**:
```json
{
  "Position": {
    "x": -3.944,      // 水平位置 (偏左)
    "y": -69.423,     // 竖直位置 (偏下)
    "z": 0.0
  },
  "Scale": {
    "x": 2.876,       // 水平缩放 (放大 2.876 倍)
    "y": 2.876,       // 竖直缩放
    "z": 1.0
  }
}
```

**模型运动 (ModelPositionMovement)**:
```json
{
  "Use": true,          // 启用位置运动
  "X": 6, "Y": 8,      // 运动幅度
  "SmoothingX": 10,    // 平滑度 (1-20)
  "SmoothingY": 10
}
```

**最后保存时间**: 2026年2月19日 23:05:26

---

### 5️⃣ 表情文件 - `*.exp3.json`

**作用**: 预设表情组，快速切换表情

**表情列表**:
| 文件名 | 表情名 | 用途 |
|--------|--------|------|
| 笑咪咪.exp3.json | 开心表情 | 互动、回应粉丝 |
| 眯眯眼.exp3.json | 迷蒙表情 | 撒娇、卖萌 |
| 眼泪.exp3.json | 悲伤表情 | 感动、难过 |
| 泪珠.exp3.json | 哭泣表情 | 极度悲伤 |

**表情实现方式**:
```json
{
  "Type": "Live2D Expression",
  "Parameters": [
    {
      "Id": "ParamExpression_3",
      "Value": 1.0,        // 参数值 (0.0-1.0)
      "Blend": "Add"       // 混合模式
    },
    {
      "Id": "ParamHide_EyesL1",
      "Value": 1.0,        // 隐藏左眼
      "Blend": "Add"
    },
    // 更多参数...
  ]
}
```

**表情机制**:
- 通过调整多个参数的值来组合表情
- 每个表情包含多个参数变化
- 使用 Blend 模式 (Add/Multiply/Overwrite) 混合叠加

---

### 6️⃣ 纹理资源 - `八千代辉夜姬.8192/`

**作用**: 高分辨率 Live2D 纹理贴图

**纹理信息**:

| 文件 | 分辨率 | 用途 |
|------|--------|------|
| texture_00.png | 8192×8192 | 主纹理 (皮肤、衣服等) |
| texture_01.png | 8192×8192 | 辅助纹理 (细节、装饰) |

**特点**:
- 🎨 **8K 超高分辨率**: 保证高质量显示
- 📐 **分层设计**: 分为 00 和 01 两层，方便管理
- 🎭 **Live2D 格式**: PNG 格式，包含透明通道
- 💾 **总体积**: ~150MB (两张图合计)

---

### 7️⃣ 其他配置文件

#### `八千代辉夜姬.xyplugin.json`
- VTubeStudio 扩展插件配置
- 用于添加自定义功能或第三方集成

#### `items_pinned_to_model.json`
- 模型配件管理
- 定义附加到模型上的物品位置

#### `八千代辉夜姬头像1.png`
- 模型缩略图头像
- 用于 UI 显示和选择

---

## 🔌 集成方式分析

### Live2D 渲染流程

```
┌─────────────────────────────────────────────────┐
│        虚拟主播系统 / OpenClaw 代理              │
│     (需要实时驱动模型和表情)                    │
└────────────────┬────────────────────────────────┘
                 │ 输入参数 (头部角度、眼睛、嘴巴等)
                 ▼
┌─────────────────────────────────────────────────┐
│   八千代辉夜姬.model3.json                      │
│   (模型主配置 - 告诉引擎如何加载)               │
└────────────────┬────────────────────────────────┘
                 │ 加载资源
                 ▼
    ┌────────────┼────────────┐
    │            │            │
    ▼            ▼            ▼
┌──────────┐ ┌──────────┐ ┌──────────────┐
│ MOC3     │ │ 纹理     │ │ 物理配置     │
│ 编译文件 │ │ 8K PNG   │ │ JSON         │
└──────────┘ └──────────┘ └──────────────┘
    │            │            │
    └────────────┼────────────┘
                 │
                 ▼
    ┌────────────────────────────┐
    │  CDI3 参数定义             │
    │  (150+ 可控制参数)         │
    └────────────────────────────┘
                 │
                 ▼
    ┌────────────────────────────┐
    │  Live2D 渲染引擎           │
    │  (计算网格变形和物理)      │
    └────────────────────────────┘
                 │
                 ▼
    ┌────────────────────────────┐
    │  最终渲染输出              │
    │  (实时 3D 动画)            │
    └────────────────────────────┘
```

### 参数驱动机制

```
OpenClaw / 虚拟主播系统
    │
    ├─► 运动捕捉数据 → 头部角度 (ParamHeadAngleX/Y/Z)
    │
    ├─► 音声分析 → 口型参数 (ParamMouthOpenY)
    │
    ├─► 表情命令 → 预设表情 (exp3.json)
    │
    ├─► 眨眼周期 → 眼睛开合 (ParamEyeLOpen)
    │
    └─► 物理模拟 → 头发/衣服摇晃 (自动计算)
```

---

## 🎬 使用场景

### 1. 直播互动

**流程**:
```
粉丝发起互动
    ↓
OpenClaw 代理处理
    ↓
驱动 Live2D 表情变化 (用 exp3.json)
    ↓
播放对应的虚拟主播反应
    ↓
粉丝看到实时虚拟形象响应
```

**所需参数**:
- 眼睛表情 (ParamEyeLOpen, ParamEyeROpen)
- 嘴巴开度 (ParamMouthOpenY)
- 眉毛位置 (ParamBrowLY, ParamBrowRY)

### 2. 定时公告

**流程**:
```
OpenClaw 执行定时任务
    ↓
播放 "开心" 表情 (笑咪咪.exp3.json)
    ↓
合成语音 + 虚拟主播形象
    ↓
在多个平台发布公告
```

### 3. 情感表达

**情感 → 表情映射**:

| 情感 | 表情文件 | 参数变化 |
|------|--------|---------|
| 😊 开心 | 笑咪咪.exp3.json | 嘴巴上扬、眼睛眯细 |
| 🥰 害羞 | 眯眯眼.exp3.json | 眼睛眯细、脸部泛红 |
| 😢 难过 | 眼泪.exp3.json | 眉毛下压、嘴巴下扬 |
| 😭 哭泣 | 泪珠.exp3.json | 完全闭眼、泪滴效果 |

### 4. 自然物理效果

**不需要驱动，自动发生**:
- 💫 头发随身体运动摇晃 (183 个物理规则)
- 👗 衣服布料自然摆动
- 📿 配件随之晃动
- ❄️ 受重力影响 (Gravity: Y = -1)

---

## 🛠️ 技术参数汇总

### 模型规格

| 指标 | 值 | 说明 |
|------|-----|------|
| **格式标准** | Live2D Cubism 3.0 | 业界标准 |
| **纹理分辨率** | 8192×8192 | 超高清 |
| **纹理数量** | 2 张 | texture_00, texture_01 |
| **可控参数数** | 150+ | 高度可定制 |
| **物理规则数** | 183 | 逼真物理效果 |
| **网格顶点数** | 795 | 高精度变形 |
| **预设表情数** | 4 | 基础表情 |
| **物理 FPS** | 60 | 流畅度 |

### 文件尺寸

| 文件 | 大小 | 说明 |
|------|------|------|
| model3.json | ~2 KB | 配置 |
| cdi3.json | ~200 KB | 参数定义 |
| physics3.json | ~1.5 MB | 物理配置 |
| moc3 | ~2 MB | 编译模型 |
| texture_00.png | ~75 MB | 主纹理 |
| texture_01.png | ~75 MB | 辅助纹理 |
| **总计** | **~150 MB** | 整体体积 |

### 系统要求

为了顺畅运行此模型：

| 需求 | 最低 | 推荐 |
|------|------|------|
| **GPU VRAM** | 2 GB | 4 GB+ |
| **CPU** | 2 核 @ 2.5GHz | 4 核 @ 3.5GHz+ |
| **内存 RAM** | 4 GB | 8 GB+ |
| **显示帧率** | 30 FPS | 60 FPS+ |
| **分辨率** | 720p | 1080p+ |

---

## 🔗 与 Yachiyo 系统的集成点

### 1. OpenClaw 代理驱动

```cpp
// 伪代码：OpenClaw 代理驱动虚拟形象
class YachiyoVirtualAvatar {
    Live2DModel model;  // 加载 model3.json
    
    void reactToMessage(string message) {
        // 分析消息情感
        Emotion emotion = analyzeEmotion(message);
        
        // 应用对应表情
        switch(emotion) {
            case HAPPY:
                model.applyExpression("笑咪咪.exp3.json");
                break;
            case SAD:
                model.applyExpression("眼泪.exp3.json");
                break;
            // ...
        }
        
        // 播放动画
        model.animate(duration: 2.0);
    }
};
```

### 2. 实时参数驱动

```cpp
// 根据 API 数据驱动模型
void updateModelParameters(MessageData msg) {
    // 参数来自 cdi3.json 的定义
    model.setParameter("ParamMouthOpenY", msg.sentiment);
    model.setParameter("ParamEyeLOpen", msg.intensity);
    model.setParameter("ParamBrowLY", msg.expression_micro);
    
    // 物理引擎自动计算 (physics3.json)
    model.updatePhysics(deltaTime);
    
    // 渲染输出
    model.render();
}
```

### 3. 前端显示

```javascript
// Web 前端显示虚拟形象
class YachiyoLive2DViewer {
    constructor() {
        // 加载模型
        this.model = Live2DFramework.loadModel(
            "yachiyo_live2d/八千代辉夜姬.model3.json"
        );
    }
    
    playExpression(expressionName) {
        const expression = `yachiyo_live2d/${expressionName}.exp3.json`;
        this.model.setExpression(expression);
    }
    
    animate(parameters) {
        // 持续更新参数
        for (let param of parameters) {
            this.model.setParameter(param.name, param.value);
        }
    }
}
```

---

## 📱 平台适配

### 支持的平台

| 平台 | 实现方式 | 适配情况 |
|------|--------|--------|
| **Web/浏览器** | Pixi.js / WebGL | ✅ 支持 (需要 WebGL) |
| **Windows/Mac** | VTubeStudio | ✅ 完全支持 |
| **iPhone/iPad** | Unity / Native | ✅ 可移植 |
| **Android** | Unity / Native | ✅ 可移植 |
| **Discord Bot** | 动态图片生成 | ⚠️ 需要额外处理 |
| **Twitch 直播** | OBS 集成 | ✅ 完全支持 |

### 动态图生成 (用于 OpenClaw)

```
Live2D 模型 (实时)
    ↓
逐帧渲染
    ↓
生成 WebP/MP4 序列
    ↓
发送到 Discord/Telegram
    ↓
粉丝看到虚拟主播视频
```

---

## 🎯 下一步建议

### 1. 在 Yachiyo 后端集成 Live2D 渲染

**需要添加**:
```cpp
// 新建文件: include/models/Live2DModel.hpp
class Live2DModel {
public:
    bool loadModel(const std::string& modelPath);
    void setParameter(const std::string& paramId, float value);
    void applyExpression(const std::string& expressionFile);
    void updatePhysics(float deltaTime);
    Image render();  // 渲染为图片
    Video renderToVideo(const std::vector<Frame>& frames);
};
```

### 2. OpenClaw 任务定义扩展

**在 OPENCLAW_VIRTUAL_AVATAR_TASKS.md 中添加**:
```yaml
Task: Avatar Animation
  Trigger: "message received"
  Actions:
    - analyze emotion
    - select expression from:
        - happy: 笑咪咪.exp3.json
        - sad: 眼泪.exp3.json
    - render live2d model
    - send image/video
```

### 3. API 端点添加

```
POST /api/avatar/animate
{
  "expression": "笑咪咪",
  "parameters": {
    "ParamMouthOpenY": 0.8,
    "ParamEyeLOpen": 1.0
  },
  "duration": 2.0
}

Response: { "video_url": "..." }
```

### 4. 前端展示组件

```
前端需要集成:
- Live2D WebGL 渲染库
- 表情切换 UI
- 实时参数调节滑块
- 录制/截图功能
```

---

## 📚 相关资源

### Live2D 官方资源

- 📖 [Live2D Cubism SDK 文档](https://docs.live2d.com/)
- 🎓 [Live2D 开发教程](https://www.live2d.com/en/download/sample-models/)
- 🔧 [Cubism 编辑器下载](https://www.live2d.com/en/download/cubism)

### VTubeStudio 资源

- 🎮 [VTubeStudio 官方网站](https://denchisoft.com/)
- 📚 [VTubeStudio 插件 API](https://github.com/DenchiSoft/VTubeStudio)

### 参考项目

- [Live2D Web Framework](https://github.com/Live2D/CubismWebFramework)
- [Pixi.js Live2D Plugin](https://github.com/guansss/pixi-live2d-display)

---

## ✅ 快速参考

### 表情切换代码示例

```python
# Python 示例
import json

# 加载表情文件
with open('yachiyo_live2d/笑咪咪.exp3.json', 'r') as f:
    expression = json.load(f)

# 获取所有参数
for param in expression['Parameters']:
    param_id = param['Id']
    param_value = param['Value']
    print(f"{param_id}: {param_value}")
```

### 参数查询

```bash
# 查看所有可用参数
grep -o '"Id": "[^"]*"' yachiyo_live2d/八千代辉夜姬.cdi3.json | sort -u

# 查看物理规则数
grep "PhysicsSettingCount" yachiyo_live2d/八千代辉夜姬.physics3.json

# 查看表情定义
cat yachiyo_live2d/笑咪咪.exp3.json | jq '.Parameters[] | .Id' | sort -u
```

---

## 🎬 总结

**八千代辉夜姬 Live2D 形象** 是一个**高质量、完整的虚拟主播模型**，具备：

✅ **高分辨率**: 8K 纹理 (8192×8192)  
✅ **精细控制**: 150+ 可调参数  
✅ **逼真物理**: 183 个物理规则  
✅ **丰富表情**: 4 个预设表情  
✅ **完整配置**: 所有必需的 Live2D 文件  

**可直接用于**:
- 🎮 虚拟直播系统
- 🤖 OpenClaw 自主代理
- 💬 社交媒体互动
- 📱 多平台展示

现在你的 Yachiyo 虚拟主播系统已经有了完整的视觉形象！🎉

---

**文档版本**: 1.0  
**最后更新**: 2026年4月2日  
**创建者**: Yachiyo AI Assistant
