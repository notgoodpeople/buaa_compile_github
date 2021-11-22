From gcc:10.2
WORKDIR  /app/
COPY lab7.cpp ./
RUN g++ lab6.cpp -o lab7
RUN chmod +x lab7