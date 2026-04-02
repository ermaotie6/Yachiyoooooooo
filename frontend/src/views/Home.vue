<template>
  <div class="home-container">
    <div class="hero-section">
      <h1>欢迎来到 Yachiyo</h1>
      <p>虚拟主播内容分享与AI互动平台</p>
      <div class="hero-buttons">
        <el-button type="primary" size="large" @click="navigateTo('/posts')">
          浏览内容
        </el-button>
        <el-button type="success" size="large" @click="navigateTo('/chat')">
          AI聊天
        </el-button>
      </div>
    </div>

    <div class="features-section">
      <h2>平台特色</h2>
      <el-row :gutter="20">
        <el-col :xs="24" :sm="12" :md="6">
          <div class="feature-card">
            <div class="feature-icon">🎭</div>
            <h3>内容分享</h3>
            <p>轻松分享您的创意内容</p>
          </div>
        </el-col>
        <el-col :xs="24" :sm="12" :md="6">
          <div class="feature-card">
            <div class="feature-icon">🤖</div>
            <h3>AI互动</h3>
            <p>与AI助手进行实时对话</p>
          </div>
        </el-col>
        <el-col :xs="24" :sm="12" :md="6">
          <div class="feature-card">
            <div class="feature-icon">🔒</div>
            <h3>内容审查</h3>
            <p>智能审查确保社区安全</p>
          </div>
        </el-col>
        <el-col :xs="24" :sm="12" :md="6">
          <div class="feature-card">
            <div class="feature-icon">👥</div>
            <h3>社区互动</h3>
            <p>与其他创作者互动交流</p>
          </div>
        </el-col>
      </el-row>
    </div>

    <div class="stats-section">
      <el-row :gutter="20">
        <el-col :xs="24" :sm="12" :md="6">
          <div class="stat-card">
            <div class="stat-number">10K+</div>
            <div class="stat-label">活跃用户</div>
          </div>
        </el-col>
        <el-col :xs="24" :sm="12" :md="6">
          <div class="stat-card">
            <div class="stat-number">100K+</div>
            <div class="stat-label">内容作品</div>
          </div>
        </el-col>
        <el-col :xs="24" :sm="12" :md="6">
          <div class="stat-card">
            <div class="stat-number">99.9%</div>
            <div class="stat-label">平台可用性</div>
          </div>
        </el-col>
        <el-col :xs="24" :sm="12" :md="6">
          <div class="stat-card">
            <div class="stat-number">24h</div>
            <div class="stat-label">AI响应时间</div>
          </div>
        </el-col>
      </el-row>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'

const router = useRouter()

const navigateTo = (path: string) => {
  router.push(path)
}
</script>

<style scoped>
.home-container {
  padding: 40px 20px;
}

.hero-section {
  text-align: center;
  padding: 60px 20px;
  color: white;
  margin-bottom: 60px;
}

.hero-section h1 {
  font-size: 48px;
  font-weight: bold;
  margin-bottom: 20px;
  text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.2);
}

.hero-section p {
  font-size: 20px;
  margin-bottom: 40px;
  opacity: 0.9;
}

.hero-buttons {
  display: flex;
  gap: 20px;
  justify-content: center;
  flex-wrap: wrap;
}

.features-section {
  margin-bottom: 60px;
}

.features-section h2 {
  font-size: 32px;
  text-align: center;
  margin-bottom: 40px;
  color: white;
}

.feature-card {
  background: white;
  padding: 30px 20px;
  border-radius: 12px;
  text-align: center;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  transition: transform 0.3s, box-shadow 0.3s;
  cursor: pointer;
}

.feature-card:hover {
  transform: translateY(-5px);
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.15);
}

.feature-icon {
  font-size: 48px;
  margin-bottom: 15px;
}

.feature-card h3 {
  font-size: 20px;
  margin-bottom: 10px;
  color: #333;
}

.feature-card p {
  color: #666;
  font-size: 14px;
}

.stats-section {
  margin-bottom: 40px;
}

.stat-card {
  background: rgba(255, 255, 255, 0.95);
  padding: 30px 20px;
  border-radius: 12px;
  text-align: center;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
}

.stat-number {
  font-size: 36px;
  font-weight: bold;
  color: #667eea;
  margin-bottom: 10px;
}

.stat-label {
  font-size: 16px;
  color: #666;
}

@media (max-width: 768px) {
  .hero-section h1 {
    font-size: 32px;
  }

  .hero-section p {
    font-size: 16px;
  }

  .features-section h2 {
    font-size: 24px;
  }
}
</style>
