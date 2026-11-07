# Security

Topology Machine is a local command-line workbench. `tm` does not open network sockets, does not capture traffic, and does not change interface state. Do not run it setuid. It reads the files you pass it and writes the paths you name.

## What this tree refuses to be

Notes distributed with the starter included a raw packet-capture daemon and a toy authenticator. They are not part of this project.

- A promiscuous AF_PACKET socket is a surveillance mechanism. It is not required for the circuit models and it is not built here.
- `O_APPEND` plus `fsync` is not write-once storage. Another process with write access, or a later open without `O_APPEND`, can alter the file. This repository does not call that pattern WORM.
- A GF(2^8) multiply is field arithmetic. It is not a MAC, a signature, or a cipher. The notes’ keyed mix is not implemented. Do not cite `tm gf mul` as authentication.

If you need a real MAC, use a reviewed construction from a current cryptographic library. This repository will not grow one out of the field multiplier.

## AGPL and network use

The project license is AGPL-3.0-only. The shipped CLI has no remote interface. If you modify the program and users interact with your version over a network, section 13 of the AGPL requires corresponding source to be offered to those users.

## Reporting

There is no separate security inbox in this tree. Report a vulnerability to the maintainers of the repository you cloned, and include a way to reproduce it. Please do not send packet captures from networks you do not administer.

## Dependencies

The only vendored runtime is Klong, by Nils M Holm, under the public domain or CC0. Its `.sys` function can run a shell command if a script calls it. Topology Machine’s own scripts do not. Prefer `kg -n` so a `KLONGOPTS` image is not loaded, which is what the tests do. A `kg` built with upstream `-DSAFE` disables `.sys`; the default upstream build used here does not pass `-DSAFE`. Treat untrusted `.kg` files as code.
