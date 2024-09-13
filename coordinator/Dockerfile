FROM python:3-alpine

RUN mkdir -p /usr/src/app
WORKDIR /usr/src/app

COPY requirements.txt /usr/src/app/

RUN pip3 install --no-cache-dir -r requirements.txt

COPY . /usr/src/app

ENV PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION="python"
# EXPOSE 8080

CMD ["python3", "-m", "flex.server"]