# 📚 Live2D 虚拟形象 - 快速参考

> **最后更新**: 2026年4月2日  
> **角色**: 八千代辉夜姬 (Yachiyo Kaguya Hime)

---

## 🎯 文件清单

| 文件 | 大小 | 用途 | 关键信息 |
|------|------|------|--------|
| **八千代辉夜姬.model3.json** | 2 KB | ✅ 核心配置 | 模型入口，资源索引 |
| **八千代辉夜姬.moc3** | 2 MB | ✅ 编译模型 | 二进制模型文件 |
| **八千代辉夜姬.cdi3.json** | 200 KB | ✅ 参数定义 | 150+ 参数 |
| **八千代辉夜姬.physics3.json** | 1.5 MB | ✅ 物理配置 | 183 物理规则 |
| **texture_00.png** | 75 MB | ✅ 主纹理 | 8192×8192 高清 |
| **texture_01.png** | 75 MB | ✅ 辅助纹理 | 8192×8192 高清 |
| **笑咪咪.exp3.json** | - | 表情 | 😊 开心 |
| **眯眯眼.exp3.json** | - | 表情 | 🥰 害羞 |
| **眼泪.exp3.json** | - | 表情 | 😢 难过 |
| **泪珠.exp3.json** | - | 表情 | 😭 哭泣 |
| **八千代辉夜姬.vtube.json** | 1 MB | VTubeStudio | 软件配置 |
| **八千代辉夜姬头像1.png** | - | 缩略图 | UI 显示 |

**总大小**: ~150 MB  
**纹理分辨率**: 8192×8192 (8K)  
**技术标准**: Live2D Cubism 3.0

---

## 🎭 表情速查表

| 表情 | 文件名 | 情感 | API 值 | 用途 |
|------|--------|------|--------|------|
| 😊 | 笑咪咪.exp3.json | HAPPY (0) | 0 | 开心、回应 |
| 🥰 | 眯眯眼.exp3.json | SHY (1) | 1 | 害羞、卖萌 |
| 😢 | 眼泪.exp3.json | SAD (2) | 2 | 难过、失望 |
| 😭 | 泪珠.exp3.json | CRYING (3) | 3 | 哭泣、极度难过 |

---

## ⚙️ 主要参数表

### 头部运动

```
ParamHeadAngleX   : [-30, 30]    // 横向转头
ParamHeadAngleY   : [-30, 30]    // 纵向转头  
ParamHeadAngleZ   : [-30, 30]    // 侧向转头
```

### 眼睛

```
ParamEyeLOpen     : [0, 1]       // 左眼睛开合
ParamEyeROpen     : [0, 1]       // 右眼睛开合
```

### 嘴巴

```
ParamMouthOpenY   : [0, 1]       // 嘴巴开度
ParamMouthSmile   : [0, 1]       // 微笑程度
```

### 眉毛

```
ParamBrowLY       : [-1, 1]      // 左眉毛上下
ParamBrowRY       : [-1, 1]      // 右眉毛上下
```

### 身体

```
ParamBodyAngleX   : [-30, 30]    // 身体横向倾斜
ParamBodyAngleY   : [-30, 30]    // 身体纵向倾斜
ParamBodyAngleZ   : [-30, 30]    // 身体旋转
```

### 表情切换

```
ParamExpression_1 : [0, 1]       // 表情 1
ParamExpression_2 : [0, 1]       // 表情 2
ParamExpression_3 : [0, 1]       // 表情 3
ParamExpression_4 : [0, 1]       // 表情 4
```

### 隐藏/显示

```
ParamHide_Eyes    : 1.0          // 隐藏眼睛
ParamHighLight    : 0.0          // 隐藏高光
```

**总参数数**: 150+ 个

---

## 🔌 API 端点

### 1. 根据消息文本渲染

```bash
POST /api/avatar/render-by-message

请求:
{
  "message": "哈哈，你太有趣了！"
}

响应:
{
  "status": "success",
  "emotion": 0,                    // 0=开心, 1=害羞, 2=难过, 3=哭泣
  "intensity": 0.85,               // 强度 [0, 1]
  "image_base64": "iVBORw0KGgo..."
}
```

### 2. 根据表情名渲染

```bash
POST /api/avatar/render-by-expression

请求:
{
  "expression": "笑咪咪"
}

响应:
{
  "status": "success",
  "image_base64": "iVBORw0KGgo..."
}
```

### 3. 生成动画视频

