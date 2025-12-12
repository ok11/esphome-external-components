FROM esphome/esphome:latest


# Install curl for healthcheck
RUN apt-get update && \
    apt-get install -y curl && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install additional tools for testing
COPY ../requirements.txt .
RUN pip3 install --no-cache-dir -r requirements.txt

# Copy wait script
COPY scripts/wait_for_esphome.sh /usr/local/bin/wait_for_esphome.sh
RUN chmod +x /usr/local/bin/wait_for_esphome.sh

# Create config directory
RUN mkdir -p /config

WORKDIR /config

EXPOSE 6052 6053

# Default command (can be overridden by docker-compose)
CMD ["dashboard", "/config"]
