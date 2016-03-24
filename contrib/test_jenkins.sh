#!/bin/bash -eExl

rc=0

if [ -z "$BUILD_NUMBER" ]; then
    echo Running interactive
    WORKSPACE=$PWD
    BUILD_NUMBER=1
    WS_URL=file://$WORKSPACE
    JENKINS_RUN_TESTS=yes
else
    echo Running under jenkins
    WS_URL=$JOB_URL/ws
fi

make_opt="-j$(($(nproc) - 1))"
inst=${WORKSPACE}/install
ibv_test=$inst/bin/ibv_test

echo Starting on host: $(hostname)

export CPPFLAGS=-I$inst/include LDFLAGS=-L$inst/lib

echo Upstream libraries

for lib in libibverbs libmlx5 ; do
    rm -rf $lib
    # /.autodirect/mswg/git/mlnx_ofed/$lib.git
    # git clone ssh://$(whoami)@l-gerrit.mtl.labs.mlnx:29418/upstream/$lib
    git init $lib
    ( cd $lib
    git fetch ssh://l-gerrit.mtl.labs.mlnx:29418/upstream/$lib $ref
    git checkout FETCH_HEAD
    ./autogen.sh
    ./configure --prefix=$inst
    make $make_opt install )
done

echo Autogen
./autogen.sh

echo Build release
./configure --prefix=$inst
make $make_opt install

if [ -n "$JENKINS_RUN_TESTS" ]; then
    # Set CPU affinity to 2 cores, for performance tests
    if [ -n "$EXECUTOR_NUMBER" ]; then
        AFFINITY="taskset -c $(( 2 * EXECUTOR_NUMBER ))","$(( 2 * EXECUTOR_NUMBER + 1))"
        TIMEOUT="timeout 40m"
    else
        AFFINITY=""
        TIMEOUT=""
    fi


    VALGRIND_ARGS="--show-reachable=yes --xml=yes --gen-suppressions=all --tool=memcheck --leak-check=full --track-origins=yes --fair-sched=try"

    for dev in $($inst/bin/ibv_devinfo -l|grep -v found:); do
        env IBV_TEST_DEV=${dev} $AFFINITY $ibv_test
        #module load tools/valgrind
        #env IBV_TEST_DEV=${dev} $AFFINITY valgrind $VALGRIND_ARGS --xml-file=$WORKSPACE/${dev}_valgrind.xml --log-file=$WORKSPACE/${dev}_valgrind.txt $ibv_test
        #module unload tools/valgrind
    done


fi


exit $rc
