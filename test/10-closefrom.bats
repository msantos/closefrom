#!/usr/bin/env bats

@test "closefrom: verify file descriptors are closed" {
  exec 111</dev/tty

  run closefrom 110 sh -xc "[ -t 111 ]"
  cat << EOF
--- output
$output
--- output
EOF

  [ "$status" -ne 0 ]

  run closefrom 111 sh -xc "[ -t 111 ]"
  cat << EOF
--- output
$output
--- output
EOF

  [ "$status" -ne 0 ]

  run closefrom 112 sh -xc "[ -t 111 ]"
  cat << EOF
--- output
$output
--- output
EOF

  [ "$status" -eq 0 ]
}
