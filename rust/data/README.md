# RSST configuration data

`rsst-unavoidable.conf` is the mathematical catalogue of 633 configurations
published by Neil Robertson, Daniel P. Sanders, Paul Seymour, and Robin Thomas.
It was retrieved unchanged from the [authors' archive](https://thomas.math.gatech.edu/FC/ftpinfo.html).
The source URL, SHA-256, byte count, and retrieval date are recorded in
[sources.json](../../verification/rsst/sources.json).

The vertices of each free completion include the abstract ring, in cyclic order,
followed by the configuration interior. The file also specifies contract edges
and cyclic neighbor order. The parser checks dimensions, incidences, and the
spherical embedding for every entry.

This third-party mathematical data is attributed to the original authors; this
repository does not claim copyright in it or relicense their programs. The
authors' C programs carry a scholarly-research permission notice and are **not
bundled** here. The reproduction script fetches them separately into ignored
build storage and checks their pinned hashes before use. The new Rust source is
an independent implementation of the mathematical procedures.
