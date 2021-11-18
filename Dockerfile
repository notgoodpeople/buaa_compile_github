From gcc:10.2
WORKDIR  /app/
COPY lab5.cpp ./
RUN g++ lab5.cpp -o lab5
RUN chmod +x lab5