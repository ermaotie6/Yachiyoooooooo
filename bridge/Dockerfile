FROM node:18-alpine

WORKDIR /app

COPY package*.json ./
RUN npm ci --only=production

COPY src/ ./src/

RUN mkdir -p logs

EXPOSE 8765 8766

CMD ["node", "src/index.js"]
