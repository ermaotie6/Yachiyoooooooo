import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { fileURLToPath, URL } from 'node:url'
import path from 'node:path'
import fs from 'node:fs'

export default defineConfig({
  plugins: [
    vue(),
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
  build: {
    rollupOptions: {
      output: {
        manualChunks: {
          'vendor-pixi': ['pixi.js', 'pixi-live2d-display/cubism4'],
          'vendor-ui': ['element-plus', '@element-plus/icons-vue'],
        }
      }
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
