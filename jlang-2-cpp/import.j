constant importedvariable as integer is 69

function puts(string as pointer, len as integer) yields none is
    drop syscall3(1, 1, string, len)
done