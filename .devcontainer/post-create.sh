#!/usr/bin/env bash

set -euxo pipefail

PS4='+${BASH_SOURCE[0]}:$LINENO: '
if [[ -t 1 ]] && type -t tput >/dev/null; then
  if (( "$(tput colors)" == 256 )); then
    PS4='$(tput setaf 10)'$PS4'$(tput sgr0)'
  else
    PS4='$(tput setaf 2)'$PS4'$(tput sgr0)'
  fi
fi

# Install prerequisite packages.
sudo apt-get -y update
sudo apt-get -y dist-upgrade
sudo apt-get -y install \
    cmake \
    python3-dev
sudo apt-get clean
sudo rm -rf /var/lib/apt/lists/*
sudo chown vscode:vscode /workspaces

# Install Boost C++ Libraries.
pushd /workspaces
BOOST_LATEST_VERSION="$(git ls-remote --tags https://github.com/boostorg/boost.git \
  | grep -E 'refs/tags/boost-[[:digit:]]+\.[[:digit:]]+\.[[:digit:]]+(-[[:digit:]]+)?$' \
  | grep -Eo '[[:digit:]]+\.[[:digit:]]+\.[[:digit:]]+(-[[:digit:]]+)?$' \
  | sort -V \
  | tail -n 1)"
BOOST_TARBALL_BASENAME="boost-$BOOST_LATEST_VERSION-cmake.tar.xz"
BOOST_TARBALL_URL="https://github.com/boostorg/boost/releases/download/boost-$BOOST_LATEST_VERSION/$BOOST_TARBALL_BASENAME"
wget "$BOOST_TARBALL_URL"
tar -xJf "$BOOST_TARBALL_BASENAME"
rm -f "$BOOST_TARBALL_BASENAME"
mv "boost-$BOOST_LATEST_VERSION" boost
pushd boost
mkdir __build
pushd __build
cmake .. \
  '-DBOOST_INCLUDE_LIBRARIES=headers;python' \
  -DBOOST_ENABLE_PYTHON=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  '-DCMAKE_C_FLAGS=-fsanitize=address -fsanitize=undefined' \
  '-DCMAKE_CXX_FLAGS=-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address -fsanitize=undefined' \
  '-DCMAKE_SHARED_LINKER_FLAGS=-fsanitize=address -fsanitize=undefined' \
  '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address -fsanitize=undefined' \
  -DBUILD_SHARED_LIBS=ON \
  -DBOOST_INSTALL_LAYOUT=tagged \
  -DCMAKE_INSTALL_PREFIX=/home/vscode/.local
VERBOSE=1 cmake --build . -j --target install
popd
rm -rf __build
mkdir __build
pushd __build
cmake .. \
  '-DBOOST_INCLUDE_LIBRARIES=headers;python' \
  -DBOOST_ENABLE_PYTHON=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON \
  -DBOOST_INSTALL_LAYOUT=tagged \
  -DCMAKE_INSTALL_PREFIX=/home/vscode/.local
VERBOSE=1 cmake --build . -j --target install
popd
popd
rm -rf boost
popd

echo 'export C_INCLUDE_PATH="$HOME/.local/include${C_INCLUDE_PATH:+:$C_INCLUDE_PATH}"' >> ~/.bashrc
echo 'export C_INCLUDE_PATH="$HOME/.local/include${C_INCLUDE_PATH:+:$C_INCLUDE_PATH}"' >> ~/.profile
export C_INCLUDE_PATH="$HOME/.local/include${C_INCLUDE_PATH:+:$C_INCLUDE_PATH}"
echo 'export CPLUS_INCLUDE_PATH="$HOME/.local/include${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}"' >> ~/.bashrc
echo 'export CPLUS_INCLUDE_PATH="$HOME/.local/include${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}"' >> ~/.profile
export CPLUS_INCLUDE_PATH="$HOME/.local/include${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}"
echo 'export LIBRARY_PATH="$HOME/.local/lib64:$HOME/.local/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"' >> ~/.bashrc
echo 'export LIBRARY_PATH="$HOME/.local/lib64:$HOME/.local/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"' >> ~/.profile
export LIBRARY_PATH="$HOME/.local/lib64:$HOME/.local/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"
echo 'export LD_LIBRARY_PATH="$HOME/.local/lib64:$HOME/.local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH="$HOME/.local/lib64:$HOME/.local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"' >> ~/.profile
export LD_LIBRARY_PATH="$HOME/.local/lib64:$HOME/.local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Install UV Python Environment Manager.
curl -LsSf https://astral.sh/uv/install.sh | sh

# Create a virtual environment and install Mahjong Python Package.
pushd /workspaces/fuhan
rm -rf .venv
uv venv
. .venv/bin/activate
uv pip install -U mahjong
popd
