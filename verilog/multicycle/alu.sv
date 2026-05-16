module alu(
	input  logic [15:0] i_a,
	input  logic [15:0] i_b,
	input  logic        i_c,
	input  logic [3:0]  i_op,
	output logic [15:0] o_result,
	output logic [4:0]  o_flags
);

	logic [16:0]	temp_res;
	logic [7:0]     w_a8;
	logic [7:0]     w_b8;

	always_comb begin
		w_a8 = i_a[7:0];
		w_b8 = i_b[7:0];
		case (i_op)
			4'h0: temp_res = i_a + i_b;
			4'h1: temp_res = i_a + i_b + {16'b0, i_c};
			4'h2: temp_res = i_a - i_b;
			4'h3: temp_res = i_a - i_b - {16'b0, ~i_c};
			4'h4: temp_res = {9'bX, w_a8 & w_b8};
			4'h5: temp_res = {9'bX, w_a8 | w_b8};
			4'h6: temp_res = {9'bX, ~(w_a8 | w_b8)};
			4'h7: temp_res = {9'bX, w_a8 ^ w_b8};
			4'h8: temp_res = {9'bX, w_a8 << i_b[2:0]};
			4'h9: temp_res = {9'bX, w_a8 >> i_b[2:0]};
			4'hA: temp_res = {9'bX, w_a8 >>> i_b[2:0]};
			4'hB: temp_res = {1'bX, i_a};
			4'hC: temp_res = {1'bX, i_b};
			default: temp_res = 17'bX;
		endcase
	end

	assign o_result = temp_res[15:0];

	always_comb begin
		if (i_op == 4'h0 || i_op == 4'h1) begin
			o_flags[3] = (i_a[7] == i_b[7]) && (i_a[7] != temp_res[7]); // Overflow
			o_flags[0] = temp_res[8]; // Carry
			o_flags[4] = temp_res[16]; // Extended Carry
		end else if (i_op == 4'h2 || i_op == 4'h3) begin
			o_flags[3] = (i_a[7] != i_b[7]) && (i_a[7] != temp_res[7]); // Overflow
			o_flags[0] = ~temp_res[8]; // inverted Carry
			o_flags[4] = ~temp_res[16]; // inverted Extended Carry
		end else begin
			o_flags[3] = 1'bX; // Overflow
			o_flags[0] = 1'bX; // Carry
			o_flags[4] = 1'bX; // Extended Carry
		end

		o_flags[1] = (temp_res[7:0] == 8'b0); // Zero
		o_flags[2] = temp_res[7]; // Negative
	end

endmodule