# Vendored Klong interpreter

`third_party/klong/` is the Klong 20221212 distribution by Nils M Holm,
taken from <https://t3x.org/klong/klong20221212.tgz>.

The recorded SHA-256 of that tarball is in `SHA256SUMS`.

Klong is in the public domain. Where a jurisdiction does not recognize
the public domain, the CC0 1.0 license in `klong/CC0` applies. Those
terms are not changed by the AGPL-3.0-only license of Topology Machine.
Do not strip `LICENSE`, `CC0`, or the copyright notices in `kg.c` and
`s9core.c`.

Build only the `kg` binary (`make -C third_party/klong kg`). Topology
Machine compares `klong/oracle.kg` with `tm oracle` by executing this
interpreter. A Python reimplementation is not a substitute.

Two upstream files are not UTF-8. They are stored as base64 sidecars so
a text-only upload can still carry their exact bytes:

- `lib/nstat.kg.b64` restores `lib/nstat.kg`, Latin-1 text. Byte 0xD7
  is a multiplication sign in a comment, not a corrupt binary.
- `klong.png.b64` restores `klong.png`, the upstream logo. `kg` does not
  need it to build or run.

`scripts/restore-upstream-bytes.sh` decodes both and checks SHA-256.
`scripts/bootstrap.sh` runs that restore before `make`.
