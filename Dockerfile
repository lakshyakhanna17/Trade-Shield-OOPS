# syntax=docker/dockerfile:1

# =============================================================================
# Stage 1: Build & Test Environment
# =============================================================================
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during apt install
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools, compilers, and required C++ libraries
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    gcc \
    g++ \
    libsqlite3-dev \
    libasio-dev \
    nlohmann-json3-dev \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copy backend source tree and database schema/seed for test execution
COPY backend/ ./backend/
COPY database/ ./database/

# Configure and compile using Ninja in Release mode
RUN cmake -B build -S backend \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCROW_BUILD_EXAMPLES=OFF \
    -DCROW_BUILD_TESTS=OFF

RUN cmake --build build --target tradeshield_server tradeshield_tests

# Run unit and integration test suite during build stage to guarantee build integrity
RUN ./build/tradeshield_tests

# =============================================================================
# Stage 2: Minimal Runtime Environment
# =============================================================================
FROM ubuntu:22.04 AS runner

ENV DEBIAN_FRONTEND=noninteractive

# Install only the runtime dependencies for SQLite and system CA certificates
RUN apt-get update && apt-get install -y --no-install-recommends \
    libsqlite3-0 \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Ensure runtime working directory and /tmp are properly structured
RUN mkdir -p /app/database /tmp && chmod 777 /app/database /tmp

# Copy compiled server binary from builder stage
COPY --from=builder /src/build/tradeshield_server /app/tradeshield_server

# Copy database schema and seed files required for SQLite startup initialization
COPY database/schema.sql /app/database/schema.sql
COPY database/seed.sql /app/database/seed.sql

RUN chmod +x /app/tradeshield_server

# Default environment configuration
# Render injects PORT dynamically; AppConfig resolves PORT with highest priority
ENV TRADESHIELD_HOST=0.0.0.0
ENV TRADESHIELD_PORT=8080
ENV TRADESHIELD_DB_PATH=/tmp/tradeshield.db
ENV TRADESHIELD_ENV=production

EXPOSE 8080

CMD ["/app/tradeshield_server"]

