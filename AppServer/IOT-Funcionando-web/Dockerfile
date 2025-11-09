### Build stage
FROM node:20-alpine AS builder
WORKDIR /app

# copy package manifests first to leverage layer caching
COPY package.json package-lock.json ./

RUN npm ci --silent

# copy the rest and build
COPY . .
RUN npm run build

### Production stage
FROM nginx:stable-alpine

# Copy built assets
COPY --from=builder /app/dist /usr/share/nginx/html

# Use custom nginx config if present
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
