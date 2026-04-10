FROM node:18-alpine

WORKDIR /app

COPY package*.json ./
# If package-lock.json exists use `npm ci`, otherwise fall back to `npm install`
RUN if [ -f package-lock.json ]; then \
			npm ci --only=production --no-audit; \
		else \
			npm install --only=production --no-audit; \
		fi

COPY src/ ./src/

RUN mkdir -p logs

EXPOSE 8765 8766

CMD ["node", "src/index.js"]
