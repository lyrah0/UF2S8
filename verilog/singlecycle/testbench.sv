`timescale 1ns / 1ps

module testbench;
	logic w_clk;
	logic w_rst_n;
	logic [15:0] w_pc;
	logic [15:0] w_instr;
	logic [15:0] w_mem_addr;
	logic [7:0]  w_mem_wdata;
	logic [7:0]  w_mem_rdata;
	logic        w_mem_we;

	// 64KB byte-addressed Memory
	logic [7:0] r_memory [0:65535];

	// Instantiate CPU
	cpu u_cpu (
	.i_clk(w_clk),
	.i_rst_n(w_rst_n),
	.o_pc(w_pc),
	.i_instr(w_instr),
	.o_mem_addr(w_mem_addr),
	.o_mem_wdata(w_mem_wdata),
	.i_mem_rdata(w_mem_rdata),
	.o_mem_we(w_mem_we)
	);

	// Clock generation
	initial begin
		w_clk = 0;
		forever #1 w_clk = ~w_clk;
	end

	// Instruction memory read
	assign w_instr = {r_memory[w_pc+1], r_memory[w_pc]};

	// Data memory read
	assign w_mem_rdata = r_memory[w_mem_addr];

	// Data memory write
	always @(posedge w_clk) begin
		if (w_mem_we) begin
			r_memory[w_mem_addr] <= w_mem_wdata;
			$display("[%0t] MEM WRITE: [%04X] = %02X", $time, w_mem_addr, w_mem_wdata);
		end
	end

	// Main simulation block
	initial begin
		$display("[%0t] Starting Simulation...", $time);
	
		// Initialize memory
		for (int i = 0; i < 65536; i++) r_memory[i] = 0;
	
		// Load program
		$readmemh("mem.hex", r_memory);

		// Reset sequence
		w_rst_n = 0;
		#2 w_rst_n = 1;
		$display("[%0t] Reset Released", $time);

		// Simulation loop
		for (int i = 0; i < 100; i++) begin
			@(posedge w_clk);
			#1; // Small delay to allow signals to settle for display
			$display("[%0t] PC: %04X | Instr: %04X | R0: %02X | R1: %02X | R2: %02X | R3: %02X | R4: %02X | R5: %02X | R6: %02X | R7: %02X | SP: %04X | I: %b | V: %b | N: %b | Z: %b | C: %b", 
				$time, w_pc, w_instr, 
				u_cpu.u_register_file.r_gpr[0], 
				u_cpu.u_register_file.r_gpr[1], 
				u_cpu.u_register_file.r_gpr[2],
				u_cpu.u_register_file.r_gpr[3],
				u_cpu.u_register_file.r_gpr[4],
				u_cpu.u_register_file.r_gpr[5],
				u_cpu.u_register_file.r_gpr[6],
				u_cpu.u_register_file.r_gpr[7],
				u_cpu.w_cf_o_sp,
				u_cpu.w_cf_flags[7],
				u_cpu.w_cf_flags[3],
				u_cpu.w_cf_flags[2],
				u_cpu.w_cf_flags[1],
				u_cpu.w_cf_flags[0]);
	    
		// Check for Halt (Branch to self)
		if (w_instr == 16'hFFFE) begin
			$display("[%0t] Halt detected (infinite loop). Simulation finished.", $time);
			$finish;
		end
	end

	$display("[%0t] Simulation timeout.", $time);
	$finish;
	end

	initial begin
		$dumpfile("cpu_sim.vcd");
		$dumpvars(0, testbench);
	end

endmodule
