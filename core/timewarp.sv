// Module protection miss/hit

module timewarp
  import ariane_pkg::*;
#(
    parameter config_pkg::cva6_cfg_t CVA6Cfg = config_pkg::cva6_cfg_empty,
    parameter type dcache_req_o_t = logic,
    parameter int HIT_TIME = 10,
    parameter int STALL_COMMIT= 10
) (
    // Subsystem Clock - SUBSYSTEM
    input logic clk_i,
    // Asynchronous reset active low - SUBSYSTEM
    input logic rst_ni,
    // Lecture du cycle en cours dans csr_regfile 
    input logic csr_lecture_cycle,
    // HIT du Dcache 
    input logic dcache_hit_i,
    // Requete du Dcache 
    input dcache_req_o_t [2:0] dcache_req_i,
    // Lock commit 
    output logic protect_en_o
);
    logic [$clog2(HIT_TIME+1)-1:0] compteur_hit;
    logic [$clog2(STALL_COMMIT+1)-1:0] compteur_stall;

    logic hit_en ;

    always_comb begin : activation_stall

        protect_en_o = 1'b0; 
        if (hit_en && csr_lecture_cycle) begin 
            protect_en_o = 1'b1; 
            $display("TIMEWARP PROTECTION ENABLED");

        end else if (compteur_stall !=  0) begin
            protect_en_o = 1'b1;
        end 
    end
    
    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (~rst_ni) begin
            hit_en <= 1'b0;
            compteur_hit <= '0;
            compteur_stall <= '0;
        end else begin 

            if (dcache_hit_i) begin
                hit_en <= 1'b1; 
                compteur_hit <= HIT_TIME[$bits(compteur_hit)-1:0];

            end else if (compteur_hit != 0) begin
                compteur_hit <= compteur_hit - 1 ; 
                if (compteur_hit == 1) begin
                    hit_en <= 1'b0; 
                end
            end 

            if (hit_en && csr_lecture_cycle) begin 
                hit_en <= 1'b0; 
                compteur_stall <= STALL_COMMIT[$bits(compteur_stall)-1:0];
            end else if (compteur_stall !=  0) begin
                compteur_stall <= compteur_stall - 1 ; 
            end 

             if (csr_lecture_cycle) begin 
                $display("TIMEWARP PROTECTION");
            end
        end 
    end

    logic protect_en_q;
    logic hit_en_q;
    logic dcache_hit_q;
    logic csr_cycle_q;

    int nb_cycle;

    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            protect_en_q <= 0;
            hit_en_q <= 0;
            dcache_hit_q <= 0;
            csr_cycle_q <= 0;
            nb_cycle <= 0;
        end else begin

            if (dcache_req_i[1].data_rvalid) begin
                $display("[cycle %0d] LOAD RETURN TIMEWARP-> rid=%0d | dcache_hit_i=%0b | hit_en(cur)=%0b | csr_cycle=%0b | protect=%0b",
                        nb_cycle,
                        dcache_req_i[1].data_rid,
                        dcache_hit_i,
                        hit_en,
                        csr_lecture_cycle,
                        protect_en_o);
            end
            if (protect_en_o != protect_en_q)
                $display("[cycle %0d] protect_en_o -> %0b \n", nb_cycle, protect_en_o);

            if (hit_en != hit_en_q)
                $display("[cycle %0d] hit_en -> %0b \n ", nb_cycle, hit_en);

            if (csr_lecture_cycle != csr_cycle_q)
                $display("[cycle %0d] csr_lecture_cycle -> %0b \n" , nb_cycle, csr_lecture_cycle);

            protect_en_q <= protect_en_o;
            hit_en_q <= hit_en;
            dcache_hit_q <= dcache_hit_i;
            csr_cycle_q <= csr_lecture_cycle;

            nb_cycle <= nb_cycle + 1;

        end
    end

endmodule 
