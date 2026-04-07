import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { fileURLToPath, URL } from 'node:url'
import path from 'node:path'
import fs from 'node:fs'

export default defineConfig({
  plugins: [
    vue(),
    // 自定义插件：将项目根目录的 resources/ 映射为静态资源（开发时）
    {
      name: 'serve-live2d-resources',
      configureServer(server) {
        server.middlewares.use('/resources', (req, res, next) => {
          const filePath = path.resolve(__dirname, '..', 'resources', req.url || '')
          if (fs.existsSync(filePath) && fs.statSync(filePath).isFile()) {
            res.setHeader('Access-Control-Allow-Origin', '*')
            const stream = fs.createReadStream(filePath)
            stream.pipe(res)
          } else {
            next()
          }
        })
      }
    }
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    }
  },
  server: {
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://localhost:8080',
        changeOrigin: true
      },
      '/ws': {
        target: 'ws://localhost:9001',
        ws: true,
        changeOrigin: true,
        rewrite: (path) => path.replace(/^\/ws/, '')
      }
    }
  }
})
