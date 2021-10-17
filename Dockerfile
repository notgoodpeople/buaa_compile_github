From gcc:10
WORKDIR  /app/
COPY lab1.c ./
RUN gcc lab1.c -o lab1
RUN chmod +x lab1