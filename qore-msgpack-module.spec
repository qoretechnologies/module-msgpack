# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
# Use the pinned source epoch for RPM headers and installed file timestamps.
%global source_date_epoch_from_changelog 1
%global use_source_date_epoch_as_buildtime 1
%if v"%{rpmversion}" >= v"4.20"
%global build_mtime_policy clamp_to_source_date_epoch
%else
%global clamp_mtime_to_source_date_epoch 1
%endif
%bcond_without tests
%bcond_without docs
Name: qore-msgpack-module
Version: 1.0.1
Release: 2%{?dist}
Summary: MessagePack serialization for Qore
License: MIT
URL: https://github.com/qoretechnologies/module-msgpack
Source0: %{name}-%{version}.tar.xz
BuildRequires: cmake >= 3.5
BuildRequires: make
BuildRequires: gcc-c++
BuildRequires: qore-devel >= 3.0.0~
BuildRequires: qore-rpm-macros >= 3.0.0~
%if %{with docs}
BuildRequires: doxygen
BuildRequires: /usr/bin/hardlink
%endif

Provides: bundled(mpack) = 1.0.0

%description
Native Qore interfaces for MessagePack serialization, deserialization and
extension types, using the bundled MPack implementation.

%if %{with docs}
%package doc
Summary: MessagePack module reference documentation
BuildArch: noarch
%description doc
API reference and examples for Qore's MessagePack module.
%endif

%prep
%autosetup
%build
%{?set_build_flags}
. %{_rpmconfigdir}/qore/module-env.sh
qore_set_source_prefix_maps "%{qore_debug_source_dir}"
cmake -S . -B build -G 'Unix Makefiles' \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE=-DNDEBUG \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCMAKE_INSTALL_LIBDIR=%{_lib} \
  -DCMAKE_SKIP_RPATH=ON -DCMAKE_IGNORE_PREFIX_PATH=/usr/local \
  -DQore_DIR=%{_libdir}/cmake/Qore -DQORE_EXECUTABLE=/usr/bin/qore \
  -DQORE_QPP_EXECUTABLE=/usr/bin/qpp \
  -DCMAKE_DISABLE_FIND_PACKAGE_Doxygen=%{!?with_docs:ON}%{?with_docs:OFF}
cmake --build build -- %{?_smp_mflags}
%if %{with docs}
cmake --build build --target docs -- %{?_smp_mflags}
%endif
%install
DESTDIR=%{buildroot} cmake --install build
chmod 755 %{buildroot}%{_libdir}/qore-modules/msgpack-api-*.qmod
%if %{with docs}
install -d %{buildroot}%{_docdir}/%{name}-doc
cp -a build/docs/msgpack/html %{buildroot}%{_docdir}/%{name}-doc/
hardlink -t -O %{buildroot}%{_docdir}/%{name}-doc
%endif
%check
%if %{with tests}
. %{_rpmconfigdir}/qore/module-env.sh
/usr/bin/qore -b --enable-debug -l "$PWD/build/msgpack-api-$(/usr/bin/qore --latest-module-api).qmod" test/msgpack.qtest -v
%endif
%files
%license debian/copyright
%doc README.md
%{_libdir}/qore-modules/msgpack-api-*.qmod
%dir %{_datadir}/qore/metadata/msgpack
%{_datadir}/qore/metadata/msgpack/*.meta.json
%if %{with docs}
%files doc
%license debian/copyright
%doc %{_docdir}/%{name}-doc/
%endif
%changelog
* Thu Oct 01 2026 David Nichols <david@qore.org> - 1.0.1-2
- Use the packaged SDK, generated ABI requirements and offline module tests.
