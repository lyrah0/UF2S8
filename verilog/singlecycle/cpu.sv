module cpu (
    input  logic        i_clk,
    input  logic        i_rst_n,
    // Instruction Memory Interface
    output logic [15:0] o_pc,
    input  logic [15:0] i_instr,
    // Data Memory Interface
    output logic [15:0] o_mem_addr,
    output logic [7:0]  o_mem_wdata,
    input  logic [7:0]  i_mem_rdata,
    output logic        o_mem_we
);

    // Registers
    logic [14:0] r_pc;      // Program Counter
    
endmodule