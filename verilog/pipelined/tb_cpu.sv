`timescale 1ns / 1ps

module tb_cpu;
	logic		clk;
	logic		rst;

	// Memory interface wires
	logic		imem_stall;
	logic [15:0]	imem_dat;
	logic [14:0]	imem_adr;
	logic		dmem_stall;
	logic [7:0]	dmem_rdat;
	logic		dmem_re;
	logic		dmem_we;
	logic [7:0]	dmem_wdat;
	logic [15:0]	dmem_adr;

	// Interrupt interface
	logic		i_int;
	logic [6:0]	i_int_id;
	logic		o_int_ack;

	// File loading
	int		fd;
	int		bytes_read;


	// debug view wires
	logic [7:0] w_gpr0;
	logic [7:0] w_gpr1;
	logic [7:0] w_gpr2;
	logic [7:0] w_gpr3;
	logic [7:0] w_gpr4;
	logic [7:0] w_gpr5;
	logic [7:0] w_gpr6;
	logic [7:0] w_gpr7;
	logic [7:0] w_csr0;
	logic [7:0] w_csr1;
	logic [7:0] w_csr2;
	logic [7:0] w_csr3;
	logic [7:0] w_csr4;
	logic [7:0] w_csr5;
	logic [7:0] w_csr6;
	logic [7:0] w_csr7;

	always_comb begin
		w_gpr0 = u_cpu.u_id.u_rf.r_gpr[0];
		w_gpr1 = u_cpu.u_id.u_rf.r_gpr[1];
		w_gpr2 = u_cpu.u_id.u_rf.r_gpr[2];
		w_gpr3 = u_cpu.u_id.u_rf.r_gpr[3];
		w_gpr4 = u_cpu.u_id.u_rf.r_gpr[4];
		w_gpr5 = u_cpu.u_id.u_rf.r_gpr[5];
		w_gpr6 = u_cpu.u_id.u_rf.r_gpr[6];
		w_gpr7 = u_cpu.u_id.u_rf.r_gpr[7];
		w_csr0 = u_cpu.u_id.u_cf.r_reg[0];
		w_csr1 = u_cpu.u_id.u_cf.r_reg[1];
		w_csr2 = u_cpu.u_id.u_cf.r_reg[2];
		w_csr3 = u_cpu.u_id.u_cf.r_reg[3];
		w_csr4 = u_cpu.u_id.u_cf.r_reg[4];
		w_csr5 = u_cpu.u_id.u_cf.r_reg[5];
		w_csr6 = u_cpu.u_id.u_cf.r_reg[6];
		w_csr7 = u_cpu.u_id.u_cf.r_reg[7];
	end

	// Instantiate CPU
	cpu u_cpu (
		.clk_i(clk),
		.rst_i(rst),

		.imem_stall_i(imem_stall),
		.imem_dat_i(imem_dat),
		.imem_adr_o(imem_adr),

		.dmem_stall_i(dmem_stall),
		.dmem_dat_i(dmem_rdat),
		.dmem_re_o(dmem_re),
		.dmem_we_o(dmem_we),
		.dmem_dat_o(dmem_wdat),
		.dmem_adr_o(dmem_adr),

		.i_int(i_int),
		.i_int_id(i_int_id),
		.o_int_ack(o_int_ack)
	);

	// Instantiate Unified RAM
	ram u_ram (
		.i_clk(clk),

		.i_imem_adr(imem_adr),
		.o_imem_dat(imem_dat),
		.o_imem_stall(imem_stall),

		.i_dmem_adr(dmem_adr),
		.i_dmem_dat(dmem_wdat),
		.i_dmem_re(dmem_re),
		.i_dmem_we(dmem_we),
		.o_dmem_dat(dmem_rdat),
		.o_dmem_stall(dmem_stall)
	);

	// Clock Generation (10ns period / 100MHz)
	always #5 clk = ~clk;

	// Log memory writes
	always @(posedge clk) begin
		if (dmem_we) begin
			$display("[%0t] MEM WRITE: [%04X] = %02X", $time, dmem_adr, dmem_wdat);
		end
		if (dmem_re) begin
			$display("[%0t] MEM READ: [%04X] = %02X", $time, dmem_adr, dmem_rdat);
		end
	end

	initial begin
		$dumpfile("tb_cpu.vcd");
		$dumpvars(0, tb_cpu);

		clk = 0;
		rst = 1;
		i_int = 0;
		i_int_id = 0;

		// Clear RAM
		for (int i = 0; i < 65536; i++) begin
			u_ram.mem[i] = 8'h00;
		end

		// Load binary into memory
		fd = $fopen("test.bin", "rb");
		if (fd == 0) begin
			$display("Error: Could not open test.bin");
			$finish;
		end
		bytes_read = $fread(u_ram.mem, fd);
		$display("Loaded %0d bytes from test.bin", bytes_read);
		$fclose(fd);

		// Reset Sequence
		#10;
		@(posedge clk);
		rst = 0;

		// Hardware interrupt generator for WFI
		fork
			begin
				wait (u_cpu.u_id.u_cu.r_wfi == 1'b1);
				$display("[%0t] CPU entered WFI, asserting hardware interrupt ID 0x04", $time);
				#30;
				@(posedge clk);
				i_int = 1'b1;
				i_int_id = 7'h04;
				wait (o_int_ack == 1'b1);
				$display("[%0t] CPU acknowledged interrupt (o_int_ack = 1)", $time);
				@(posedge clk);
				i_int = 1'b0;
			end
			begin
				#3000;
			end
		join_any

		#400;

		$display("\n================ SIMULATION FINISHED ================");
		$display("Final Registers:");
		for (int i = 0; i < 8; i++) begin
			$display("r%0d = 0x%02X", i, u_cpu.u_id.u_rf.r_gpr[i]);
		end
		$display("mem[0x0200] = 0x%02X", u_ram.mem[16'h0200]);
		$display("mem[0x0204] = 0x%02X", u_ram.mem[16'h0204]);
		$display("=====================================================");

		$finish;
	end

endmodule