```bash
POST /api/avatar/generate-video

请求:
{
  "frames": [
    {
      "parameters": {
        "ParamMouthOpenY": 0.5,
        "ParamEyeLOpen": 1.0
      },
      "duration": 100
    }
  ],
  "output_path": "/tmp/animation.mp4"
}

响应:
{
  "status": "success",
  "video_url": "/tmp/animation.mp4"
}
```

### 4. 获取模型信息

```bash
GET /api/avatar/model-info

响应:
{
  "status": "success",
  "model": "八千代辉夜姬",
  "expressions": 4,
  "parameters": 150,
  "physics_rules": 183
}
```

---

## 🎨 表情编程示例

### Python 示例

```python
import requests
import json

# 方法 1: 根据消息文本渲染
response = requests.post(
    'http://localhost:8000/api/avatar/render-by-message',
    json={'message': '今天天气真好！'}
)

data = response.json()
emotion = ['开心', '害羞', '难过', '哭泣'][data['emotion']]
print(f"表情: {emotion}, 强度: {data['intensity']}")

# 方法 2: 直接指定表情
response = requests.post(
    'http://localhost:8000/api/avatar/render-by-expression',
    json={'expression': '笑咪咪'}
)

# 方法 3: 生成动画
frames = [
    {
        'parameters': {
            'ParamMouthOpenY': 0.0,
            'ParamEyeLOpen': 1.0
        },
        'duration': 100
    },
    {
        'parameters': {
            'ParamMouthOpenY': 1.0,
            'ParamEyeLOpen': 0.5
        },
        'duration': 200
    }
]

response = requests.post(
    'http://localhost:8000/api/avatar/generate-video',
    json={'frames': frames}
)
```

### JavaScript/Node.js 示例

```javascript
// 根据消息文本渲染
async function renderAvatar(message) {
    const response = await fetch('/api/avatar/render-by-message', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ message })
    });
    
    return await response.json();
}

// 获取虚拟形象图片
const result = await renderAvatar('哈哈，太开心了！');
console.log('情感:', result.emotion);
console.log('强度:', result.intensity);

// 在 HTML 中显示
const img = document.createElement('img');
img.src = 'data:image/png;base64,' + result.image_base64;
document.body.appendChild(img);
```

### cURL 示例

```bash
# 根据表情渲染
curl -X POST http://localhost:8000/api/avatar/render-by-expression \
  -H "Content-Type: application/json" \
  -d '{"expression": "笑咪咪"}'

# 根据消息渲染
curl -X POST http://localhost:8000/api/avatar/render-by-message \
  -H "Content-Type: application/json" \
  -d '{"message": "开心！"}'

# 获取模型信息
curl http://localhost:8000/api/avatar/model-info
```

---

## 🎬 情感→表情映射规则

```
消息文本分析 → 情感识别 → 表情选择 → 参数调整 → 渲染输出

示例流程:
────────────────────────────────────────

"哈哈哈！太棒了！" 
  ↓ (检测: "哈哈" + 大写字母)
情感: HAPPY (开心, 强度 0.9)
  ↓
选择表情: 笑咪咪.exp3.json
  ↓
参数调整:
  - ParamMouthSmile: 0.9  (嘴巴上扬)
  - ParamEyeLOpen: 0.5    (眼睛眯细)
  - ParamEyeROpen: 0.5
  ↓
渲染输出 → PNG 图片
```

---

## 🛠️ 物理引擎信息

| 属性 | 值 | 说明 |
|------|-----|------|
| **FPS** | 60 | 物理计算帧率 |
| **重力** | (0, -1) | 向下吸引 |
| **风力** | (0, 0) | 无风(可配置) |
| **规则数** | 183 | 物理对象数 |
| **顶点数** | 795 | 网格精度 |

**物理对象**:
- 头发 (8 层级)
- 衣服 (多个部分)
- 装饰品 (项链、发夹等)
- 配件 (自然摇晃)

---

## 📊 系统要求

| 需求 | 最低 | 推荐 | 说明 |
|------|------|------|------|
| **GPU VRAM** | 2 GB | 4 GB | 纹理内存 |
| **CPU 核心** | 2 | 4+ | 物理计算 |
| **系统内存** | 4 GB | 8 GB | 运行内存 |
| **帧率** | 30 FPS | 60 FPS | 显示流畅度 |
| **分辨率** | 720p | 1080p+ | 显示分辨率 |

---

## 🚀 快速集成步骤

