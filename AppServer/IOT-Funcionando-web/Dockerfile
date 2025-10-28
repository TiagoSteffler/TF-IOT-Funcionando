# Multi-stage Dockerfile: build Vite app with Node, then serve with nginx

# Builder stage
FROM node:18-alpine AS builder
WORKDIR /app

# Install dependencies (use package-lock or yarn.lock if present)
COPY package*.json ./
RUN npm ci --silent

# Copy rest of source and build
COPY . .
RUN npm run build

# Production stage: serve with nginx
FROM nginx:stable-alpine
COPY --from=builder /app/dist /usr/share/nginx/html
# Use our nginx config to enable SPA fallback
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
