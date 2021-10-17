From gcc:10
WORKDIR  /app/
COPY Token.c ./
RUN gcc Token.c -o Token
RUN chmod +x Token