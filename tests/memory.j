define copy_done as pointer is "Copy completed!\n"
define test_str as pointer is "Hello\n"
define memory_global as pointer is allocate(7)

function strlen(str as pointer) yields integer is
    define str_len as integer is 0
    ; get size of string
    while load8(pointer(str plus str_len)) not-equal 0 do
        str_len is str_len plus 1
    done
    return str_len
done

function puts(str as pointer) yields none is
    drop syscall3(1, 1, str, strlen(str))
done

function memcopy(dest as pointer, src as pointer, len as integer) yields none is
    define i as integer is 0
    while i less len do 
        store8(pointer(dest plus i), load8(pointer(src plus i)))
        i is i plus 1
    done
done

function memset(dest as pointer, len as integer, val as integer) yields none is
    define i as integer is 0
    while i less len do
        store8(pointer(dest plus i), val)
        i is i plus 1
    done
done

function main() yields none is 
    define memory_local as pointer is allocate(8)
    define large_memory as pointer is allocate(256)
    store64(memory_local, 46116884)
    print(load64(memory_local))
    memcopy(memory_global, test_str, strlen(test_str))
    puts(memory_global)
    memset(large_memory, 256, 48)
    store8(pointer(large_memory plus 254), 10)
    store8(pointer(large_memory plus 255), 0)
    puts(large_memory)
done