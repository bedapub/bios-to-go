FROM gcc:4.9

LABEL author="Roland Schmucki, Jitao David Zhang, Martin Ebeling" \
      description="The BIOS library, to-go, with examples" \
      maintainer="roland.schmucki@roche.com"

COPY . /usr/bios-to-go/

RUN cd /usr/bios-to-go && mkdir bin && make

ENV PATH "$PATH:/usr/bios-to-go/bin"
