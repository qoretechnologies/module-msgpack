## module-msgpack

[msgpack]: https://msgpack.org
[qore]: https://github.com/qorelanguage/qore

https://github.com/qorelanguage/module-msgpack

[MessagePack][msgpack] module for the [Qore][qore] programming language.

## Building

```
mkdir build
cd build
cmake ..
make
sudo make install
```

### Building documentation

```
cd build
cmake .. # not needed if done before
make docs
```

Develop CI
----------

On develop, each platform can reuse its successful full job for the same Git
tree and Qore environment, including after a squash merge. Both platforms
must have passed in the same prior pipeline. Reused statuses link to the
original jobs. A changed fingerprint or missing evidence runs the full suite.
Set QORE_CI_FORCE_FULL=1 to force both suites.
