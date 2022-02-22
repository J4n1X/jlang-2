define obo_test as pointer is "Testing one-by-one string printing!\n"
define strlen_test as pointer is "Testing the strlen function!\n"

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

function main() yields integer is 
    print_one_by_one(obo_test)
    drop syscall3(1, 1, strlen_test, strlen(strlen_test))
    return 0
done