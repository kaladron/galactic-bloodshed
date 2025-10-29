# SPDX-License-Identifier: Apache-2.0
"""
Network communication for Galactic Bloodshed client

This module handles:
- Network buffer for partial lines
- Connection to game server
- Async send/receive operations
"""

import asyncio
import logging
import socket
from collections import deque
from typing import List, Optional


class NetworkBuffer:
    """Buffer for handling partial lines from network"""
    
    def __init__(self):
        self.buffer = ""
        self.lines: deque[str] = deque()
    
    def add_data(self, data: str) -> List[str]:
        """Add data to buffer and return complete lines"""
        self.buffer += data
        
        # Split on newlines but keep incomplete last line
        parts = self.buffer.split('\n')
        self.buffer = parts[-1]  # Keep incomplete line
        
        # Add complete lines to queue
        complete_lines = parts[:-1]
        self.lines.extend(complete_lines)
        
        return complete_lines
    
    def get_line(self) -> Optional[str]:
        """Get next complete line from buffer"""
        return self.lines.popleft() if self.lines else None
    
    def has_lines(self) -> bool:
        """Check if buffer has complete lines"""
        return len(self.lines) > 0


class GameConnection:
    """Manages connection to game server"""
    
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None
        self.reader: Optional[asyncio.StreamReader] = None
        self.writer: Optional[asyncio.StreamWriter] = None
        self.buffer = NetworkBuffer()
    
    async def connect(self) -> bool:
        """Establish connection to game server"""
        try:
            self.reader, self.writer = await asyncio.open_connection(
                self.host, self.port
            )
            logging.info(f"Connected to {self.host}:{self.port}")
            return True
        except Exception as e:
            logging.error(f"Connection failed: {e}")
            return False
    
    async def send(self, message: str):
        """Send message to server"""
        if not self.writer:
            return
        
        if not message.endswith('\n'):
            message += '\n'
        
        self.writer.write(message.encode('utf-8'))
        await self.writer.drain()
    
    async def receive(self) -> Optional[str]:
        """Receive data from server"""
        if not self.reader:
            return None
        
        try:
            data = await self.reader.read(4096)
            if not data:
                return None
            
            text = data.decode('utf-8', errors='replace')
            return text
        except Exception as e:
            logging.error(f"Receive error: {e}")
            return None
    
    def close(self):
        """Close connection"""
        if self.writer:
            self.writer.close()
