From gcc:10.2
WORKDIR  /app/
COPY lab9.cpp ./
RUN g++ lab9.cpp -o lab9
RUN chmod +x lab9
