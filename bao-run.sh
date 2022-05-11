#!/bin/bash

function run_test() {
  # ./$1 $2
  taskset -ac 0-`expr $2 - 1` ./$1 $2
}

echo "test spin lock using cmpxchg"
run_test "test-spinlock-cmpxchg" $1

echo "test spin lock using xchg"
run_test "test-spinlock-xchg" $1

echo "test spin lock using k42"
run_test "test-spinlock-k42" $1

echo "test spin lock using mcs"
run_test "test-spinlock-mcs" $1

echo "test spin lock using ticket"
run_test "test-spinlock-ticket" $1

echo "test spin lock using pthread"
run_test "test-spinlock-pthread" $1

echo "test spin lock using xchg-backoff"
run_test "test-spinlock-xchg-backoff" $1

