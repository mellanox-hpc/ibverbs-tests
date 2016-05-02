#!/bin/bash -eExl

rc=0

if [ -z "$BUILD_NUMBER" ]; then
    echo Running interactive
    WORKSPACE=$PWD
else
    echo Running under jenkins
fi

make_opt="-j$(($(nproc) - 1))"
inst=${WORKSPACE}/install
ibv_test=$inst/bin/ibv_test

echo Starting on host: $(hostname)

echo "Autogen"
./autogen.sh

echo "Build release"
./configure --prefix=$inst LDFLAGS=-L$inst/lib CPPFLAGS=-I$inst/include
make $make_opt install

# Set CPU affinity to 2 cores, for performance tests
if [ -n "$EXECUTOR_NUMBER" ]; then
    AFFINITY="taskset -c $(( 2 * EXECUTOR_NUMBER ))","$(( 2 * EXECUTOR_NUMBER + 1))"
    TIMEOUT="timeout 40m"
else
    AFFINITY=""
    TIMEOUT=""
fi


VALGRIND_ARGS="--show-reachable=yes --xml=yes --gen-suppressions=all --tool=memcheck --leak-check=full --track-origins=yes --fair-sched=try"

for dev in $(ibstat -l); do
    env IBV_TEST_DEV=${dev} $AFFINITY $ibv_test
    module load tools/valgrind
    env IBV_TEST_DEV=${dev} $AFFINITY valgrind $VALGRIND_ARGS --xml-file=$WORKSPACE/${dev}_valgrind.xml --log-file=$WORKSPACE/${dev}_valgrind.txt $ibv_test
    module unload tools/valgrind
done

exit $rc
