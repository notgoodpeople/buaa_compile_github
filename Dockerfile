From gcc:10.2
WORKDIR  /app/
COPY lab3.cpp ./
RUN g++ lab3.cpp -o lab3
RUN chmod +x lab3