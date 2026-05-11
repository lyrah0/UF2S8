module alu(
	input  logic [7:0] i_a,
	input  logic [7:0] i_b,
	input  logic        i_c,
	input  logic [3:0]  i_op,
	output logic [7:0] o_result,
	output logic [3:0]  o_flags
);

	logic [8:0]	temp_res;
	logic		v_flag;

	always_comb begin

		case (i_op)
			4'h0: temp_res = i_a + i_b;
			4'h1: temp_res = i_a + i_b + {8'b0, i_c};
			4'h2: temp_res = i_a - i_b;
			4'h3: temp_res = i_a - i_b - {8'b0, i_c};
			4'h4: temp_res = {1'b0, i_a & i_b};
			4'h5: temp_res = {1'b0, i_a | i_b};
			4'h6: temp_res = {1'b0, ~(i_a | i_b)};
			4'h7: temp_res = {1'b0, i_a ^ i_b};
			4'h8: temp_res = {1'b0, i_a << i_b[2:0]};
			4'h9: temp_res = {1'b0, i_a >> i_b[2:0]};
			4'hA: temp_res = {1'b0, i_a >>> i_b[2:0]};
			4'hB: temp_res = {1'b0, i_a};
			4'hC: temp_res = {1'b0, i_b};
			default: temp_res = 9'bZ;
		endcase
	end

	assign o_result = temp_res[7:0];

	always_comb begin
		if (i_op == 4'h0 || i_op == 4'h1) begin
			v_flag = (i_a[7] == i_b[7]) && (i_a[7] != temp_res[7]);
		end else if (i_op == 4'h2 || i_op == 4'h3) begin
			v_flag = (i_a[7] != i_b[7]) && (i_a[7] != temp_res[7]);
		end else begin
			v_flag = 1'b0;
		end

		o_flags[0] = temp_res[8]; // Carry
		o_flags[1] = (temp_res[7:0] == 8'b0); // Zero
		o_flags[2] = temp_res[7]; // Negative
		o_flags[3] = v_flag; // Overflow
	end

endmodule