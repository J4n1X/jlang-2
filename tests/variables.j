; global variables
assign 0 to var1.
assign 1 to var2.
assign 0 to var3.

assign 15 to reps.

function main is
    print var3.
    assign var1 plus var2 to var3.
    assign var2 to var1.
    assign var3 to var2.
    assign reps minus 1 to reps.
    if reps greater 0 do
        call main.
    done.
done.


;assign 0 to var1.
;
;function main is
;    assign var1 plus 1 to var1.
;    print var1.
;done