FROM base/devel as build

# install dependencies in build container
RUN pacman --noconfirm -Syu \
  ocaml ocaml-findlib opam \
  ntl boost boost-libs cmake
RUN opam init -y; \
  opam switch -y 4.06.0; \
  eval `opam config env`; \
  opam install -y camlp4 ocamlfind ocamlbuild batteries;

# (re-)build
WORKDIR /app
COPY . .
RUN eval `opam config env`; \
  make cleanall; \
  make

# copy dependencies
RUN mkdir /deps; \
  find bin -type f | while read bin; do \
    ldd $bin | cut -d' ' -f 3 | grep /usr/lib | while read lib; do \
      cp $lib /deps; \
    done; \
  done

# copy everything into minimal image
FROM gcr.io/distroless/base
COPY --from=build /app/bin /bin
COPY --from=build /app/config /config
COPY --from=build /deps /deps

# let the minimal image find libraries in the correct order
ENV LD_LIBRARY_PATH=/lib/x86_64-linux-gnu/:/deps
# expose default ports
EXPOSE 12347
EXPOSE 12357