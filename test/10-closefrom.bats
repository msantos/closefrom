#!/usr/bin/env bats

@test "closefrom: verify file descriptors are closed" {
  exec 111</dev/null

  run closefrom 110 ls -al /proc/self/fd/111
  cat << EOF
--- output
$output
--- output
EOF

  [ "$status" -ne 0 ]

  run closefrom 111 ls -al /proc/self/fd/111
  cat << EOF
--- output
$output
--- output
EOF

  [ "$status" -ne 0 ]

  run closefrom 112 ls -al /proc/self/fd/111
  cat << EOF
--- output
$output
--- output
EOF

  [ "$status" -eq 0 ]
}
