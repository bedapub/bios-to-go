bios-to-go: the BIOS (Bioinfo-C) library, to go
===
Jitao David Zhang, Roland Schmucki, Martin Ebeling, Detlef Wolf, April 2023

# Background

This repository contains static files of the BIOS (Bioinfo-C) library, both header
files and source files. Users can write programs depending on the BIOS
(Bioinfo-C) library, compile them against the static files.

# Usage

Head and source files of the BIOS (Bioinfo-C) library are found in the `kern`
directory. Users can clone from this library, write their own C programs in the
`src` directory, and compile them together with BIOS files.

# Technical note

The `kern` library contains source code from the
[`Bioinfo-C`](https://sourceforge.net/projects/bioinfoc/) library originally
written by Clemens Broger and colleagues, and maintained by Detlef Wolf. There
are minor changes by Jitao David Zhang to be compatible and warning-free (as
much as possible) for GCC 10.0, which was done for the development of `ribios`
packages, especially [`ribiosUtils`](https://github.com/bedapub/ribiosUtils).

Because the Bioinfo-C library is not any more updated actively, and the source
codes in `ribiosUtils` are adapted for R use, the source code in this directory
is not synced with neither of the origins. Any changes will be only visible
here. Critical updates need to be ported to other sources. Please contact Jitao
David Zhang and Detlef Wolf in these cases.

# Docker image

A Docker image is being created GitHub-Action and stored under the GitHub Packages Registry. The URI's the to images are

- Docker image: `ghcr.io/bedapub/bios-to-go:main`

Here, a command to run the Docker image with Singularity
```bash
singularity run docker://ghcr.io/bedapub/bios-to-go:main bash
```
