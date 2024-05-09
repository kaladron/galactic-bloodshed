FROM debian:testing

# Update packages
RUN apt update && apt upgrade -y

# Install additional tools (optional)
RUN apt install -y git build-essential python3 python3-pip adduser sudo cmake

# Create a new user (replace "codespaceuser" with your desired username)
RUN adduser --disabled-password --gecos "" codespaceuser
RUN usermod -aG sudo codespaceuser
RUN passwd -d codespaceuser

# Give ownership of the workspace directory to the new user
RUN mkdir -p /workspaces
RUN chown -R codespaceuser:codespaceuser /workspaces

# Switch to the new user for subsequent commands
USER codespaceuser

# Copy your project files (optional)
WORKDIR /workspaces
COPY . .
