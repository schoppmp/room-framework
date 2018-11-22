FROM base/devel as build

# install dependencies in build container
RUN pacman --noconfirm -Syu \
  ocaml ocaml-findlib opam \
  ntl boost boost-libs cmake eigen
RUN opam init --disable-sandboxing -y; \
  opam switch create -y 4.06.0; \
  eval `opam config env`; \
  opam install -y camlp4 ocamlfind ocamlbuild batteries;

# (re-)build library dependencies
WORKDIR /app
COPY lib /app/lib
COPY Makefile /app/Makefile
RUN eval `opam config env`; \
  make cleanall; \
  make libs NDEBUG=1

# build binaries
COPY src /app/src
RUN eval `opam config env`; \
  make clean; \
  make NDEBUG=1

COPY config config
COPY benchmarks benchmarks

# expose default ports
EXPOSE 12347
EXPOSE 12357
