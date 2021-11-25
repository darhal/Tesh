#!/bin/bash

set -e

echo '######### Checking AUTHORS file'
if grep -q 'NUSSBAUM Lucas' AUTHORS; then
  echo "ERROR AUTHORS file was not modified (not fatal for now)."
fi

echo '######### Checking rapport.pdf file'
if [ ! -f rapport.pdf ]; then
  echo "ERROR rapport.pdf does not exist (not fatal for now)."
fi

echo '######### Installing dependencies'
apt-get update
apt-get -y install --no-install-recommends build-essential libreadline-dev electric-fence sudo

echo '######### Trying to build'
rm -f tesh # in case it was committed
make || (echo ERROR: make failed; exit 1)
if [ ! -f tesh ]; then
  echo ERROR 'make' did not generate a 'tesh' file
  exit 1
fi

if ldd tesh | grep -q libasan; then
  echo "ERROR 'tesh' is linked with libasan. Did you build with -fsanitize?"
  exit 1
fi

ti=`mktemp /tmp/tesh_test.input.XXXXXX`
to=`mktemp /tmp/tesh_test.ouput.XXXXXX`
te=`mktemp /tmp/tesh_test.correct.XXXXXX`
currentdir=$(pwd)
Red='\033[0;31m'          # Red
Green='\033[0;32m'        # Green
Color_Off='\033[0m'       # Text Reset
test_nb=0
test_passed=0
test_failed=0

display() {
echo Input:
echo ------------------------------------------------
cat $ti
echo ------------------------------------------------
echo Output:
echo ------------------------------------------------
cat $to
echo ------------------------------------------------
echo Expected output:
echo ------------------------------------------------
cat $te
echo ------------------------------------------------
mo=`md10sum < $to | awk '{print $1}'`
me=`md10sum < $te | awk '{print $1}'`
test_nb=$((test_nb+1))
if ! [ "$mo" = "$me" ]; then
  test_failed=$((test_failed+1))
  printf "${Red}==> Test#${test_nb} : ERROR outputs do not match.${Color_Off}\n"
  # exit 1
else
  test_passed=$((test_passed+1))
  printf "${Green}==> Test#${test_nb} : passed successfully.${Color_Off}\n"
fi
}

# enable Electric Fence
export LD_PRELOAD=libefence.so.0.0
export EF_DISABLE_BANNER=1
export EF_ALLOW_MALLOC_0=1


echo '######### Trying to test tesh with a basic script (two commands)'
cat <<-EOF > $ti 2>&1
echo foo
echo bar baz
echo foo bar baz
echo foo bar baz
echo bar
echo 1
echo 2
echo 3
echo 4
./tests/fixtures/echo 5
echo 6
EOF
timeout 5s bash -c "./tesh < $ti" > $to 2>&1
timeout 10s bash -c "`cat $ti`" > $te 2>&1
display $ti $to $te

echo '######### Trying to test tesh with a more advanced script (commands chaining)'
cat <<-EOF > $ti 2>&1
false ; echo A
true ; echo B
false && echo a
true && echo b
false || echo c
true || echo d
echo foo ; echo bar
false && echo a
true && echo b
false || echo c
true || echo d
./tests/fixtures/ret1 && echo a
./tests/fixtures/ret0 && echo b
./tests/fixtures/ret1 || echo c
./tests/fixtures/ret0 || echo d
false && echo a || echo b || echo c && echo d || echo e && echo f && false || echo g ; false && echo a || echo b || echo c && echo d || echo e && echo f && false || echo g
cd ./tests/fixtures
cd d2
cd d2.1
pwd
ls
EOF
timeout 5s bash -c "./tesh < $ti" > $to 2>&1
timeout 10s bash -c "`cat $ti`" > $te 2>&1
display $ti $to $te

echo '######### Trying to test tesh with a more advanced script (input/output redirections)'
cat <<-EOF > $ti 2>&1
echo foo > /tmp/toto
echo bar > /tmp/toto
echo baz >> /tmp/toto
cat < /tmp/toto
cat ./tests/fixtures/redir/lorem-ipsum > /tmp/TMPFILE
echo sortie de echo
cat /tmp/TMPFILE
cat < ./tests/fixtures/redir/lorem-ipsum
cat ./tests/fixtures/redir/lorem-ipsum > /tmp/TMPFILE
cat ./tests/fixtures/redir/lorem-ipsum > /tmp/TMPFILE
cat ./tests/fixtures/redir/lorem-ipsum >> /tmp/TMPFILE
echo sortie de echo
cat /tmp/TMPFILE
EOF
timeout 5s bash -c "./tesh < $ti" > $to 2>&1
timeout 10s bash -c "`cat $ti`" > $te 2>&1
display $ti $to $te

