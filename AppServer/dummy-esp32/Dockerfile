FROM python:3.11-slim

WORKDIR /app

# Install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy simulator code
COPY dummy_esp32.py .

# Run the simulator
CMD ["python", "-u", "dummy_esp32.py"]
