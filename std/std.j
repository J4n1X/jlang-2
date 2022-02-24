constant POINTER_SIZE as integer is 8
constant INTEGER_SIZE as integer is 8

constant STDIN as integer is 0
constant STDOUT as integer is 1
constant STDERR as integer is 2

constant SYS_read as integer is 0
constant SYS_write as integer is 1
constant SYS_exit as integer is 60

function ptr_plus(ptr as pointer, offset as integer) yields pointer is
    return ptr plus pointer(offset)
done

; returns dest if successful, 0 if not
function memcpy(dest as pointer, src as pointer, size as integer) yields pointer is
    if dest equal 0 do return 0 done
    if src equal 0 do return 0 done
    if size equal 0 do return dest done

    define i as integer is 0
    while i less size do
        store8(dest plus pointer(i), load8(src plus pointer(i)))
        i is i plus 1
    done
    return dest
done

function memset(dest as pointer, value as integer, size as integer) yields pointer is
    if dest equal 0 do return 0 done
    if size equal 0 do return dest done

    define i as integer is 0
    while i less size do
        store8(dest plus pointer(i), value)
        i is i plus 1
    done
    return dest
done

constant newline as integer is 10
function strlen(str as pointer) yields integer is
    if str equal 0 do return 0 done

    define i as integer is 0
    while load8(str plus pointer(i)) not-equal 0 do
        ;drop syscall3(SYS_write, STDOUT, str plus pointer(i), 1)
        ;drop syscall3(SYS_write, STDOUT, address-of(newline), 1)
        i is i plus 1
    done
    return i
done

function fputs(fd as integer, str as pointer) yields none is
    if str equal 0 do return none done
    drop syscall3(SYS_write, fd, str, strlen(str))
done

function puts(str as pointer) yields none is
    fputs(STDOUT, str)
done

function eputs(str as pointer) yields none is
    fputs(STDERR, str)
done

function exit(status as integer) yields none is
    syscall1(SYS_exit, status)
done

