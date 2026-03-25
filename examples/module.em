import "system::ffi"

extern function puts(str: *i8) i32

function main(String[] args): i32 {
  puts("Hello, World!");
  return 0;
}