# Live2D 缺失分析与修复方案

## 一、当前状态

### 模型文件

| 文件 | 状态 | 说明 |
|------|------|------|
| `yachiyo.moc3` (7MB) | ✅ | 模型骨骼/网格数据 |
| `yachiyo.model3.json` | ✅ | 模型描述文件 (入口) |
| `yachiyo.physics3.json` | ✅ | 物理模拟 (头发/衣服摆动) |
| `yachiyo.cdi3.json` | ✅ | 显示参数定义 |
| `texture_00.png` / `texture_01.png` | ✅ | 纹理贴图 |

### 表情文件 (Expressions)

| 文件 | 对应参数 | 状态 | 用途 |
|------|---------|------|------|
| `f_smile.exp3.json` | ParamExpression_3 | ✅ | 微笑 (happy) |
| `f_sad.exp3.json` | ParamExpression_1 | ✅ | 悲伤 (sad) |
| `f_cry.exp3.json` | ParamExpression_2 | ✅ | 哭泣 (cry) |
| `f_squint.exp3.json` | ParamExpression_4 | ✅ | 眯眼 (被复用为 angry/surprised) |
| `f_default.exp3.json` | (组合) | ❌ | 默认表情 (neutral) |
| `f_angry.exp3.json` | (组合) | ❌ | 生气 (angry) |
| `f_surprised.exp3.json` | (组合) | ❌ | 惊讶 (surprised) |
| `f_confused.exp3.json` | (组合) | ❌ | 困惑 |
| `f_thinking.exp3.json` | (组合) | ❌ | 思考 |

### 动作文件 (Motions)

| 文件 | 状态 | 用途 |
|------|------|------|
| `motions/m_idle.motion3.json` | ❌ | 待机 |
| `motions/m_greet.motion3.json` | ❌ | 打招呼 |
| `motions/m_wave.motion3.json` | ❌ | 挥手 |
| `motions/m_nod.motion3.json` | ❌ | 点头 |
| `motions/m_shake.motion3.json` | ❌ | 摇头 |
| `motions/m_think.motion3.json` | ❌ | 思考 |

---

## 二、影响评估

### 当前能工作的

| 功能 | 状态 | 说明 |
|------|------|------|
| 模型加载和渲染 | ✅ | pixi-live2d-display 正常 |
| 物理模拟 (头发摆动) | ✅ | physics3.json 正常 |
| 微笑表情 f_smile | ✅ | |
| 悲伤表情 f_sad | ✅ | |
| 口型同步 (ParamMouthOpenY) | ✅ | Web Audio API 驱动 |
| 眼球追踪 | ✅ | setEyeTrackingTarget() |

### 当前不工作的

| 功能 | 问题 | 影响 |
|------|------|------|
| **所有动作 (Motions)** | motions/ 目录完全缺失 | 点头/挥手/思考等动作无视觉效果 |
| 生气表情 | f_angry 缺失, fallback → f_squint | 生气和眯眼表情相同, 区分度差 |
| 惊讶表情 | f_surprised 缺失, fallback → f_cry | 惊讶和哭泣表情相同 |
| 默认表情 | f_default 缺失 | 无 "重置到中性" 的能力 |
| 困惑/思考表情 | f_confused/f_thinking 缺失 | 无法表达这两个情感 |

---

## 三、修复方案

### 方案 A：创建缺失的 expression 文件（5分钟，立即生效）

这个模型有 **4 个表情参数槽位** (ParamExpression_1~4)，已经被占用了：
- ParamExpression_1 → f_sad
- ParamExpression_2 → f_cry
- ParamExpression_3 → f_smile
- ParamExpression_4 → f_squint

**由于没有独立参数了，f_angry / f_surprised / f_confused / f_thinking 必须用多个参数的组合来实现。** 同时需要利用现有的眼睛和眉毛参数。

#### 创建 f_default.exp3.json（中性表情）

```json
{
    "Type": "Live2D Expression",
    "Parameters": [
        {"Id": "ParamExpression_1", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamExpression_2", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamExpression_3", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamExpression_4", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamHide_EyesL1", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamHighLightHide_EyesL1", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamHide_EyeSocket", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamHide_EyeSocket2", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamBrowLY", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamBrowRY", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamMouthForm", "Value": 0.0, "Blend": "Add"}
    ]
}
```

#### 创建 f_angry.exp3.json（生气 = 眯眼 + 眉毛下压）

```json
{
    "Type": "Live2D Expression",
    "Parameters": [
        {"Id": "ParamExpression_4", "Value": 1.0, "Blend": "Add"},
        {"Id": "ParamBrowLY", "Value": -0.3, "Blend": "Add"},
        {"Id": "ParamBrowRY", "Value": -0.3, "Blend": "Add"},
        {"Id": "ParamBrowLAngle", "Value": 0.3, "Blend": "Add"},
        {"Id": "ParamBrowRAngle", "Value": 0.3, "Blend": "Add"},
        {"Id": "ParamMouthForm", "Value": -0.2, "Blend": "Add"},
        {"Id": "ParamExpression_1", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamExpression_2", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamExpression_3", "Value": 0.0, "Blend": "Add"}
    ]
}
```

#### 创建 f_surprised.exp3.json（惊讶 = 瞪眼 + 张嘴）

