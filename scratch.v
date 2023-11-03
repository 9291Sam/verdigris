module register(
    input[2:0] a,
    input clock,
    input reset,
    input write_enable, // also sometimes just called enable
    output reg[2:0] output
);


always @ (posedge clock or negedge reset)
begin

    if (reset == 1'b0)
        output <= 3'b000
    else
        if (enable == 1'b1) // if enable is set we update the value contained otherwise it just stays the same
            output <= a;
        // else keep whatever was already there
        // in the else case read from the flip flop and whatever is already there is what goes back
        
end


endmodule