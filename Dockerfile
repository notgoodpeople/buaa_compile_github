From gcc:10.2
WORKDIR  /app/
COPY lab2.c ./
RUN gcc lab2.c -o lab2
RUN chmod +x lab2
