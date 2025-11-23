FROM python:3.10-slim

# Disable Python output buffering for Docker logs
ENV PYTHONUNBUFFERED=1

WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY api.py .
CMD ["python", "-u", "api.py"]