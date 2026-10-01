RPM packaging
=============

Copyright 2026 Qore Technologies, s.r.o.

The canonical qore-msgpack-module.spec supports Fedora, Enterprise Linux and
openSUSE. It requires the Qore 3.0 SDK and qore-rpm-macros from the same repository.
The default build includes module tests and a separate documentation package.
Dependencies on the installed Qore ABI and SDK version are generated from the
built module; do not replace them with an unversioned qore dependency.

Prepare a pinned source bundle with qore-packaging, then build it in the target
distribution with networking disabled::

    python3 tools/packaging.py prepare --repo ../module-msgpack --ref COMMIT \
      --name qore-msgpack-module --version 1.0.1 \
      --spec qore-msgpack-module.spec --output work/msgpack-source
    python3 tools/build-local.py --source work/msgpack-source \
      --image TARGET_SDK_IMAGE --output results/msgpack-build --jobs 2

These commands run from the qore-packaging repository. Source preparation uses
the committed tree. Install the SDK's language documentation index for complete
Doxygen cross-references. --without docs and --without tests are available for
local diagnosis; repository qualification uses the defaults and also runs the
suite against installed RPMs outside the checkout. Native modules retain the
distribution's normal ELF stripping and separate debug packages.
