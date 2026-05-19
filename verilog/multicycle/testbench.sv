`timescale 1ns / 1ps

module testbench;
	logic w_clk;
	logic w_rst;
	
	// Wishbone wires
	logic        w_ack;
	logic        w_stall;
	logic [7:0]  w_dat_i;
	logic        w_stb;
	logic        w_cyc;
	logic        w_we;
	logic        w_lock;
	logic        w_tgc;
	logic [7:0]  w_dat_o;
	logic [15:0] w_adr;

	// Interrupt wires
	logic        w_interrupt;
	logic [6:0]  w_interrupt_id;
	logic        w_interrupt_ack;

	// Instantiate Multicycle CPU
	cpu u_cpu (
		.clk_i(w_clk),
		.rst_i(w_rst),
		.ack_i(w_ack),
		.dat_i(w_dat_i),
		.stb_o(w_stb),
		.cyc_o(w_cyc),
		.we_o(w_we),
		.lock_o(w_lock),
		.tgc_o(w_tgc),
		.dat_o(w_dat_o),
		.adr_o(w_adr),

		.i_int(w_interrupt),
		.i_int_id(w_interrupt_id),
		.o_int_ack(w_interrupt_ack)
	);

	// Instantiate Wishbone RAM Slave
	wb_ram_slave u_ram (
		.clk_i(w_clk),
		.cyc_i(w_cyc),
		.stb_i(w_stb),
		.we_i(w_we),
		.dat_i(w_dat_o),
		.adr_i(w_adr),
		.ack_o(w_ack),
		.dat_o(w_dat_i)
	);

	// Clock generation
	initial begin
		w_clk = 0;
		forever #1 w_clk = ~w_clk;
	end

	// Log memory writes
	always @(posedge w_clk) begin
		if (w_cyc && w_stb && w_we && !w_stall) begin
			$display("[%0t] MEM WRITE: [%04X] = %02X", $time, w_adr, w_dat_o);
		end
	end

	// Main simulation block
	initial begin
		logic interrupt_triggered;
		$display("[%0t] Starting Multicycle CPU Simulation...", $time);

		// Initialize inputs
		interrupt_triggered = 0;
		w_interrupt = 0;
		w_interrupt_id = 0;

		// Load program
		$readmemh("mem.hex", u_ram.ram);

		// Reset sequence (active high)
		w_rst = 1;
		#4 w_rst = 0;
		$display("[%0t] Reset Released", $time);

		// Simulation loop
		// We use 800 cycles to allow multicycle instructions (several cycles each) to run
		for (int i = 0; i < 800; i++) begin
			@(posedge w_clk);
			#0.1; // Small delay to allow signals to settle for display
			
			$display("[%0t] Stage: %0d | PC: %04X | Instr Reg: %04X | R0: %02X | R1: %02X | R2: %02X | R3: %02X | R4: %02X | R5: %02X | R6: %02X | R7: %02X | SP: %04X | I: %b | V: %b | N: %b | Z: %b | C: %b | WB: cyc=%b stb=%b ack=%b adr=%04X dat_i=%02X dat_o=%02X req=%b", 
				$time, 
				u_cpu.cu.r_stage, 
				{u_cpu.r_pc, 1'b0}, 
				u_cpu.r_instruction, 
				u_cpu.rf.r_gpr[0], 
				u_cpu.rf.r_gpr[1], 
				u_cpu.rf.r_gpr[2],
				u_cpu.rf.r_gpr[3],
				u_cpu.rf.r_gpr[4],
				u_cpu.rf.r_gpr[5],
				u_cpu.rf.r_gpr[6],
				u_cpu.rf.r_gpr[7],
				u_cpu.cf.o_sp,
				u_cpu.cf.o_flags[4],
				u_cpu.cf.o_flags[3],
				u_cpu.cf.o_flags[2],
				u_cpu.cf.o_flags[1],
				u_cpu.cf.o_flags[0],
				w_cyc, w_stb, w_ack, w_adr, w_dat_i, w_dat_o, u_cpu.wb.i_req);

			// Trigger an external interrupt when WFI is reached and some time has passed
			if (u_cpu.cu.r_wfi && !interrupt_triggered && $time >= 150) begin
				$display("[%0t] WFI detected. Triggering external interrupt...", $time);
				w_interrupt = 1;
				w_interrupt_id = 7'h01; // Breakpoint
				interrupt_triggered = 1;
			end

			// Clear interrupt on ack
			if (w_interrupt_ack) begin
				$display("[%0t] Interrupt Acknowledged.", $time);
				w_interrupt = 0;
			end

			// Check for Halt (Branch to self or infinite loop)
			if (u_cpu.r_instruction == 16'hFFFE) begin
				$display("[%0t] Halt detected (infinite loop). Simulation finished.", $time);
				$finish;
			end

			if (u_cpu.cu.w_illegal_instruction) begin
				$display("[%0t] Illegal instruction detected (%04X).", $time, u_cpu.r_instruction);
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