```json
{
    "Type": "Live2D Expression",
    "Parameters": [
        {"Id": "ParamExpression_3", "Value": 0.5, "Blend": "Add"},
        {"Id": "ParamEyeLOpen", "Value": 0.3, "Blend": "Add"},
        {"Id": "ParamEyeROpen", "Value": 0.3, "Blend": "Add"},
        {"Id": "ParamMouthOpenY", "Value": 0.3, "Blend": "Add"},
        {"Id": "ParamBrowLY", "Value": 0.3, "Blend": "Add"},
        {"Id": "ParamBrowRY", "Value": 0.3, "Blend": "Add"},
        {"Id": "ParamExpression_1", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamExpression_2", "Value": 0.0, "Blend": "Add"},
        {"Id": "ParamExpression_4", "Value": 0.0, "Blend": "Add"}
    ]
}
```

#### 创建 f_confused.exp3.json + f_thinking.exp3.json

使用类似的参数组合策略（眯眼 + 偏头 + 眉毛），可以后续微调。

### 方案 B：Motion 文件问题 — 3 种解决路径

#### 路径 B1：用 Live2D Cubism Editor 制作（推荐，需要工具）

1. 下载 Live2D Cubism Editor (免费版即可): https://www.live2d.com/en/download/cubism/
2. 打开模型工程文件 (`.cmox` 或 `.can3`)，如果没有工程文件，可以用 `.moc3` 导入
3. 在 Animation 工作区创建 6 个 Motion：
   - `m_idle` → 轻微呼吸动画，循环
   - `m_greet` → 手臂抬起 + 挥手 + 微笑
   - `m_wave` → 手臂左右摆动
   - `m_nod` → 头部上下点头
   - `m_shake` → 头部左右摇动
   - `m_think` → 歪头 + 手托下巴
4. 导出为 `.motion3.json` 格式
5. 放入 `resources/live2d/motions/` 目录

**这是获得完整动作体验的唯一方法。没有 Live2D Cubism Editor 无法生成 .motion3.json 文件。**

#### 路径 B2：前端参数动画模拟（无 Motion 文件时的替代方案）

利用 CDI 中已有的参数 (ParamAngleX/Y, ParamBodyAngleX/Y/Z 等)，在前端用 JavaScript 创建简单的 CSS-like 动画：

```typescript
// 在 Live2DComponent.vue 中添加
const playParameterAnimation = (paramName: string, from: number, to: number, durationMs: number) => {
  const start = performance.now()
  const tick = () => {
    const elapsed = performance.now() - start
    const progress = Math.min(1, elapsed / durationMs)
    const value = from + (to - from) * easeInOut(progress)
    
    const coreModel = (model as any).internalModel?.coreModel
    if (coreModel) {
      coreModel.setParameterValueById(paramName, value)
    }
    
    if (progress < 1) requestAnimationFrame(tick)
  }
  requestAnimationFrame(tick)
}

// 模拟动作
const simulateNod = () => {
  playParameterAnimation('ParamAngleX', 0, 0.1, 200)  // 低头
  setTimeout(() => playParameterAnimation('ParamAngleX', 0.1, 0, 200), 200) // 抬头
}

const simulateWave = () => {
  playParameterAnimation('ParamArmRAngle', 0, 1.0, 400)  // 抬手
  setTimeout(() => playParameterAnimation('ParamArmRAngle', 1.0, 0, 400), 400) // 放下
}
```

**需要先确认模型有哪些 Body 参数可用。** 查看 CDI:

```bash
python3 -c "
import json
cdi = json.load(open('resources/live2d/yachiyo.cdi3.json'))
for p in cdi.get('Parameters', []):
    pid = p.get('Id','')
    if any(x in pid for x in ['Angle','Body','Arm','Head','Hand']):
        print(f'  {pid}')
"
```

#### 路径 B3：下载免费 Live2D Motion 资源

一些免费 Live2D 模型自带 motion 文件，可以从这些资源中提取作为参考：

- Live2D 官方示例模型 (如 Haru, Hiyori, Mao) 自带 motions
- Nizima 市场有免费/付费 motion 素材: https://nizima.com
- BOOTH 也有 Live2D motion 素材

**注意**: Motion 文件绑定特定模型的骨骼名称。从其他模型复制的 motion 文件，参数 ID 不匹配的话不会生效。只能作学习参考。

### 推荐实施顺序

```
第一优先级 (立即可做):
  1. 创建 5 个缺失的 .exp3.json 文件
  2. 更新 Live2DAnimationService.cpp 的 emotion 映射

第二优先级 (有时间时):
  3. 用 Live2D Cubism Editor 制作 6 个 motion3.json 文件
  4. 在此之前, 采用路径 B2 的 JavaScript 参数动画作为降级方案

第三优先级 (锦上添花):
  5. 微调参数值, 使表情/动作更自然
  6. 增加更多情感→表情映射
```

### 后端映射修正

更新 `Live2DAnimationService.cpp` 中的 `mapEmotionToExpression()`:

```
当前:                                 修正后:
"生气/angry"    → f_squint           → f_angry (新建)
"惊讶/surprised" → f_cry             → f_surprised (新建)
无映射            → f_smile(fallback)  → f_default  (新建, 中性)
无映射            -                    → f_confused (新建)
无映射            -                    → f_thinking (新建)
```
