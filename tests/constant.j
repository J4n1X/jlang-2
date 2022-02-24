import "std/std.j"


; Structure with a key string(constant) and 256 bytes of data
constant Pair_key as integer is 0
constant Pair_value as integer is 8
constant Pair_VALUE_SIZE as integer is 256
constant Pair_SIZE as integer is Pair_VALUE_SIZE plus INTEGER_SIZE multiply 2 

function Pair_new(dest as pointer, key as pointer, value as pointer, value_len as integer) yields none is
    if value_len greater-equal Pair_SIZE do
        eputs("[ERROR] Pair.new: value_len greater-equal Pair_VALUE_SIZE\n")
        exit(1)
    done
    store64(ptr_plus(dest, Pair_key), key)
    drop memcpy(ptr_plus(dest, Pair_value), value, value_len)
done

function Pair_print(pair as pointer) yields none is
    ;print(ptr_plus(pair, Pair_key))
    ;print(ptr_plus(pair, Pair_value))
    puts("[Pair] key: ")
    puts(load64(ptr_plus(pair, Pair_key)))
    puts("\n[Pair] value: ")
    puts(ptr_plus(pair, Pair_value))
    puts("\n")
done

constant hellomsg as pointer is "Hello, World!\n"
function main() yields none is
    define pair_1 as pointer is allocate(Pair_SIZE)
    Pair_new(pair_1, "key1", "value1", 7)
    Pair_print(pair_1)
    return none
done