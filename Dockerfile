From gcc:10.2
WORKDIR  /app/
COPY lab4.cpp ./
RUN g++ lab4.cpp -o lab4
RUN chmod +x lab4