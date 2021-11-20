From gcc:10.2
WORKDIR  /app/
COPY lab6.cpp ./
RUN g++ lab6.cpp -o lab6
RUN chmod +x lab6