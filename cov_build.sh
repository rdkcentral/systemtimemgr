##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2024 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

WORKDIR=`pwd`

apt-get update
apt-get install -y libjsonrpccpp-dev

cd $WORKDIR/systimerfactory
autoreconf -i
export CXXFLAGS="-I../interface/ "
./configure --prefix=${RDKLOGGER_INSTALL_DIR}
make clean && make && make install

cd $WORKDIR
export INSTALL_DIR='/usr/local'
export top_srcdir=`pwd`
export top_builddir=`pwd`

autoreconf --install
export CXXFLAGS="-I./interface/ -I./systimerfactory/"
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
export LDFLAGS="-L/usr/local/lib -lpthread  -lsystimerfactory -lrdkloggers -lsecure_wrapper"

./configure --prefix=${INSTALL_DIR} && make && make install