echo '######### Trying to test tesh with a more advanced script 2 (Pipes)'
cat <<-EOF > $ti 2>&1
cat ./tests/fixtures/redir/lorem-ipsum-long | grep Duis
cat ./tests/fixtures/redir/lorem-ipsum-long | grep am | grep Ut > /tmp/TMPFILE ; cat ./tests/fixtures/redir/lorem-ipsum-long | grep Lorem >> /tmp/TMPFILE ; tac < /tmp/TMPFILE
cat ./tests/fixtures/redir/lorem-ipsum-very-long | md10sum
EOF
timeout 5s bash -c "./tesh < $ti" > $to 2>&1
timeout 10s bash -c "`cat $ti`" > $te 2>&1
display $ti $to $te

echo '######### Test du comportement de tesh -e'
cat <<-EOF > $ti 2>&1
echo a
echo b
false
echo c
EOF
timeout 5s bash -c "./tesh < $ti" > $to 2>&1
timeout 10s bash -c "`cat $ti`" > $te 2>&1
display $ti $to $te

echo '######### Test du comportement de tesh avec root ()'
cat <<-EOF > $ti 2>&1
echo foo
echo bar
EOF
timeout 5s bash -c "./tesh < $ti" > $to 2>&1
timeout 10s bash -c "`cat $ti`" > $te 2>&1
display $ti $to $te

echo '######### Test du comportement des script t1'
cat <<-EOF > $ti 2>&1
Testing with script t1
EOF
timeout 5s bash -c "./tesh ./tests/fixtures/scripts/t1" > $to 2>&1
timeout 10s bash -c "bash -c ./tests/fixtures/scripts/t1" > $te 2>&1
display $ti $to $te

echo '######### Test du comportement des script t2'
cat <<-EOF > $ti 2>&1
Testing with script t2
EOF
timeout 5s bash -c "./tesh ./tests/fixtures/scripts/t2" > $to 2>&1
timeout 10s bash -c "bash -c ./tests/fixtures/scripts/t2" > $te 2>&1
display $ti $to $te

echo '######### Test du comportement des script t3'
cat <<-EOF > $ti 2>&1
Testing with script t3
EOF
timeout 5s bash -c "./tesh ./tests/fixtures/scripts/t3" > $to 2>&1
timeout 10s bash -c "bash -c ./tests/fixtures/scripts/t3" > $te 2>&1
display $ti $to $te

echo '######### Test du comportement des script t1 (with -e)'
cat <<-EOF > $ti 2>&1
Testing with script t1
EOF
timeout 5s bash -c "./tesh -e ./tests/fixtures/scripts/t1" > $to 2>&1
timeout 10s bash -c "bash -e ./tests/fixtures/scripts/t1" > $te 2>&1
display $ti $to $te

echo '######### Test du comportement des script t2 (with -e)'
cat <<-EOF > $ti 2>&1
Testing with script t2
EOF
cat <<-EOF > $te 2>&1
a
b
EOF
timeout 5s bash -c "./tesh -e ./tests/fixtures/scripts/t2" > $to 2>&1
display $ti $to $te

echo '######### Test du comportement des script t3 (with -e)'
cat <<-EOF > $ti 2>&1
Testing with script t3
EOF
cat <<-EOF > $te 2>&1
      2      15     104
      2      15     104
EOF
timeout 5s bash -c "./tesh -e ./tests/fixtures/scripts/t3" > $to 2>&1
display $ti $to $te

# disable Electric Fence
export LD_PRELOAD=

echo '######### Test du comportement de tesh avec bg 1'
cat <<-EOF > $ti 2>&1
true &
./tests/fixtures/sleeper 0.2
echo bar
fg
EOF
cat <<-EOF > $te 2>&1
[2]
bar
[2->0]
EOF
(sudo unshare -p -f --mount-proc ./tesh < $ti > $to 2>&1)
display $ti $to $te

echo '######### Test du comportement de tesh avec bg 2'
cat <<-EOF > $ti 2>&1
true &
false &
./tests/fixtures/sleeper 0.2
fg 2
fg 3
EOF
cat <<-EOF > $te 2>&1
[2]
[3]
[2->0]
[3->1]
EOF
(sudo unshare -fp --mount-proc ./tesh < $ti > $to 2>&1)
display $ti $to $te

echo '######### Test du comportement de tesh avec bg 3'
cat <<-EOF > $ti 2>&1
./tests/fixtures/sleeper 0.5 echo bar &
echo foo
fg
EOF
cat <<-EOF > $te 2>&1
[2]
foo
bar
[2->0]
EOF
(sudo unshare -p -f --mount-proc ./tesh < $ti > $to 2>&1)
display $ti $to $te


if [ $test_passed -eq $test_nb ] ; then
  printf "${Green}*** Test suite finished : all test cases ${test_passed}/${test_nb} passed ***${Color_Off}\n"
elif [ $test_passed -gt $test_failed ] ; then
  printf "${Green}*** Test suite finished : ${test_passed}/${test_nb} passed and ${Red}${test_failed}/${test_nb}${Green} failed ***${Color_Off}\n"
  exit 1
else
  printf "${Red}*** Test suite finished : ${Green}${test_passed}/${test_nb}${Red} passed and ${test_failed}/${test_nb} failed ***${Color_Off}\n"
  exit 1
fi
