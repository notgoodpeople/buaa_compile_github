From gcc:10.2
WORKDIR  /app/
COPY lab3.cpp ./
RUN g++ lab3.cpp -o main.ll
RUN clang -emit-llvm -S libsysy.c -o lib.ll
RUN llvm-link main.ll lib.ll -S -o out.ll
RUN chmod +x out.ll