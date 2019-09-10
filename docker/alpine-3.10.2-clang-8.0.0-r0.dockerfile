FROM alpine:3.10.2

WORKDIR /workspace

ENV CLANG_VERSION=8.0.0-r0
RUN apk add --no-cache clang==$CLANG_VERSION

ENTRYPOINT ["clang-format"]
CMD ["--help"]
