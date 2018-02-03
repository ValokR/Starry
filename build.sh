
#!/bin/sh

if [ $# -ne 1 ]; then
  echo "$0 <debug|release>"
  exit 1
fi
CONFIG=$1
if [ ${CONFIG} != debug -a ${CONFIG} != release ]; then
  echo "$0 <debug|release>"
  exit 1
fi
PROJECT=$(basename $(pwd))
if [ ! -e cmake-build-${CONFIG} ]; then
  mkdir cmake-build-${CONFIG}
  cd cmake-build-${CONFIG}
  ${HOME}/clion-2016.3/bin/cmake/bin/cmake -DCMAKE_BUILD_TYPE=$(echo ${CONFIG} | perl -ane 'chomp;print ucfirst($_);') ..
  cd ..
fi
${HOME}/clion-2016.3/bin/cmake/bin/cmake --build cmake-build-${CONFIG} --target ${PROJECT} -- -j 2
