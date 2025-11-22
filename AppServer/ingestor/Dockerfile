FROM python:3.10-slim

WORKDIR /app

# Instala as bibliotecas Python
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copia o script principal
COPY main.py .

CMD ["python", "main.py"]