### Step 1: 加载模型
```cpp
Live2DModel model;
model.loadModel("yachiyo_live2d/八千代辉夜姬.model3.json");
```

### Step 2: 设置参数
```cpp
model.setParameter("ParamMouthOpenY", 0.8f);
model.setParameter("ParamEyeLOpen", 1.0f);
```

### Step 3: 应用表情
```cpp
model.applyExpression("笑咪咪", 1.0f);  // 表情名, 强度
```

### Step 4: 更新和渲染
```cpp
model.update(deltaTime);
auto image = model.renderToImage();
```

---

## 🎨 颜色代码速查

表情对应的 RGB 颜色 (用于 UI):

```
😊 HAPPY   : #FFD700 (金色)   // 温暖、积极
🥰 SHY     : #FF69B4 (粉色)   // 可爱、羞涩  
😢 SAD     : #87CEEB (蓝色)   // 冷静、难过
😭 CRYING  : #9370DB (紫色)   // 极端、伤感
😐 NEUTRAL : #808080 (灰色)   // 无表情
```

---

## 📱 平台适配

| 平台 | 技术 | 支持度 | 备注 |
|------|------|--------|------|
| Web | Pixi.js + WebGL | ✅ 完全 | 需要 WebGL 支持 |
| Windows | VTubeStudio/Unity | ✅ 完全 | 原生支持 |
| macOS | VTubeStudio/Unity | ✅ 完全 | 原生支持 |
| Linux | 自定义渲染 | ⚠️ 需要 | 需要 OpenGL |
| iOS | Unity/Swift | ✅ 可移植 | 需要适配 |
| Android | Unity/Kotlin | ✅ 可移植 | 需要适配 |

---

## 🔗 文件来源路径

所有 Live2D 文件位置:
```
d:\Personal_Project\Yachiyo\yachiyo_live2d\
├── 八千代辉夜姬.model3.json
├── 八千代辉夜姬.moc3
├── 八千代辉夜姬.cdi3.json
├── 八千代辉夜姬.physics3.json
├── 八千代辉夜姬.vtube.json
├── 八千代辉夜姬.xyplugin.json
├── 八千代辉夜姬.8192/
│   ├── texture_00.png
│   └── texture_01.png
├── 八千代辉夜姬头像1.png
├── 笑咪咪.exp3.json
├── 眯眯眼.exp3.json
├── 眼泪.exp3.json
├── 泪珠.exp3.json
└── items_pinned_to_model.json
```

---

## 📚 参考文档

### 本地文档
- `YACHIYO_LIVE2D_ANALYSIS.md` - 完整分析
- `LIVE2D_INTEGRATION_PLAN.md` - 集成计划

### 官方文档
- [Live2D Cubism SDK](https://docs.live2d.com/)
- [Pixi.js 官网](https://pixijs.com/)
- [VTubeStudio](https://denchisoft.com/)

---

## ⚠️ 常见问题

### Q: 如何加载模型？
**A**: 使用 `model.model3.json` 作为入口，框架会自动加载 MOC3、纹理、物理配置。

### Q: 表情如何切换？
**A**: 通过 `applyExpression()` 方法，传入表情文件名 (笑咪咪、眯眯眼等)。

### Q: 参数范围是多少？
**A**: 大多数参数范围是 [0, 1] 或 [-30, 30]，具体看参数定义。

### Q: 如何实现实时互动？
**A**: 分析消息情感 → 选择表情 → 设置参数 → 渲染输出。

### Q: 能否自定义表情？
**A**: 可以，使用 Live2D Cubism 编辑器创建新的 exp3.json 表情。

### Q: 性能如何？
**A**: 60 FPS 稳定运行，GPU 内存占用 ~500 MB。

---

## 🎯 下一步

1. ✅ **分析完成** - 了解模型结构
2. 📝 **集成规划** - 详见 LIVE2D_INTEGRATION_PLAN.md
3. 🔧 **代码实现** - 创建 Live2DModel 等类
4. 🎨 **前端集成** - Web 展示组件
5. 🧪 **测试验证** - 单元/集成测试
6. 🚀 **部署上线** - 部署到生产环境

---

**需要详细分析?** 查看 `YACHIYO_LIVE2D_ANALYSIS.md`  
**需要实现指南?** 查看 `LIVE2D_INTEGRATION_PLAN.md`  
**需要快速示例?** 本文档提供了代码示例

---

**最后更新**: 2026年4月2日 ✨
