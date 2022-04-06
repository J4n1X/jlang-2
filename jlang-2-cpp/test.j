import "import.j"

constant testconst as integer is 10

function add(a as integer, b as integer) yields integer is
    return a plus b
done

function main() yields integer is
    define test as integer is 10
    syscall3(1, 1, "Hello, World\n", 13)
    return 69
done