Name:           kagome-cpp
Version:        1.0.2
Release:        1%{?dist}
Summary:        Modern C++ implementation of Japanese morphological analyzer

License:        Apache-2.0
URL:            https://github.com/fatalbanana/kagome-cxx
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.22
BuildRequires:  libarchive-devel
BuildRequires:  libicu-devel
%if 0%{?el9}
BuildRequires:  gcc-toolset-12-gcc-c++
%endif
%if 0%{?el10}
BuildRequires:  gcc-toolset-15-gcc-c++
%endif
%if 0%{getenv:ASAN}
%if 0%{?el9}
BuildRequires:  gcc-toolset-12-libasan-devel
%endif
%if 0%{?el10}
BuildRequires:  gcc-toolset-15-libasan-devel
%endif
%endif

Requires:           libicu
Requires:           libarchive

%description
Kagome C++ is a modern C++20 implementation of the Japanese morphological
analyzer kagome, originally written in Go. This implementation uses modern
C++ features and libraries for high-performance Japanese text tokenization.

Features:
 * Modern C++20 with concepts and format library
 * High performance with efficient hash tables
 * Unicode support with proper UTF-8 handling
 * Multiple tokenization modes (Normal, Search, Extended)
 * Lattice-based analysis using Viterbi algorithm
 * Dictionary support (System and user dictionaries)
 * Memory efficient with object pooling


%package devel
Summary:        Development files for Kagome C++ library
Requires:       libicu-devel
Requires:       libarchive-devel

%description devel
Kagome C++ is a modern C++20 implementation of the Japanese morphological
analyzer kagome, originally written in Go.

This package contains the development files (headers and static libraries).


%package tools
Summary:        Command-line tools for Kagome Japanese morphological analyzer
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       %{name}-dict-ipa

%description tools
This package provides command-line utilities for Japanese text analysis
using the Kagome morphological analyzer.

Included tools:
 * kagome_main - Interactive and batch text analysis
 * kagome_tests - Test suite for validation

The tools support multiple tokenization modes, JSON output, and lattice
visualization for detailed morphological analysis debugging.


%package dict-ipa
Summary:        IPA dictionary for Kagome Japanese morphological analyzer
BuildArch:      noarch

%description dict-ipa
This package contains the IPA (Information-technology Promotion Agency)
dictionary required by the Kagome Japanese morphological analyzer.

The dictionary provides:
 * Part-of-speech information
 * Reading (pronunciation) data
 * Word segmentation rules
 * Morphological features for Japanese text analysis

This dictionary is based on the IPAdic dictionary and is required for
proper operation of the Kagome tokenizer.


%package rspamd-tokenizer
Summary:        Japanese tokenizer plugin for Rspamd mail system
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       %{name}-dict-ipa
Suggests:       rspamd

%description rspamd-tokenizer
This package provides a shared library plugin for Rspamd mail processing
system, enabling Japanese text tokenization for spam detection and email
classification.

The plugin uses the Kagome C++ library to perform morphological analysis
of Japanese text, providing better tokenization for Japanese emails than
standard Unicode word breaking.

Features:
 * Automatic Japanese text detection
 * High-performance morphological analysis
 * Seamless integration with Rspamd tokenizer framework
 * Configurable confidence thresholds
 * Fallback to default tokenizer for non-Japanese text


%prep
%autosetup -n %{name}-%{version}


%build
%if 0%{?el9}
source /opt/rh/gcc-toolset-12/enable
%endif
%if 0%{?el10}
source /usr/lib/gcc-toolset/15-env.source
%endif
# Conditionally enable ASAN if environment variable is set
%if 0%{getenv:ASAN}
%cmake -DCMAKE_BUILD_TYPE=Debug -DKAGOME_ENABLE_ASAN=ON
%else
%cmake -DCMAKE_BUILD_TYPE=Release
%endif
%cmake_build

%check
sh -c 'export ASAN_OPTIONS="abort_on_error=0:detect_leaks=1:log_path=./asan.log:print_summary=1"; %ctest' || ctest_result=$?
find . -name "asan.log.*" 2>/dev/null | xargs -I {} sh -c 'echo "=== {} ===" && cat {}' 2>/dev/null || true
exit ${ctest_result:-0}

%install
%cmake_install


%files
%license LICENSE

%files devel
%{_includedir}/kagome/
%{_libdir}/libkagome_cpp.a
%{_libdir}/libkagome_c_api.a
%doc README.md

%files tools
%{_bindir}/kagome_main
%{_bindir}/kagome_tests
%doc README.md DICTIONARY.md

%files dict-ipa
%{_datadir}/kagome/

%files rspamd-tokenizer
%{_libdir}/kagome_rspamd_tokenizer.so*


%changelog
* Thu Feb 13 2025 Kagome C++ Team <vsevolod@rspamd.com> - 1.0.2-1
- Fix stemmed flag propagation in tokenizer
- Update to version 1.0.2
