cmake_minimum_required(VERSION 3.4)
project(test)

set(list1 1 2 3)
set(list2 4 5)

set(item1 a)
set(item2 b)

# in lists: cannot contain $
message("=== in list ===")
foreach(v IN LISTS list1 list2)
    message(${v})
endforeach()

# in items: must contain $
message("=== in items ===")
foreach(v IN ITEMS ${item1} ${item2})
    message(${v})
endforeach()

# default (same as in items): can mix items and lists
message("=== default ===")
foreach(v sss ${item1} ${list1} ${item2} eee)
    message(${v})
endforeach()
