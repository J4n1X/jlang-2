define obo_test as pointer is "Testing one-by-one string printing!\n"

define strlen_test as pointer is "Testing the strlen function!\n"

define replace_test as pointer is "Some day, there will be a total of 6 tests\n"
define replace_test_seg1_len as integer is 36
define replace_test_seg2_len as integer is 7
define replace_char as integer is 56 ; ASCII value for '9'


define replace_failed_msg as pointer is "Replacing a character in a string failed as the match character could not be found.\n"

function strlen(str as pointer) yields integer is 
    define len as integer is 0
    while load8(str plus len) not-equal 0 do
        len is len plus 1
    done
    return len
done

function print_one_by_one(str as pointer) yields none is
    define str_loc as integer is 0
    define str_char as integer is load8(str plus str_loc)
    while str_char not-equal 0 do
        drop syscall3(1, 1, address-of(str_char), 1)
        str_loc is str_loc plus 1
        str_char is load8(str plus str_loc)
    done
done

function replace_value_at_pointer(dest as pointer, value as integer) yields none is
    store8(dest, value)
    drop syscall3(1, 1, replace_test, replace_test_seg1_len)
    drop syscall3(1, 1, dest, 1) ; this may have to be changed
    drop syscall3(1, 1, replace_test plus replace_test_seg1_len, replace_test_seg2_len)
done

function main() yields integer is 
    print_one_by_one(obo_test)
    drop syscall3(1, 1, strlen_test, strlen(strlen_test))
    replace_value_at_pointer(address-of(replace_char), 57)
    return 0
done