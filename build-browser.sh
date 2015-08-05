BUILD_PATH=./
OUT_LIB_FILE=./lib/*
OUT_BIN_FILE=./bin/*
OUT_MAKE_FILE=./CMakeCache.txt
tmp_INSTALL_DIR=/usr/local/cuprumtest
ThirdParty_DIR=./Source/ThirdParty
Download_DIR=./Source/Download
CPU_NUM=0
BUILD_MIDORI=0
INSTALL_DEP=0

USE_32BITS=0
ARCH_64BITS="x86_64"
ARCH=$(arch)
if [ ${ARCH} == ${ARCH_64BITS} ];then
	USE_32BITS=0
	echo "Using 64 bit Linux"
else
	USE_32BITS=1
	echo "Using 32 bit Linux"
fi

if [ -z $1 ];then
	echo "Usage:"
	echo "  ./build.sh [release] [4]"
	echo "  	use 4 processor to build release version"
	echo "  ./build.sh [deb_package]"
	echo "  	 build deb_package"
	echo "  ./build.sh [clean]"
	echo "  	clean out file before build"
fi

# Read command parameters.
for arg in "$@"
do
  if [ $arg -a $arg = "--midori" ];then
    BUILD_MIDORI=1
  fi

  if [ $arg -a $arg = "--install-dependency" ];then
    INSTALL_DEP=1
  fi
done

#test cup process num
cat /proc/cpuinfo | grep "cpu cores" | while read cpu cores dot cpu_num
do
  	     echo $cpu_num >.tmp_cpu_num
done
cat .tmp_cpu_num && CPU_NUM=`cat .tmp_cpu_num` || CPU_NUM=1
CPU_NUM=$[CPU_NUM+CPU_NUM]

if [ -z $2 ];then
	echo
else
	if [ $2 -lt $CPU_NUM ]
	then
		CPU_NUM=$2
	fi
fi

if [ $INSTALL_DEP -eq 1 ];then
	# ZRL install dependencies.
	sudo apt-get install debhelper autoconf automake autopoint autotools-dev bison cmake flex gawk gnome-common gperf intltool itstool libatk1.0-dev libenchant-dev libfaad-dev libgeoclue-dev libgirepository1.0-dev libgl1-mesa-dev libgl1-mesa-glx libgnutls28-dev libcairo2-dev libgtk2.0-dev libgtk-3-dev libgudev-1.0-dev libharfbuzz-dev libicu-dev libgdk-pixbuf2.0-dev libjpeg8-dev libmpg123-dev libopus-dev libpango1.0-dev libpng12-dev libpulse-dev librsvg2-dev libsecret-1-dev libsoup2.4-dev libsqlite3-dev libtheora-dev libtool libvorbis-dev libwebp-dev libxcomposite-dev libxslt1-dev libxt-dev libxtst-dev ruby xfonts-utils git gobject-introspection icon-naming-utils libcroco3-dev libegl1-mesa-dev libp11-kit-dev libpciaccess-dev libffi-dev libxcb-xfixes0-dev libxfont-dev libxkbfile-dev llvm llvm-dev python-dev ragel x11proto-bigreqs-dev x11proto-composite-dev x11proto-gl-dev x11proto-input-dev x11proto-randr-dev x11proto-resource-dev x11proto-scrnsaver-dev x11proto-video-dev x11proto-xcmisc-dev x11proto-xf86dri-dev xfonts-utils xtrans-dev xutils-dev git-svn subversion ninja-build valac-0.20 librsvg2-bin libgcr-3-dev libnotify-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-0 libgstreamer1.0-0 libgstreamer-plugins-bad1.0-dev libgstreamer-plugins-good1.0-dev libgstreamer-plugins-good1.0-0 libgstreamer-plugins-bad1.0-0 libgstreamer-plugins-base1.0-dev libsoup-gnome2.4-dev libuchardet0 qtcreator qt-sdk libjson-c-dev
fi

pushd $BUILD_PATH
case $1 in
"clean" )
	echo "clean out file..."
		rm -rf $OUT_MAKE_FILE && echo "	clean CMakeCache.txt success "
		rm -rf $OUT_LIB_FILE && echo "	clean lib success"
		rm -rf $OUT_BIN_FILE && echo "	clean bin success"
	echo "clean end..."
	;;
"release" )

	#add by luyue
if [ ${USE_32BITS} -eq 1 ]; then
	echo "------------build 32 bits"
	cd $ThirdParty_DIR
	tar -zxvf openssl-1.0.0d.tar.gz && cd openssl-1.0.0d
	./config && make && cd ../../../
	mkdir lib 
	cp -rf $ThirdParty_DIR/openssl-1.0.0d/lib*.a ./lib

	cd $ThirdParty_DIR
	tar -zxvf uchardet.tar.gz && cd uchardet
	cmake . && make && cd ../../../
	mkdir -p lib 
	cp -rf $ThirdParty_DIR/uchardet/src/lib*.a ./lib
	
	echo "build release version start..." && sleep 3
	cmake -DUSE_32BITS=1 -DPORT=GTK -DAPP_DEBUG=ON -DDEVELOPER_MODE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_SKIP_BUILD_RPATH=FALSE -DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE -DCOMPILE_MODE=OFF -DENABLE_MIDORI=$BUILD_MIDORI && make -j${CPU_NUM} && echo ******build release SUCCESS********
else
#compile project on 64bits Linux machine
	echo "-----------build 64 bits"
	cd $ThirdParty_DIR
	tar -zxvf openssl-1.0.0d.tar.gz && cd openssl-1.0.0d
	./config shared && make && cd ../../../
	mkdir lib 
	cp -rf $ThirdParty_DIR/openssl-1.0.0d/lib*.so* ./lib
	
	cd $ThirdParty_DIR
	tar -zxvf uchardet.tar.gz && cd uchardet
	cmake . && make && cd ../
	cp -rf ./uchardet/src/lib*.so* ../../lib

	tar -zxvf uci.tar.gz && cd uci
	make
	cp -rf ./libuci.so* ../../../lib
	make clean && cd ../

	tar -zxvf aria2-1.18.9.tar.gz && cd aria2-1.18.9
	chmod +x configure && ./configure --enable-libaria2 && make
	cp -rf ./src/.libs/libaria2.so* ../../../lib
	make clean && cd ../../../

	cd ./Source && tar -zxvf Download.tar.gz && cd ../
	cd $Download_DIR
	cd ./client/wjson && make
	cd ../ && make
	cp -rf *.a ../../../lib
	make clean && cd ../

	cd ./release && make
	make clean
	cd ../../../

	echo "build release version start..." && sleep 3
	cmake -DUSE_64BITS=1 -DPORT=GTK -DAPP_DEBUG=ON -DDEVELOPER_MODE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_SKIP_BUILD_RPATH=FALSE -DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE -DCOMPILE_MODE=OFF -DENABLE_MIDORI=$BUILD_MIDORI && make -j${CPU_NUM} && echo ******build release SUCCESS********
	cp $Download_DIR/release/cdosbrowser_download ./bin
	sudo mkdir -p /usr/local/lib/cdosbrowser && sudo cp ./lib/libaria2.so.0.0.0 /usr/local/lib/cdosbrowser
	sudo cp ./lib/libuci.so.0.8 /usr/local/lib/cdosbrowser
	sudo cp ./lib/libaria2.so.0.0.0 /usr/local/lib/cdosbrowser/libaria2.so.0
	sudo mkdir -p /usr/local/libexec/cdosbrowser && sudo cp ./bin/cdosbrowser_download /usr/local/libexec/cdosbrowser
fi
	;;
"deb_package" )
	echo "build deb package"
	dpkg-buildpackage -b && echo ******build dev package SUCCESS********
	;;
"help" )
	echo "Usage:"
	echo "  ./build.sh [release] [4]"
	echo "  	use 4 processor to build release version"
	echo "  ./build.sh [deb_package]"
	echo "  	 build deb_package"
	echo "  ./build.sh [clean]"
	echo "  	clean out file before build"
;;
esac
popd
